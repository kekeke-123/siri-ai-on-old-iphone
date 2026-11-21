# Siri Plus Kernel

The kernel is a modified llama.cpp runtime with a C-only ABI (`spk.h`).

- `include/spk/spk.h` - the public API the app calls through `SPKBridge`
- `src/` - engine, session, sampler, vision, translate, ASR, guards
- `tools/spk_selftest.cpp` - headless load/prefill/decode/checkpoint test

Compatibility notes for the bundled models live in
[`docs/KERNEL-QWEN35.md`](../docs/KERNEL-QWEN35.md).

Build: `bash scripts/build-ios.sh` (iOS arm64). Selftest:
`SPK_SELFTEST_MODELS=0.8b bash scripts/kernel-selftest.sh`.
