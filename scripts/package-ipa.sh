#!/usr/bin/env bash
# Package the built .app into a sideload friendly IPA.
#
# We ad hoc sign on the runner before zipping. Resign tools (Sideloadly, AltStore
# and the phone side resigners) cannot work on a Mach-O that carries no
# LC_CODE_SIGNATURE at all: they try to grow the binary to make room for one and
# then abort with "CodeDirectory 与 Mach-O 不一致", leaving an incomplete IPA that
# installs and crashes. An ad hoc pass takes a couple of seconds and leaves a real
# code slot plus a CodeResources seal, which those tools can then replace cleanly
# with the user's own certificate.
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

# strip debug symbols, they only bloat the download
find "$OUT/Payload/SiriAI.app" -name '*.dSYM' -exec rm -rf {} + 2>/dev/null || true

# nested code first, the outer bundle seals it afterwards
find "$OUT/Payload/SiriAI.app" -maxdepth 4 \
    \( -name '*.appex' -o -name '*.dylib' -o -name '*.framework' \) \
    -print | sort -r > "$OUT/.nested"
while IFS= read -r nested; do
    [ -e "$nested" ] || continue
    echo "== ad hoc sign ${nested#$OUT/Payload/}"
    codesign --force --sign - --timestamp=none "$nested"
 done < "$OUT/.nested"
rm -f "$OUT/.nested"

echo "== ad hoc sign SiriAI.app"
codesign --force --sign - --timestamp=none "$OUT/Payload/SiriAI.app"
codesign -dv "$OUT/Payload/SiriAI.app" 2>&1 | sed -n '1,5p'

# a real seal means the resign tools have something to replace
if [ ! -f "$OUT/Payload/SiriAI.app/_CodeSignature/CodeResources" ]; then
    echo "CodeResources missing after signing" >&2
    exit 1
fi

( cd "$OUT" && zip -qry "SiriAI-unsigned.ipa" "Payload" )
echo "== $OUT/SiriAI-unsigned.ipa =="
ls -lh "$OUT"
