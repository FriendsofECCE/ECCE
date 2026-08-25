#!/bin/bash
# Usage: build_mod.sh <src-subdir-relative-to-src> [make-args...]
export ECCE_HOME=/tmp/ecce
export ECCE_SHARED_LIBS=false
cd "$ECCE_HOME"
source build/build_setup.sh
MOD="$1"
shift
cd "src/$MOD"
make "$@"
