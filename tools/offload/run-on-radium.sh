#!/bin/sh
# Run a long ECCE build or test suite on radium instead of this machine.
#   run-on-radium.sh <git ref> <build|ctest [-R re]|teaching|apps|run ...> [args]
# The ref must already be pushed to GitHub.  See tools/offload/README.md.
set -eu
if [ $# -lt 2 ]; then
    sed -n '2,4p' "$0" | sed 's/^# \{0,1\}//' >&2
    exit 2
fi
here=$(cd "$(dirname "$0")" && pwd)
host=${RADIUM_HOST:-radium}
args=$(printf '%q ' "$@")
# remote-run.sh is sent on stdin, so radium needs no checkout to start from.
exec ssh -o BatchMode=yes -o ServerAliveInterval=30 "$host" "bash -s -- $args" < "$here/remote-run.sh"
