#!/usr/bin/env bash
# Package the built .app into a sideload friendly IPA.
#
# Resign tools need the main Mach-O to carry an LC_CODE_SIGNATURE slot. Our app
# is built with CODE_SIGNING_ALLOWED=NO, so it has none, and the tools try to grow
# the binary to make room and then give up ("CodeDirectory 与 Mach-O 不一致"),
# leaving an IPA that installs as an empty icon. We sign ad hoc at the Mach-O
# level on the runner: that is cheap, needs no certificate and gives the tools a
# valid CodeDirectory to replace.
#
# A full bundle seal is attempted too, but it is not fatal: codesign treats
# Metadata.appintents as a nested bundle on recent macOS and refuses the outer
# envelope. The resign tools recompute the seal anyway.
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

APP="$OUT/Payload/SiriAI.app"
EXE="$APP/$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$APP/Info.plist")"

# 1. Mach-O level: this is the part the resign tools actually require
echo "== ad hoc sign executables"
find "$APP" -maxdepth 4 -type f -perm -u+x \
    \( -path '*.appex/*' -o -name "$(basename "$EXE")" \) -print | sort -r > "$OUT/.machos"
while IFS= read -r bin; do
    file "$bin" | grep -q 'Mach-O' || continue
    echo "   $(basename "$bin")"
    codesign --force --sign - --timestamp=none "$bin"
done < "$OUT/.machos"
rm -f "$OUT/.machos"

# 2. bundle level, best effort
for bundle in "$APP/PlugIns/SiriAIWidget.appex" "$APP"; do
    echo "== ad hoc seal $(basename "$bundle")"
    if ! codesign --force --sign - --timestamp=none "$bundle"; then
        echo "   warning: bundle seal skipped for $(basename "$bundle")"
    fi
done

codesign -dv "$EXE" 2>&1 | sed -n '1,4p' || true

echo "== package"
( cd "$OUT" && zip -qry "SiriAI-unsigned.ipa" "Payload" )
echo "== $OUT/SiriAI-unsigned.ipa =="
ls -lh "$OUT"
