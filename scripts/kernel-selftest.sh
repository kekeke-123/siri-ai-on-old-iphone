#!/usr/bin/env bash
# Build the kernel for the host machine and run the headless smoke test on real
# GGUF weights. This is the CI counterpart of "does it actually answer?".
#
#   SPK_SELFTEST_MODELS=0.8b,0.5b   which weights to fetch and run
#   SPK_SELFTEST_METAL=1            use the host GPU (off by default: CI is x86)
#   SPK_SELFTEST_TOKENS=24          tokens per turn
#
# Models are cached in build/selftest/models, so repeated runs are cheap.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build/selftest"
MODELS="$BUILD/models"
HAVE="${SPK_SELFTEST_MODELS:-0.8b,0.5b}"

mkdir -p "$MODELS"

bash "$ROOT/scripts/fetch-runtime.sh"

cmake -S "$ROOT/kernel" -B "$BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DRUNTIME_DIR="$ROOT/third_party/runtime" \
    -DGGML_METAL="${SPK_SELFTEST_METAL:-OFF}" \
    -DGGML_OPENMP=OFF \
    -DGGML_RPC=OFF \
    -DBUILD_SHARED_LIBS=OFF \
    -DSPK_BUILD_SELFTEST=ON

cmake --build "$BUILD" --config Release -j "$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

BIN="$BUILD/spk_selftest"
if [ ! -x "$BIN" ]; then
    echo "== spk_selftest did not build at $BIN"
    exit 1
fi

fetch() {
    local name="$1" url="$2" min="$3"
    local out="$MODELS/$name"
    if [ -s "$out" ] && [ "$(wc -c < "$out" | tr -d ' ')" -ge "$min" ]; then
        echo "== $name cached"
        return 0
    fi
    echo "== fetching $name"
    curl -fL --retry 5 --retry-all-errors --max-time 1200 -C - -o "$out.part" "$url"
    mv "$out.part" "$out"
    local size
    size=$(wc -c < "$out" | tr -d ' ')
    if [ "$size" -lt "$min" ]; then
        echo "== $name is $size bytes, expected >= $min"
        exit 1
    fi
    echo "== $name ok ($size bytes)"
}

want() {
    case ",$HAVE," in
        *",$1,"*) return 0 ;;
        *) return 1 ;;
    esac
}

STATUS=0
run() {
    local label="$1" path="$2"
    echo
    echo "===== selftest: $label"
    if SPK_SELFTEST_METAL="${SPK_SELFTEST_METAL:-0}" \
       SPK_SELFTEST_TOKENS="${SPK_SELFTEST_TOKENS:-24}" \
       "$BIN" "$path"; then
        echo "===== $label PASS"
    else
        echo "===== $label FAIL"
        STATUS=1
    fi
}

if want 0.8b; then
    fetch "Qwen3.5-0.8B-Q4_K_M.gguf" \
        "https://huggingface.co/unsloth/Qwen3.5-0.8B-GGUF/resolve/main/Qwen3.5-0.8B-Q4_K_M.gguf" \
        500000000
    run "qwen35-0.8b (hybrid delta-net)" "$MODELS/Qwen3.5-0.8B-Q4_K_M.gguf"
fi

if want 2b; then
    fetch "Qwen3.5-2B-Q4_K_M.gguf" \
        "https://huggingface.co/unsloth/Qwen3.5-2B-GGUF/resolve/main/Qwen3.5-2B-Q4_K_M.gguf" \
        1200000000
    run "qwen35-2b (hybrid delta-net)" "$MODELS/Qwen3.5-2B-Q4_K_M.gguf"
fi

if want 0.5b; then
    fetch "qwen2.5-0.5b-instruct-q4_k_m.gguf" \
        "https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct-GGUF/resolve/main/qwen2.5-0.5b-instruct-q4_k_m.gguf" \
        450000000
    run "qwen2.5-0.5b (compat tier)" "$MODELS/qwen2.5-0.5b-instruct-q4_k_m.gguf"
fi

exit "$STATUS"
