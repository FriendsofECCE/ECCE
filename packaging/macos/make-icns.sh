#!/bin/bash
# make-icns.sh <out.icns>: the application icon from packaging/desktop.
set -e
src=$(cd "$(dirname "$0")/../desktop" && pwd)
work=$(mktemp -d)
set_dir=$work/ecce.iconset
mkdir -p "$set_dir"
# The SVG is the one resolution-independent source and qlmanage renders
# it on any Mac; the 256 px PNG is the fallback.
qlmanage -t -s 1024 -o "$work" "$src/ecce.svg" >/dev/null 2>&1 || true
base=$work/ecce.svg.png
[ -s "$base" ] || { echo "make-icns: qlmanage cannot render ecce.svg, upscaling the 256 px PNG" >&2; base=$src/ecce-256.png; }
for s in 16 32 128 256 512; do
  sips -z $s $s "$base" --out "$set_dir/icon_${s}x${s}.png" >/dev/null
  sips -z $((s*2)) $((s*2)) "$base" --out "$set_dir/icon_${s}x${s}@2x.png" >/dev/null
done
iconutil -c icns "$set_dir" -o "$1"
rm -rf "$work"
