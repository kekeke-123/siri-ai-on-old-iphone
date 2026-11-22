#!/usr/bin/env bash
# Rebrand the kernel to LLAI and cut the first 1.0 release line.
#
# Deliberately text-only: the C symbol prefix (spk_) is a stable ABI that the
# shipped app, the DLL contract and the Windows import library already depend
# on, so it is kept and documented as such. Everything an operator reads -
# version string, product name, docs, banner - becomes LLAI.
#
# Idempotent: a second run finds nothing to change.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "== version: 1.0.0 =="
sed -i 's/#define SPK_VERSION_STRING "1\.4\.0"/#define SPK_VERSION_STRING "1.0.0"/' \
    kernel/include/spk/spk.h

echo "== product name: LLAI =="
files=$(git grep -l -e "Siri Plus Kernel" -e "Siri Plus 内核" || true)
if [ -n "$files" ]; then
    for f in $files; do
        sed -i \
            -e 's/Siri Plus Kernel/LLAI Kernel/g' \
            -e 's/Siri Plus 内核（SPK）/LLAI 内核/g' \
            -e 's/Siri Plus 内核/LLAI 内核/g' \
            "$f"
    done
fi

echo "== app-facing version label =="
sed -i 's/return "1\.4\.2"/return "LLAI 1.0.0"/' app/Sources/SettingsView.swift 2>/dev/null || true

echo "== change summary =="
git diff --stat

if git diff --quiet; then
    echo "nothing to rebrand"
else
    echo "rebranded to LLAI 1.0.0"
fi
