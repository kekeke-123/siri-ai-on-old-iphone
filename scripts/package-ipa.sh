#!/usr/bin/env bash
# Package the unsigned .app into a TrollStore / sideload friendly IPA.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
APP="${1:-$ROOT/build/DerivedData/Build/Products/Release-iphoneos/SiriAI.app}"
OUT="$ROOT/build/out"

if [ ! -d "$APP" ]; then
    echo "app bundle not found at $APP" >&2
    exit 1
fi

rm -rf "$OUT"
mkdir -p "$OUT/Payload"
cp -R "$APP" "$OUT/Payload/"

# strip anything that would make a sideloaded install fail
rm -rf "$OUT/Payload/SiriAI.app/_CodeSignature" || true
find "$OUT/Payload/SiriAI.app" -name '*.dSYM' -exec rm -rf {} + 2>/dev/null || true

( cd "$OUT" && zip -qry "SiriAI-unsigned.ipa" "Payload" )
echo "== $OUT/SiriAI-unsigned.ipa =="
ls -lh "$OUT"
