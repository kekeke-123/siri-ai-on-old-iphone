# LLAI Kernel x Qwen 3.5

Compatibility record for the bundled Qwen family, in particular the **hybrid**
Qwen 3.5 models (`qwen35`, and the MoE sibling `qwen35moe`) that the app ships
as its default and high tier.

Everything below was checked against the runtime snapshot the kernel is built
from (`third_party/runtime`, tag `v1.0.0`, commit `b1b770d`). Paths are given so
each claim can be re-verified with one grep.

## 1. Architecture support upstream

| Item | Where | Status |
|---|---|---|
| arch registry `qwen35` / `qwen35moe` | `src/llama-arch.cpp:45` | present |
| model factory | `src/llama-model.cpp:351` | `llama_model_qwen35` / `llama_model_qwen35moe` |
| tensor/hparam loader | `src/models/qwen35.cpp` (669 lines) | present |
| hybrid memory wiring | `src/llama-model.cpp:3332` | per-layer filter: dense attn layers -> KV, recurrent layers -> recurrent state |
| sequence state API | `src/llama-context.cpp:4095` | `memory->state_write(io, seq_id, flags)` |

Qwen 3.5 is **not** a plain transformer: layer `i` is a gated delta net (linear
attention) unless `(i + 1) % full_attention_interval == 0` (default interval 4),
so only every 4th layer keeps a KV cache. That is why it runs on 4 GB phones at
all: the KV cache is a quarter of what a normal model needs at the same context
length, and the rest of the state is a fixed-size recurrent matrix.

Kernel consequence: `spk_info.recurrent` is filled from
`llama_model_is_recurrent()`, and the session checkpoint/restore path uses the
**per-sequence** state API (`llama_state_seq_get_size` / `_get_data` /
`_set_data`), not the whole-context one. On hybrid models that is the only
correct call, because the recurrent half has no KV-cache equivalent.

## 2. Operator coverage

The graph for a `qwen35` block needs these ops beyond dense attention
(`src/models/delta-net-base.cpp`, `src/models/qwen35.cpp`):

| Op | Used for | CPU | Metal |
|---|---|---|---|
| `GGML_OP_GATED_DELTA_NET` | the delta-rule recurrence itself | yes (`ggml-cpu.c:2096`, `2298`, `3034`) | yes (`ggml-metal-device.m:141` pipeline, `:1904` support check) |
| `GGML_OP_SSM_CONV` | short conv over q/k/v before the recurrence | yes | yes, needs `has_simdgroup_reduction` |
| `GGML_OP_SILU`, `SIGMOID`, `SOFTPLUS` | gating | yes | yes |
| `GGML_OP_CONCAT`, `ADD`, `MUL`, `GET_ROWS` | state assembly, embeddings | yes | yes |
| dense attention + QK RMS norm on non-recurrent layers | normal `build_attn` path | yes | yes |

Metal gate conditions that matter on device:

* `GGML_OP_GATED_DELTA_NET` requires `has_simdgroup_reduction` -> Apple GPU
  family 7 and newer, i.e. **A12 (iPhone XS/XR) and up**. Older A11 hardware
  falls back to the CPU for that op. Nothing breaks, it just gets slower, which
  is exactly why iPhone 11 and older are pinned to the Qwen 2.5 0.5B tier.
* It also requires `head_v_dim % 32 == 0`. Qwen 3.5 0.8B / 2B use 128, so the
  condition holds.
* Quantized `Q4_K_M` weights are dequantized to `f16` before the recurrence, so
  no special quantized kernel is needed. `MUL_MAT` / `MUL_MAT_ID` (the MoE
  variant's router) are the usual quantized paths and are already covered.

Conclusion: **all operators the Qwen 3.5 dense and hybrid paths need exist on
both backends the app uses**. The only hardware-dependent degradation is the
A11-and-below CPU fallback described above.

## 3. What this round changed in the kernel

| Change | Why |
|---|---|
| `spk_engine_arch()` (`spk_engine.cpp`) exposes and caches the arch name parsed from the model description | sampler, logs and the app status line all want it without loading hparams |
| arch-aware sampler defaults (`spk_sampler.cpp`): Qwen family -> `temp 0.7`, `top_p 0.8`, `top_k 20`, `repeat 1.05` | Qwen's own cards; the generic `40 / 0.95 / 1.1` set made the 0.8B model loop |
| ChatML fallback in `spk_apply_template` (`spk_session.cpp`) | the runtime template matcher only recognises known shapes; a Qwen 3.5 template that failed to match used to kill the whole turn. It now degrades to hand-built `<|im_start|>` ChatML and keeps the diagnostic in `last_error` |
| `kernel/tools/spk_selftest.cpp` + `scripts/kernel-selftest.sh` + `selftest.yml` | proves load / prefill / decode / checkpoint / restore on real weights in CI, including the hybrid path |

## 4. Known limits (honest list)

* **No 1.5B variant exists.** Qwen 3.5 ships 0.8B / 2B / 4B / 9B; the 1.5B slot
  is only covered by Qwen 2.5 1.5B, which the app does not bundle.
* **MoE (`qwen35moe`) is supported by the runtime but not bundled.** It needs
  `MUL_MAT_ID` and more RAM than a supported phone has to spare.
* **A11 and older** get the CPU fallback for the delta-net op, so they should
  stay on the 0.5B compat tier.
* **Thinking mode** is model-side. The kernel does not rewrite model output; if
  a template enables a reasoning block, the answer carries it. That is a
  prompt/latency choice, not a content filter.
* The kernel performs **no prompt filtering of any kind** (see
  `docs/PERFORMANCE.md`, content policy). Arch compatibility above is purely
  about tensors and ops.

## 5. How to re-verify

```bash
# 1. ops exist on both backends
rg -n "GATED_DELTA_NET" third_party/runtime/ggml/src/ggml-cpu third_party/runtime/ggml/src/ggml-metal
# 2. arch is registered
rg -n "qwen35" third_party/runtime/src/llama-arch.cpp third_party/runtime/src/llama-model.cpp
# 3. real weights, real tokens (x86 CI runner, CPU backend)
SPK_SELFTEST_MODELS=0.8b,0.5b bash scripts/kernel-selftest.sh
```

The `selftest` workflow runs step 3 on every change under `kernel/`, and its log
prints the model description, layer/embd counts, `recurrent`, TTFT, decode
throughput and the first line of the answer for each tier.
