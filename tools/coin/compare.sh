#!/bin/bash
# Side-by-side headless comparison of the vendored-Inventor and Coin3D
# viewer builds (#166, stages 2-4).
#
#   tools/coin/compare.sh [outdir]          # default: build-coin-compare/
#
# Needs two configured build trees, each with its own private install
# prefix (cmake -DCMAKE_INSTALL_PREFIX/-DECCE_HOME_DIR/-DECCE_WRAPPER_
# DESTINATION, see tests/apps/README):
#   VENDORED_BUILD (default build-oiv)     configured with ECCE_USE_COIN=OFF
#   COIN_BUILD     (default build-cmake)   configured with ECCE_USE_COIN=ON
# and Xvfb.  Runs only on private displays :170-:179.  Env: NO_BUILD=1
# skips building/installing, ONLY=builtin|calc renders just that half
# (keeps the other half of raw/), JOBS (default 6), ECCE_XVFB.
#
# Output: <outdir>/<scene>/{vendored,coin,diff}.png, summary.txt (every
# pair, metrics), contact-sheet.png (only pairs differing beyond tolerance
# after the vendored green cast of #83 is removed), raw/ (PPM renders).
set -eo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=$(realpath -m "${1:-$ROOT/build-coin-compare}")
VEND=$(realpath -m "${VENDORED_BUILD:-$ROOT/build-oiv}")
COIN=$(realpath -m "${COIN_BUILD:-$ROOT/build-cmake}")
JOBS=${JOBS:-6}
export ECCE_TEST_XDISPLAYS=170-179 LIBGL_ALWAYS_SOFTWARE=1

cache() { sed -n "s/^$2:[A-Z]*=//p" "$1/CMakeCache.txt"; }

[ "$(cache "$VEND" ECCE_USE_COIN)" = OFF ] || { echo "$VEND is not a vendored build" >&2; exit 1; }
[ "$(cache "$COIN" ECCE_USE_COIN)" = ON ]  || { echo "$COIN is not a Coin build" >&2; exit 1; }

if [ -z "$NO_BUILD" ]; then
  for b in "$VEND" "$COIN"; do
    ninja -C "$b" -j"$JOBS"
    ninja -C "$b" -j"$JOBS" viewer-scenes
    cmake --install "$b" >/dev/null
  done
fi

[ -n "$ONLY" ] || rm -rf "$OUT/raw"
mkdir -p "$OUT/raw"
for pair in "vendored:$VEND" "coin:$COIN"; do
  name=${pair%%:*}; b=${pair#*:}
  # The wrappers run $ECCE_HOME/bin/builder; check it is this tree's build.
  home=$(cache "$b" ECCE_HOME_DIR)
  cmp -s "$home/bin/builder" "$b/builder" ||
    { echo "$home/bin/builder is not the build in $b; run without NO_BUILD" >&2; exit 1; }
  echo "== rendering $name ($b)"
  python3 "$ROOT/tools/coin/compare.py" render "$name" "$b" "$home" \
    "$(cache "$b" ECCE_WRAPPER_DESTINATION)" "$OUT/raw" ${ONLY:+--only $ONLY}
done

python3 "$ROOT/tools/coin/compare.py" diff "$OUT/raw" "$OUT"
echo "contact sheet: $OUT/contact-sheet.png"
