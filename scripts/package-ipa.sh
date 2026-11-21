#!/usr/bin/env bash
# Package the built .app into an IPA that third-party signers can re-sign.
#
# The bundle is built with CODE_SIGNING_ALLOWED=NO, so the main binary has no
# LC_CODE_SIGNATURE slot at all. Re-signing tools read that as "CodeDirectory
# does not match Mach-O", try to grow the slot, and give up halfway leaving an
# uninstallable package. An ad-hoc signature closes that hole: it writes a real
# CodeDirectory plus CodeResources, which any signer can then replace wholesale.
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

APP_PATH="$OUT/Payload/$(basename "$APP")"

find "$APP_PATH" -name '*.dSYM' -exec rm -rf {} + 2>/dev/null || true

# nested code first, then the container
if [ -d "$APP_PATH/PlugIns/SiriAIWidget.appex" ]; then
    echo "== ad-hoc sign widget extension =="
    codesign --force --sign - --timestamp=none \
        "$APP_PATH/PlugIns/SiriAIWidget.appex"
fi

for fw in "$APP_PATH"/Frameworks/*; do
    [ -e "$fw" ] || continue
    echo "== ad-hoc sign $(basename "$fw") =="
    codesign --force --sign - --timestamp=none "$fw"
done

echo "== ad-hoc sign app =="
codesign --force --sign - --timestamp=none "$APP_PATH"

# verify, but do not fail the build on it: some resource-heavy bundles trip
# --strict on file mode bits that no signer actually cares about
if ! codesign --verify --deep --strict "$APP_PATH" 2>&1; then
    echo "warning: ad-hoc signature verification reported problems" >&2
fi

echo "== embedded signature slot =="
/usr/bin/codesign -d --verbose=2 "$APP_PATH/SiriAI" 2>&1 | head -5 || true

( cd "$OUT" && zip -qry "SiriAI-unsigned.ipa" "Payload" )
echo "== $OUT/SiriAI-unsigned.ipa =="
ls -lh "$OUT"
