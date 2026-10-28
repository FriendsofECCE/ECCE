#!/bin/bash
# #666: the ssh control directory belongs to the state directory, so a new
# session reuses the old one's name. Neither the new session's start sweep
# nor the last program of an old session may remove it once `ecce` has
# claimed it; an unclaimed one is still reaped.
# Usage: control_dir_claim.sh SOURCE_DIR
set -u
src="${1:?source dir}"
gw="$src/packaging/gateway"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
mkdir -p "$work/run" "$work/home/.ECCE" "$work/ecce/bin"
export XDG_RUNTIME_DIR="$work/run" ECCE_REALUSERHOME="$work/home" \
       ECCE_HOME="$work/ecce" ECCE_SESSION_LIVENESS=proc
unset ECCE_SSH_CONTROL_DIR ECCE_GATEWAY_LOCKED
. "$gw/ecce-session-lib.sh"
fail=0

setup_old() {   # a directory left by an earlier session, long unclaimed
  ecce_ssh_control_setup
  dir="$ECCE_SSH_CONTROL_DIR"
  touch -d '-10 minutes' "$dir/.born" "$dir/.claim"
}
check() {  # name, want (kept|gone)
  if [ -d "$dir" ]; then got=kept; else got=gone; fi
  if [ "$got" = "$2" ]; then echo "ok   $1"; else echo "FAIL $1: $got"; fail=1; fi
}

# The starting session's own sweep (ecce-gateway-start) leaves its directory.
setup_old
ECCE_GATEWAY_LOCKED=1 ECCE_REAP_QUIET=1 "$gw/ecce-gateway-reap"
check "start sweep keeps the starting session's directory" kept

# Another session's last program exits between `ecce` and the gateway lock.
setup_old
ecce_ssh_control_setup            # what `ecce` does: renews .claim
ECCE_REAP_QUIET=1 "$gw/ecce-gateway-reap" --if-idle
check "a claimed directory survives another session's reaper" kept

# Unclaimed and no program left: reaped as before.
setup_old
ECCE_REAP_QUIET=1 "$gw/ecce-gateway-reap" --if-idle
check "an unclaimed directory is reaped" gone

# ecce-gateway-start makes it again when it went before the lock.
ecce_ssh_control_setup
check "setup recreates a reaped directory" kept
exit $fail
