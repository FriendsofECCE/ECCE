#!/bin/bash
# remote-reach.sh <root>: the -remote start of ecce-gateway-start (#133)
# from an installed tree, <root>/bin holding the wrappers and <root>/ecce
# ECCE_HOME (ECCE.app's Contents/Resources, or a staging prefix). A broker
# port and a data server on this machine stand in for a central server:
#   - the wrapper finds ECCE_HOME without being told,
#   - a server that answers gives a broker file naming it,
#   - one that does not gives exit 2 and the reason for each port.
set -u
root=$(cd "$1" && pwd)
work=$(mktemp -d)
pid=""
trap '[ -n "$pid" ] && kill "$pid" 2>/dev/null; rm -rf "$work"' EXIT
fail() { echo "remote-reach: FAIL: $*" >&2; exit 1; }

# Two listening ports, written to $work/ports once both accept.
python3 -c '
import socket, sys
ss = []
for _ in range(2):
    s = socket.socket(); s.bind(("127.0.0.1", 0)); s.listen(16); ss.append(s)
open(sys.argv[1] + ".tmp", "w").write(" ".join(str(s.getsockname()[1]) for s in ss) + "\n")
import os; os.rename(sys.argv[1] + ".tmp", sys.argv[1])
import select
while True:
    for s in select.select(ss, [], [])[0]:
        c, _ = s.accept(); c.close()
' "$work/ports" &
pid=$!
for _ in $(seq 50); do [ -s "$work/ports" ] && break; sleep 0.2; done
read -r bport dport < "$work/ports" || fail "no test listener"

mkdir -p "$work/remote" "$work/home"
cat > "$work/remote/DataServers" <<EOF
<DataServers>
  <Server>
    <Url>http://127.0.0.1:$dport/Ecce</Url>
  </Server>
</DataServers>
EOF

start() {
  env -u ECCE_HOME -u ECCE_SESSION_ID HOME="$work/home" ECCE_REALUSERHOME="$work/home" \
    ECCE_REMOTE_SERVER=1 ECCE_REMOTE_DIR="$work/remote" ECCE_BROKER_PORT="$bport" \
    PATH=/usr/bin:/bin:/usr/sbin:/sbin "$root/bin/ecce-gateway-start" > "$work/out" 2>&1
}

start; rc=$?
cat "$work/out"
[ "$rc" = 0 ] || fail "server answering: exit $rc"
bf=$(ls "$work/home/.ECCE"/broker_* 2>/dev/null | head -n1)
[ -n "$bf" ] || fail "no broker file"
grep -qx "host=127.0.0.1" "$bf" && grep -qx "port=$bport" "$bf" || { cat "$bf"; fail "broker file does not name 127.0.0.1:$bport"; }
echo "remote-reach: server answering: ok ($(tr '\n' ' ' < "$bf"))"

kill "$pid"; wait "$pid" 2>/dev/null; pid=""
rm -f "$work/home/.ECCE"/broker_*
start; rc=$?
cat "$work/out"
[ "$rc" = 2 ] || fail "server down: exit $rc, expected 2"
for p in "broker 127.0.0.1:$bport: Connection refused" "data server 127.0.0.1:$dport: Connection refused"; do
  grep -qF "$p" "$work/out" || fail "server down: message lacks '$p'"
done
echo "remote-reach: server down: ok"
