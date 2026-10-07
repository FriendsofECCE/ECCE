#!/bin/sh
# After-pictures for the login dialog and the backdrop fix from a build tree
# (for tools/offload/run-offload.sh <ref> run tests/look/login_backdrop.sh OUTDIR).
set -e
out=${1:?output directory}
bld=${BLD:?BLD}/b
cd "$(dirname "$0")/../.."
python3 tests/look/login_backdrop.py login "$out" --tag after --libdir "$bld"
python3 tests/look/login_backdrop.py backdrop "$out" --tag after --libdir "$bld"
