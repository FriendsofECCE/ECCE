#!/bin/bash
# #1007: the reap sweep removes an mqtt-*.sock a crash left behind, tested by
# connecting, and never the socket of a live broker.
# Usage: stale_broker_socket.sh SOURCE_DIR EBROKER
set -u
src="${1:?source dir}"; eb="${2:?eccebroker}"
gw="$src/packaging/gateway"
work="$(mktemp -d)"
trap 'kill $bpid 2>/dev/null; rm -rf "$work"' EXIT
mkdir -p "$work/run" "$work/home/.ECCE" "$work/ecce/libexec" "$work/ecce/bin"
chmod 700 "$work/home/.ECCE"
ln -s "$eb" "$work/ecce/libexec/eccebroker"
export XDG_RUNTIME_DIR="$work/run" ECCE_REALUSERHOME="$work/home" \
       ECCE_HOME="$work/ecce" ECCE_SESSION_LIVENESS=proc HOST=testhost \
       ECCE_SESSION_ID=a1b2c3d4e5f60718 DISPLAY=:7
unset ECCE_HOST ECCE_NO_MESSAGING ECCE_GATEWAY_LOCKED
fail=0
check() { if [ "$2" = "$3" ]; then echo "ok   $1"; else echo "FAIL $1: $3"; fail=1; fi; }
exists() { if [ -e "$1" ]; then echo kept; else echo gone; fi; }

# A live broker (its socket in ~/.ECCE), and two stale sockets: a socket file
# nobody listens on, in the state directory and in a runtime-directory one.
"$eb" --serve 60 >/dev/null 2>&1 || { echo "FAIL eccebroker did not start"; exit 1; }
live="$(sed -n 's/^socket=//p' "$work/home/.ECCE"/broker_*)"
bpid="$(pgrep -f "eccebroker --serve 60" | head -n1)"
mkstale() {
  python3 -c 'import socket,sys; s=socket.socket(socket.AF_UNIX); s.bind(sys.argv[1]); s.close()' "$1"
}
stale1="$work/home/.ECCE/mqtt-deadbeef.sock"
mkdir -m 700 "$work/run/ecce-$(id -u)-dead"
stale2="$work/run/ecce-$(id -u)-dead/mqtt-cafe0000.sock"
mkstale "$stale1"; mkstale "$stale2"
check "stale sockets made" "$(exists "$stale1")$(exists "$stale2")" keptkept

# One a broker file names is not probed (a probe is a connection to a live broker).
stale3="$work/home/.ECCE/mqtt-named000.sock"
mkstale "$stale3"
echo "socket=$stale3" > "$work/home/.ECCE/broker_otherhost_0000000000000000"
ECCE_GATEWAY_LOCKED=1 ECCE_REAP_QUIET=1 "$gw/ecce-gateway-reap"
check "a sweep removes the stale socket in the state directory" "$(exists "$stale1")" gone
check "and the stale socket in a runtime-directory folder" "$(exists "$stale2")" gone
check "and that folder" "$(exists "$(dirname "$stale2")")" gone
check "a socket a broker file names is left for the file's sweep" "$(exists "$stale3")" kept
check "a live broker's socket is kept" "$(exists "$live")" kept
"$eb" --alive "$live"; check "and still answers" "$?" 0
kill "$bpid"; sleep 1
exit $fail
