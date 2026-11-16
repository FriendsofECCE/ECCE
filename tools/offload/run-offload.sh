#!/bin/bash
# Run a long ECCE build or test suite on an offload machine.
#   run-offload.sh [--host nerdegg|niobium|beryllium|auto] <git ref> <build|ctest [-R re]|teaching|apps|apptest <test.py ...>|clip|run ...> [args]
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
      # Both hosts have 6 physical cores (nproc counts 12 threads) and are
      # shared: one job at a time, 4 build jobs, so 2 cores stay free.  Pre-merge
      # suites: ctesti -LE long (the release run keeps the long tests).
      # NerdEgg: 6 physical cores (nproc counts 12), shared: one job at a
      # time, 4 build jobs, so 2 cores stay free.  Pre-merge suites:
      # ctesti -LE long (the release run keeps the long tests).
      # Builds run outside Slurm for now; to go back through it,
      # BUILDWRAP="srun -c 4 --mem=12G --".
      # DISPLAY=:987 has no server: over ssh DISPLAY is unset and the
      # wrappers fall back to :0, the user's screen.
      nerdegg)   echo 'OFF=$HOME/ecce-offload MODE=native SLOTS=1 JOBS=4 CTEST_JOBS=3 KEEP=3 BUILDWRAP= DISPLAY=:987' ;;
      # The user's two desktops (niobium 8 threads, beryllium 20 cores):
      # whichever the user is not sitting at is an offload host.
      # beryllium is reached through its reverse tunnel (ssh alias).
      niobium)   echo 'OFF=$HOME/tmp/ecce/offload MODE=native SLOTS=1 JOBS=6 CTEST_JOBS=3 KEEP=3 BUILDWRAP=nice\ -n\ 10 DISPLAY=:987' ;;
      beryllium) echo 'OFF=$HOME/ecce-offload MODE=native SLOTS=3 JOBS=8 CTEST_JOBS=4 KEEP=3 BUILDWRAP=nice\ -n\ 10 DISPLAY=:987 PATH=/opt/orca/orca_6_1_1_linux_x86-64_shared_openmpi418_nodmrg:$HOME/opt/xtb-git/bin:$HOME/opt/crest-git/bin:/usr/local/bin:/usr/bin:/bin' ;;
      *) echo "unknown host $1" >&2; return 1 ;;
    esac
}
# Free slots on a host (-1 if unreachable, within 5 s).
on() {  # run a command line on a host; niobium is this machine
    if [ "$1" = niobium ]; then shift; bash -c "$*"; else
        local h=$1; shift; ssh -o BatchMode=yes -o ConnectTimeout=5 -o ServerAliveInterval=30 "$h" "$*"; fi
}
free() {
    on "$1" "$(conf "$1"); n=0; mkdir -p \$OFF/locks;
      for i in \$(seq 1 \$SLOTS); do ( exec 9>\$OFF/locks/slot\$i; flock -n 9 ) && n=\$((n+1)); done; echo \$n" 2>/dev/null || echo -1
}
# The desktop the user is at (niobium or beryllium) is not used by auto;
# set it with: echo beryllium > ~/.config/ecce-offload/desktop
desktop=$(cat "$HOME/.config/ecce-offload/desktop" 2>/dev/null || echo niobium)
host_free() { [ "$1" != "$desktop" ]; }
if [ "$host" != auto ] && ! host_free "$host"; then
    echo "offload: $host is the desktop in use (~/.config/ecce-offload/desktop); use --host only if the user agreed" >&2
    exit 3
fi
if [ "$host" = auto ]; then
    # NerdEgg first, then the user's desktop machine he is not at.
    # A host that does not answer is skipped; a full one queues the job.
    order="nerdegg beryllium niobium"
    host= queue=
    for h in $order; do
        if ! host_free "$h"; then
            echo "offload: $h is the desktop in use, skipped" >&2; continue
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
if [ "$host" = niobium ]; then
    exec nice -n 19 bash -c "env $envs bash -s -- $args" < "$here/remote-run.sh"
fi
exec ssh -o BatchMode=yes -o ConnectTimeout=5 -o ServerAliveInterval=30 "$host" "env $envs bash -s -- $args" < "$here/remote-run.sh"
