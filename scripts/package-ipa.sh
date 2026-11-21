#!/usr/bin/env bash
# Package the built .app into a sideload friendly IPA.
#
# Default output is raw and unsigned: no embedded signature, no _CodeSignature.
# Phone side resign tools expect exactly that (they strip whatever they find and
# then build their own CodeDirectory + CMS). Set SPK_ADHOC=1 to embed an ad hoc
# signature instead, which some desktop tools prefer.
#
# The build already links with -no_fixup_chains, so the main Mach-O carries the
# classic LC_DYLD_INFO_ONLY rebase/bind tables that older resign engines can
# actually parse. Chained fixups made them compute wrong offsets and abort with
# "CodeDirectory 与 Mach-O 不一致".
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
# Live Activities are deferred: the extension also broke third party resign tools.
rm -rf "$APP/PlugIns"

echo "== load commands of main executable =="
EXE="$APP/$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$APP/Info.plist")"
otool -l "$EXE" 2>/dev/null | grep -E 'LC_DYLD_INFO_ONLY|LC_DYLD_CHAINED_FIXUPS|LC_CODE_SIGNATURE' | sed 's/^/   /' || true

if [ "${SPK_ADHOC:-0}" = "1" ]; then
    echo "== ad hoc signing (SPK_ADHOC=1)"
    while IFS= read -r nested; do
        [ -e "$nested" ] || continue
        codesign --force --sign - --timestamp=none "$nested"
    done < <(find "$APP" -maxdepth 4 \( -name '*.appex' -o -name '*.dylib' \) -print | sort -r)
    codesign --force --sign - --timestamp=none "$APP"
else
    echo "== raw unsigned package (no ad hoc signature)"
    rm -rf "$APP/_CodeSignature"
    find "$APP" -name '_CodeSignature' -type d -exec rm -rf {} + 2>/dev/null || true
fi

echo "== package"
( cd "$OUT" && zip -qry "SiriAI-unsigned.ipa" "Payload" )
echo "== $OUT/SiriAI-unsigned.ipa =="
ls -lh "$OUT"
