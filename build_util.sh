#!/bin/bash
export ECCE_HOME=/tmp/ecce
export ECCE_SHARED_LIBS=false
cd "$ECCE_HOME"
source build/build_setup.sh
echo "ECCE_SYSDIR=$ECCE_SYSDIR"
echo "ECCE_MAKEINC_DIR=$ECCE_MAKEINC_DIR"
cd src/util
make "$@"
