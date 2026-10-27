#!/bin/bash
# Run a long ECCE build or test suite on an offload machine.
#   run-offload.sh [--host radium|nerdegg|auto] <git ref> <build|ctest [-R re]|teaching|apps|apptest <test.py ...>|clip|run ...> [args]
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
      radium)    echo 'OFF=$HOME/ecce-offload MODE=native SLOTS=2 JOBS=6 CTEST_JOBS=4 KEEP=8' ;;
      # Other agents share nerdegg's CPUs through Slurm, so the build asks it
      # for them; test jobs go through Slurm by their own --queue slurm.
      nerdegg)   echo 'OFF=$HOME/ecce-offload MODE=native SLOTS=1 JOBS=6 CTEST_JOBS=2 KEEP=3 BUILDWRAP="srun -c 6 --"' ;;
      *) echo "unknown host $1" >&2; return 1 ;;
    esac
}
# Free slots on a host (-1 if unreachable, within 5 s).
free() {
    ssh -o BatchMode=yes -o ConnectTimeout=5 "$1" "$(conf "$1"); n=0; mkdir -p \$OFF/locks;
      for i in \$(seq 1 \$SLOTS); do ( exec 9>\$OFF/locks/slot\$i; flock -n 9 ) && n=\$((n+1)); done; echo \$n" 2>/dev/null || echo -1
}
# radium is in use on evenings 18-21: start nothing from 17:00 to 21:00,
# unless its owner said it is free (OFFLOAD_IGNORE_HOURS=1).
radium_free() {
    [ "${OFFLOAD_IGNORE_HOURS:-0}" = 1 ] && return 0
    local hm=$((10#$(date +%H) * 60 + 10#$(date +%M)))
    [ "$hm" -lt $((17 * 60)) ] || [ "$hm" -ge $((21 * 60)) ]
}
host_free() { case $1 in radium) radium_free ;; *) true ;; esac; }
if [ "$host" != auto ] && ! host_free "$host"; then
    echo "offload: $host is not available now (owner's hours)" >&2
    exit 3
fi
if [ "$host" = auto ]; then
    # radium is the only host.
    # A host that does not answer is skipped; a full one queues the job.
    order="radium"
    host= queue=
    for h in $order; do
        if ! host_free "$h"; then
            echo "offload: $h outside its hours, skipped" >&2; continue
        fi
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
