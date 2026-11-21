#!/usr/bin/env bash
# Package the built .app into a sideload friendly IPA.
#
# Why we sign at all: the app is built with CODE_SIGNING_ALLOWED=NO, so the main
# Mach-O has no LC_CODE_SIGNATURE. Resign tools then try to grow the binary to
# make room and give up with "CodeDirectory 与 Mach-O 不一致", producing an IPA
# that installs as an empty icon. An ad hoc pass on the runner is free and leaves
# a valid CodeDirectory the tools can replace with the user's certificate.
#
# Metadata.appintents is dropped: the training processor already failed to parse
# it on the runner, and codesign refuses to touch any bundle that contains it
# (it is recognised as a malformed nested bundle).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC_APP="${1:-$ROOT/build/DerivedData/Build/Products/Release-iphoneos/SiriAI.app}"
OUT="$ROOT/build/out"

if [ ! -d "$SRC_APP" ]; then
    echo "app bundle not found at $SRC_APP" >&2
    exit 1
fi

rm -rf "$OUT"
mkdir -p "$OUT/Payload"
cp -R "$SRC_APP" "$OUT/Payload/"

APP="$OUT/Payload/SiriAI.app"
find "$APP" -name '*.dSYM' -exec rm -rf {} + 2>/dev/null || true
rm -rf "$APP/Metadata.appintents"

EXE="$APP/$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$APP/Info.plist")"

# nested code first: widget extension, then any embedded dylibs
while IFS= read -r nested; do
    [ -e "$nested" ] || continue
    echo "== ad hoc seal ${nested#$APP/}"
    codesign --force --sign - --timestamp=none "$nested"
done < <(find "$APP" -maxdepth 4 \( -name '*.appex' -o -name '*.dylib' \) -print | sort -r)

echo "== ad hoc seal SiriAI.app"
if ! codesign --force --sign - --timestamp=none "$APP"; then
    # codesign still resolves the surrounding bundle even for a single file, so
    # sign the executable outside the bundle and copy it back
    echo "== fallback: sign executable out of tree"
    TMP="$(mktemp -d)"
    cp "$EXE" "$TMP/SiriAI"
    codesign --force --sign - --timestamp=none "$TMP/SiriAI"
    cp "$TMP/SiriAI" "$EXE"
    rm -rf "$TMP"
fi

codesign -dv "$EXE" 2>&1 | sed -n '1,4p' || true

if [ -f "$APP/_CodeSignature/CodeResources" ]; then
    echo "== bundle seal present"
else
    echo "== warning: no bundle seal, executable signature only"
fi

echo "== package"
( cd "$OUT" && zip -qry "SiriAI-unsigned.ipa" "Payload" )
echo "== $OUT/SiriAI-unsigned.ipa =="
ls -lh "$OUT"
