#!/usr/bin/env bash
# Build the LLAI Kernel as device (arm64) static libraries.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build/kernel"

cmake -S "$ROOT/kernel" -B "$BUILD" \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=16.0 \
    -DCMAKE_BUILD_TYPE=Release \
    -DRUNTIME_DIR="$ROOT/third_party/runtime" \
    -DGGML_METAL=ON \
    -DGGML_METAL_EMBED_LIBRARY=ON \
    -DGGML_OPENMP=OFF \
    -DGGML_RPC=OFF \
    -DBUILD_SHARED_LIBS=OFF

cmake --build "$BUILD" --config Release -j "$(sysctl -n hw.logicalcpu)"

echo "== static libraries =="
find "$BUILD" -name '*.a' | sort
