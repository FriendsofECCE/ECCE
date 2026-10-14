#!/bin/bash
# make-icns.sh <out.icns>: the application icon from packaging/desktop.
set -e
src=$(cd "$(dirname "$0")/../desktop" && pwd)
work=$(mktemp -d)
set_dir=$work/ecce.iconset
mkdir -p "$set_dir"
# The 1024 px PNG comes from ecce.svg via make-icon.py.  qlmanage is no
# substitute: it draws an SVG as a Quick Look page, the 128 px icon in the
# corner of a white 1024 px sheet.
base=$src/ecce-1024.png
for s in 16 32 128 256 512; do
  sips -z $s $s "$base" --out "$set_dir/icon_${s}x${s}.png" >/dev/null
  sips -z $((s*2)) $((s*2)) "$base" --out "$set_dir/icon_${s}x${s}@2x.png" >/dev/null
done
iconutil -c icns "$set_dir" -o "$1"
rm -rf "$work"
