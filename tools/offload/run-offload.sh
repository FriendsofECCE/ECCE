#!/bin/bash
# Run a long ECCE build or test suite on an offload machine.
#   run-offload.sh [--host radium|tellurium|auto] <git ref> <build|ctest [-R re]|teaching|apps|run ...> [args]
# The ref must already be pushed to GitHub.  See tools/offload/README.md.
set -eu
host=auto
if [ "${1:-}" = --host ]; then host=$2; shift 2; fi
if [ $# -lt 2 ]; then
    sed -n '2,4p' "$0" | sed 's/^# \{0,1\}//' >&2
    exit 2
fi
here=$(cd "$(dirname "$0")" && pwd)

# Per-host settings: work root, container or native, parallel jobs, ninja -j.
conf() {
    case $1 in
      radium)    echo 'OFF=$HOME/ecce-offload MODE=container SLOTS=2 JOBS=6' ;;
      tellurium) echo 'OFF=/mnt/games/ecce MODE=native SLOTS=3 JOBS=3 CTEST_JOBS=3' ;;
      *) echo "unknown host $1" >&2; return 1 ;;
    esac
}
# Free slots on a host (-1 if unreachable, within 5 s).
free() {
    ssh -o BatchMode=yes -o ConnectTimeout=5 "$1" "$(conf "$1"); n=0; mkdir -p \$OFF/locks;
      for i in \$(seq 1 \$SLOTS); do ( exec 9>\$OFF/locks/slot\$i; flock -n 9 ) && n=\$((n+1)); done; echo \$n" 2>/dev/null || echo -1
}
if [ "$host" = auto ]; then
    # Builds prefer tellurium, test suites radium.  A host that does not
    # answer (tellurium is off at night) is skipped; if every reachable host
    # is full the job queues on the first reachable one.
    case $2 in build|ctest) order="tellurium radium" ;; *) order="radium tellurium" ;; esac
    host= queue=
    for h in $order; do
        n=$(free "$h")
        [ "$n" -ge 0 ] || { echo "offload: $h unreachable, skipped" >&2; continue; }
        [ -n "$queue" ] || queue=$h
        if [ "$n" -gt 0 ]; then host=$h; break; fi
    done
    host=${host:-$queue}
    if [ -z "$host" ]; then
        echo "offload: no host reachable; run the command locally (niobium)" >&2
        exit 3
    fi
    echo "offload host: $host" >&2
fi
envs=$(conf "$host")
args=$(printf '%q ' "$@")
# remote-run.sh is sent on stdin, so the host needs no checkout to start from.
exec ssh -o BatchMode=yes -o ConnectTimeout=5 -o ServerAliveInterval=30 "$host" "env $envs bash -s -- $args" < "$here/remote-run.sh"
