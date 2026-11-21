#!/usr/bin/env bash
# Download the bundled weights into app/Resources/Models.
# Skipped entirely when SPK_SKIP_MODELS=1 (used when iterating on code only).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEST="$ROOT/app/Resources/Models"

if [ "${SPK_SKIP_MODELS:-0}" = "1" ]; then
    echo "== bundled models skipped (SPK_SKIP_MODELS=1)"
    exit 0
fi

mkdir -p "$DEST"

#'       name|url|minimum bytes (a partial download must never be baked into the IPA)
FULL_MODELS="
Qwen3.5-2B-Q4_K_M.gguf|https://huggingface.co/unsloth/Qwen3.5-2B-GGUF/resolve/main/Qwen3.5-2B-Q4_K_M.gguf|1200000000
Qwen3.5-0.8B-Q4_K_M.gguf|https://huggingface.co/unsloth/Qwen3.5-0.8B-GGUF/resolve/main/Qwen3.5-0.8B-Q4_K_M.gguf|500000000
"

# 低配包：0.8B + Qwen2.5-0.5B。2B 版 Q4_K_M 一个人就 1.28 GB，三个全塞进去
# IPA 会超过 GitHub Release 单文件 2 GB 的上限，所以拆成两个 flavor。
COMPAT_MODELS="
Qwen3.5-0.8B-Q4_K_M.gguf|https://huggingface.co/unsloth/Qwen3.5-0.8B-GGUF/resolve/main/Qwen3.5-0.8B-Q4_K_M.gguf|500000000
qwen2.5-0.5b-instruct-q4_k_m.gguf|https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct-GGUF/resolve/main/qwen2.5-0.5b-instruct-q4_k_m.gguf|450000000
"

case "${SPK_MODEL_SET:-full}" in
    compat) MODELS="$COMPAT_MODELS" ;;
    full|"") MODELS="$FULL_MODELS" ;;
    *) echo "unknown SPK_MODEL_SET=${SPK_MODEL_SET}"; exit 2 ;;
esac

fetch_one() {
    local name="$1" url="$2" min="$3"
    local out="$DEST/$name"
    if [ -s "$out" ]; then
        local have
        have=$(wc -c < "$out" | tr -d ' ')
        if [ "$have" -ge "$min" ]; then
            echo "== $name already present ($have bytes)"
            return 0
        fi
        echo "== $name too small ($have < $min), refetching"
        rm -f "$out"
    fi
    echo "== fetching $name"
    curl -fL --retry 6 --retry-delay 5 --retry-all-errors \
         --max-time 1800 -C - -o "$out.part" "$url"
    mv "$out.part" "$out"
    local size
    size=$(wc -c < "$out" | tr -d ' ')
    if [ "$size" -lt "$min" ]; then
        echo "== $name is $size bytes, expected >= $min"
        rm -f "$out"
        exit 1
    fi
    echo "== $name ok ($size bytes)"
}

while IFS='|' read -r name url min; do
    [ -z "${name:-}" ] && continue
    fetch_one "$name" "$url" "$min"
done <<< "$MODELS"

du -sh "$DEST"
ls -l "$DEST"
