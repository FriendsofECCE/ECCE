#!/bin/bash
# make-app.sh <stage-dir> <out-dir>: ECCE.app and ECCE-<version>.dmg from a
# `cmake --install` into <stage-dir>/ecce (ECCE_HOME) and <stage-dir>/bin
# (the wrappers).  macOS only.  ECCE_DEPS names the prefix that
# build-deps.sh built; without it Homebrew supplies what gets bundled.
set -euo pipefail
STAGE=$(cd "$1" && pwd)
mkdir -p "$2"
OUT=$(cd "$2" && pwd)
HERE=$(cd "$(dirname "$0")" && pwd)

full=$(sed -n 's/^ECCE_VERSION_STRING=//p' "$STAGE/bin/ecce" | head -1 | tr -d "'\"")
VERSION=$(echo "${full#v}" | sed 's/^\([0-9]*\.[0-9]*\.[0-9]*\).*/\1/')
[ -n "$VERSION" ] || { echo "make-app: no version in $STAGE/bin/ecce" >&2; exit 1; }
# Info.plist needs the numeric version; the file name keeps the full one (alpha, rc).
FULLVER="${full#v}"

APP=$OUT/ECCE.app
RES=$APP/Contents/Resources
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$RES" "$APP/Contents/Frameworks"
cp -R "$STAGE/ecce" "$RES/ecce"
cp -R "$STAGE/bin" "$RES/bin"

# The broker and its password tool go in libexec, which the launcher
# puts on PATH for ecce-find-mosquitto.
mkdir -p "$RES/ecce/libexec"
for t in mosquitto mosquitto_passwd; do
  if [ -n "${ECCE_DEPS:-}" ]; then
    p=$(ls "$ECCE_DEPS/sbin/$t" "$ECCE_DEPS/bin/$t" 2>/dev/null | head -1 || true)
  else
    p=$(command -v "$t" || true)
    [ -n "$p" ] || p=$(ls "$(brew --prefix)/sbin/$t" "$(brew --prefix)/bin/$t" 2>/dev/null | head -1)
  fi
  [ -n "$p" ] || { echo "make-app: $t not found" >&2; exit 1; }
  cp -L "$p" "$RES/ecce/libexec/$t"
done

sed "s/@VERSION@/$VERSION/" "$HERE/Info.plist.in" > "$APP/Contents/Info.plist"
install -m 755 "$HERE/launcher.sh" "$APP/Contents/MacOS/ecce"
"$HERE/make-icns.sh" "$RES/ecce.icns"

python3 "$HERE/bundle_libs.py" "$APP/Contents/Frameworks" "$RES/ecce"

# Every Mach-O in the bundle, for the Homebrew check and for signing.
machos=$(mktemp)
find "$APP" -type f | while read -r f; do
  case "$(head -c4 "$f" | xxd -p)" in
    cffaedfe|cafebabe|feedfacf) echo "$f" ;;
  esac
done > "$machos"
echo "make-app: $(wc -l < "$machos") Mach-O files"
if xargs otool -L < "$machos" 2>/dev/null | grep -E "/(opt/homebrew|usr/local)/${ECCE_DEPS:+|$ECCE_DEPS/}"; then
  echo "make-app: references to Homebrew remain (above)" >&2; exit 1
fi

# install_name_tool voids signatures and arm64 will not run unsigned
# code, so each Mach-O is signed ad hoc, the bundle last.
while read -r f; do codesign --force -s - "$f"; done < "$machos"
rm -f "$machos"
codesign --force -s - "$APP"
codesign --verify --verbose=2 "$APP"

rm -f "$OUT"/ECCE-*.dmg
dmgdir=$(mktemp -d)
cp -R "$APP" "$dmgdir/"
ln -s /Applications "$dmgdir/Applications"
hdiutil create -volname "ECCE $VERSION" -srcfolder "$dmgdir" -ov -format UDZO "$OUT/ECCE-$FULLVER-$(uname -m).dmg"
rm -rf "$dmgdir"
ls -l "$OUT"/ECCE-*.dmg
