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
# tellurium is someone's gaming computer: use it only in the hours its
# owner allows, and start a job only when an hour of the window is left.
# Until 2026-10-08 14:00 any time before 20:30; after that weekday and
# weekend evenings 17-21 and weekend mornings 07-12.
tellurium_free() {
    local now day hm
    now=$(date +%s); day=$(date +%u); hm=$((10#$(date +%H) * 60 + 10#$(date +%M)))
    if [ "$now" -lt "$(date -d '2026-10-08 13:00' +%s)" ]; then
        [ "$hm" -le $((19 * 60 + 30)) ]; return
    fi
    [ "$hm" -ge $((17 * 60)) ] && [ "$hm" -le $((20 * 60)) ] && return 0
    [ "$day" -ge 6 ] && [ "$hm" -ge $((7 * 60)) ] && [ "$hm" -le $((11 * 60)) ]
}
# radium is in use on evenings 18-21: start nothing from 17:00 to 21:00.
radium_free() {
    local hm=$((10#$(date +%H) * 60 + 10#$(date +%M)))
    # Owner's exception: free this evening until 2026-10-08 17:00.
    [ "$(date +%s)" -lt "$(date -d '2026-10-08 17:00' +%s)" ] && return 0
    [ "$hm" -lt $((17 * 60)) ] || [ "$hm" -ge $((21 * 60)) ]
}
host_free() { case $1 in tellurium) tellurium_free ;; radium) radium_free ;; *) true ;; esac; }
if [ "$host" != auto ] && ! host_free "$host"; then
    echo "offload: $host is not available now (owner's hours)" >&2
    exit 3
fi
if [ "$host" = auto ]; then
    # Builds prefer tellurium, test suites radium.  A host that does not
    # answer, or is outside its owner's hours, is skipped; if every reachable host
    # is full the job queues on the first reachable one.
    case $2 in build|ctest) order="tellurium radium" ;; *) order="radium tellurium" ;; esac
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
