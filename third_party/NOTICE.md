# Third-party attribution

## Runtime

The LLAI Kernel (`kernel/`) is a self-contained C/C++ library that links
against the open source inference runtime **llama.cpp**, licensed under the MIT
license, copyright the ggml authors.

- Upstream: https://github.com/ggml-org/llama.cpp
- Pinned tag: `v1.0.0` (commit `b1b770d90d94cad9e9ea5b993bdbc383a522fb28`)
- It is **not vendored** in this repository. `scripts/fetch-runtime.sh` clones
  it at build time and applies the patches in `patches/`.
- The upstream `LICENSE` file is kept intact inside the fetched tree and ships
  with every build. Our patches are derivative works of that tree and remain
  under the same MIT terms.

`kernel/src/spk_core.h` is the single translation unit allowed to reference
upstream identifiers; the rest of the kernel exposes only `spk_*` names.

## Trademarks

"Siri", "iPhone", "Apple Intelligence", "Dynamic Island" and "Shortcuts" are
trademarks of Apple Inc. This project is an independent, clean-room
implementation. It contains no Apple source code, model weights, fonts, sounds
or UI assets, and is not affiliated with or endorsed by Apple.

## Model weights

Model files (GGUF) are downloaded by the user and are not distributed here.
Each model carries its own license; check it before shipping anything built
with it.
