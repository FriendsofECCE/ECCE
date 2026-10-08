#!/bin/bash
# bundle-python.sh <dest>: a relocatable CPython with wxPython in <dest>
# (ECCE.app/Contents/Resources/python), for the machine's own architecture.
# The Theory/Runtype Details dialogs, the first-start window and the other
# Python helpers run on it, so a Mac needs no Python of its own (#133).
# python-build-standalone is built for macOS 10.9 (Intel) / 11.0 (Apple
# silicon); the wxPython wheels for 10.13 / 11.0, so both are within the
# deployment targets the app is built for (minos.py checks).
set -euo pipefail
DEST=$1
PBS_TAG=20261003
PY_VERSION=3.13.16
WX_VERSION=4.2.4
case "$(uname -m)" in
  arm64) triple=aarch64-apple-darwin ;;
  x86_64) triple=x86_64-apple-darwin ;;
  *) echo "bundle-python: unknown architecture $(uname -m)" >&2; exit 1 ;;
esac
name="cpython-$PY_VERSION+$PBS_TAG-$triple-install_only_stripped.tar.gz"
url=https://github.com/astral-sh/python-build-standalone/releases/download/$PBS_TAG

W=$(mktemp -d)
trap 'rm -rf "$W"' EXIT
curl -fsSL --retry 3 "$url/${name//+/%2B}" -o "$W/py.tgz"
curl -fsSL --retry 3 "$url/SHA256SUMS" -o "$W/sums"
want=$(grep -F " $name" "$W/sums" | awk '{print $1}')
got=$(shasum -a 256 "$W/py.tgz" | awk '{print $1}')
[ -n "$want" ] && [ "$want" = "$got" ] || { echo "bundle-python: checksum mismatch for $name" >&2; exit 1; }
tar -xzf "$W/py.tgz" -C "$W"
rm -rf "$DEST"
mkdir -p "$(dirname "$DEST")"
mv "$W/python" "$DEST"

# The wheel must be chosen for the macOS the interpreter was built for, not
# for the runner's deployment target.
env -u MACOSX_DEPLOYMENT_TARGET "$DEST/bin/python3" -m pip install \
  --disable-pip-version-check --no-deps --no-compile --only-binary=:all: \
  "wxPython==$WX_VERSION"

# Not needed to run the dialogs: the package tools, Tk, docs, tests, headers.
P=$DEST/lib/python3.13
rm -rf "$DEST/include" "$DEST/share" "$DEST/lib/pkgconfig" \
  "$DEST"/lib/tcl* "$DEST"/lib/tk* "$DEST"/lib/itcl* "$DEST"/lib/thread* "$DEST"/lib/libtcl* \
  "$DEST"/bin/pip* "$DEST"/bin/idle* "$DEST"/bin/pydoc* "$DEST"/bin/*-config \
  "$P/ensurepip" "$P/idlelib" "$P/tkinter" "$P/turtledemo" "$P/pydoc_data" "$P/test" \
  "$P"/lib-dynload/_tkinter* \
  "$P"/site-packages/pip "$P"/site-packages/pip-* "$P"/site-packages/setuptools* \
  "$P"/site-packages/wx/*.pyi "$P"/site-packages/wx/include "$P"/site-packages/wx/py \
  "$P"/site-packages/wxPython*/direct_url.json
find "$DEST" -name __pycache__ -type d -prune -exec rm -rf {} +

# What the dialogs need, in the state they will run in.
env -i PATH=/usr/bin:/bin "$DEST/bin/python3" -c 'import wx, sys; print("bundled", sys.version.split()[0], "wxPython", wx.version())'
du -sh "$DEST"
