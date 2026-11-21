#!/usr/bin/env bash
# Fetch the pinned runtime and apply the Siri Plus Kernel patch series.
# Everything lands in third_party/runtime, which is git-ignored.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TAG="${RUNTIME_TAG:-v1.0.0}"
REPO="${RUNTIME_REPO:-https://github.com/ggml-org/llama.cpp}"
DIR="$ROOT/third_party/runtime"

echo "== fetching runtime $TAG =="
rm -rf "$DIR"
mkdir -p "$ROOT/third_party"
git clone --depth 1 --branch "$TAG" "$REPO" "$DIR"

echo "== applying kernel patches =="
cd "$DIR"
for p in "$ROOT"/patches/*.patch; do
    echo "  $(basename "$p")"
    git apply --whitespace=nowarn "$p"
done

echo "== runtime ready at $DIR =="
