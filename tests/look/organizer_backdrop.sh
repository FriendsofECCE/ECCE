#!/bin/sh
# For tools/offload/run-offload.sh <ref> run tests/look/organizer_backdrop.sh OUTDIR
# after an `apps` run has installed the tree.
set -e
export ECCE_TEST_HOME=${BLD:?BLD}/install ECCE_TEST_WRAPPERS=$BLD/install-bin
cd "$(dirname "$0")/../.."
python3 tests/look/organizer_backdrop.py "${1:-$BLD/lb-org}"
