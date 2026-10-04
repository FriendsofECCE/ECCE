#!/bin/bash
# Viewer frame-time benchmark: builds viewer-bench, runs it, saves the table.
#
# Paste-ready, on the machine with the real GPU, in an X11 GNOME session
# (nothing to install beyond the packages in GETTING_STARTED.md section 1):
#
#   git fetch origin wip/viewer-bench   # or: git clone <repo>, then fetch
#   git checkout wip/viewer-bench
#   tools/viewer-bench/run.sh
#
# It builds in ./build-cmake (the first build compiles the viewer libraries,
# several minutes), opens one 800x800 window for about a minute and a half
# (do not cover it or move the pointer over it), and prints the path of the
# results file: build-cmake/viewer-bench-<host>-<date>.txt.  Send that file.
#
# Optional: BENCH_FRAMES=360 BENCH_SECONDS=8 tools/viewer-bench/run.sh
# (frames per run / time cap per run; defaults 180 / 4).
set -eo pipefail

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"

if [ -z "$DISPLAY" ]; then
  echo "DISPLAY is not set; run this from the desktop session." >&2
  exit 1
fi

BUILD=$ROOT/build-cmake
mkdir -p "$BUILD"
cd "$BUILD"
[ -f build.ninja ] || cmake -G Ninja ..
ninja ${JOBS:+-j$JOBS} viewer-bench

OUT=$BUILD/viewer-bench-$(hostname -s)-$(date +%Y%m%d-%H%M).txt

# ECCE_HOME=<repo> is enough: the program only reads data/client from it.
# vblank_mode / __GL_SYNC_TO_VBLANK turn vsync off for Mesa / NVIDIA so the
# frame times are not pinned to the monitor refresh; x11 keeps it on GLX.
STATE=$(mktemp -d)
trap 'rm -rf "$STATE"' EXIT
export ECCE_HOME=$ROOT ECCE_REALUSERHOME=$STATE
export GDK_BACKEND=x11 vblank_mode=0 __GL_SYNC_TO_VBLANK=0

{
  echo "viewer-bench $(git -C "$ROOT" rev-parse --short HEAD) on $(hostname -s), $(date -Is)"
  echo "session: ${XDG_SESSION_TYPE:-?} ${XDG_CURRENT_DESKTOP:-?}"
  "$BUILD/viewer-bench"
} 2>&1 | tee "$OUT"

echo
echo "Results written to: $OUT"
