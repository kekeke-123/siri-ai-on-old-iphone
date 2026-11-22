# Porting the LLAI Kernel

The kernel is portable C++17. A new host means: build the pinned runtime, build
`kernel/`, install an ops table, then write a thin bridge in the host\'s own
language. Nothing in `kernel/src` includes UIKit, Foundation or any other Apple
header - that separation is enforced by review, not by the compiler.

## What is platform specific

| Concern | Where it lives | iOS | Android | Linux | Windows |
|---|---|---|---|---|---|
| Physical RAM | `spk_platform.cpp` | sysctl | override ops | sysconf | GlobalMemoryStatusEx |
| Core count | `spk_platform.cpp` | hardware_concurrency | override ops | hardware_concurrency | GetActiveProcessorCount |
| Thermal | host side | ProcessInfo | PowerManager | /sys | none |
| GPU | `spk_make_ctx_params` | Metal | Vulkan (TODO) | CUDA/CPU | CPU |
| mmap model | runtime | yes | yes | yes | yes |
| Log sink | `spk_engine_set_log_callback` | os_log | logcat | stderr | OutputDebugString |

Overriding the table is the whole port:

```c
static uint64_t ram(void * ud)   { return 4ull * 1024 * 1024 * 1024; }
static int32_t  cores(void * ud) { return 8; }
static int32_t  heat(void * ud)  { return 0; }
static spk_platform_ops ops = { ram, cores, heat, NULL };
spk_platform_set_ops(&ops);
```

## Android

1. `scripts/fetch-runtime.sh` pins the runtime; cross-compile it with the NDK
   toolchain, `-DGGML_VULKAN=ON` once the Vulkan backend is stable on your SoC.
2. Build `kernel/` as a shared library (`SPK_LINK_SHARED` is Windows-only today,
   on Android pass `-DCMAKE_SHARED_LIBRARY` yourself) and `System.loadLibrary`.
3. Call the same C entry points through the NDK. No JNI wrapper is required
   beyond a `native` declaration per function.
4. Path policy already accepts `/data/data/<pkg>/files/...`; the guard only
   rejects relative paths.

## Sizing rules the kernel applies

Derived, not configured by the app:

- `n_ctx`: 6 GB+ RAM -> 4096, 4 GB+ -> 3072, else 2048.
- `n_batch` / `n_ubatch`: 512 / 256 on 6 GB+, else 256 / 128.
- threads: platform core count, capped at 4 - past four the graph barrier on a
  1B model costs more than the parallel gain.
- model guard: reject anything above 45% of physical RAM unless
  `cfg.max_model_gb` says otherwise.

A host that knows better (a desktop, a server) sets `cfg.n_ctx`, `cfg.n_batch`
and `cfg.n_threads` explicitly and the derivation is skipped.

## GPU policy

Weights may sit on the GPU; K/V cache and attention stay on the CPU. That split
is deliberate: on the A14 the fully offloaded Qwen 3.5 graph failed its first
decode, and keeping the quantised cache off the GPU removed the failure while
keeping the matmuls accelerated. `cfg.use_metal` only switches the weight path.

## Adding a backend

A backend touches exactly two places: `spk_make_ctx_params` (per-layer offload
flags) and the runtime build flags. The engine, session, sampler and guard code
never learn about it.
