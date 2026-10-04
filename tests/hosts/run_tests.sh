#!/bin/bash
# Two-host, two-account test of the central-server (mode 2) and shared-
# broker (mode 3) deployments (#223), in rootless podman, against the SHIPPED
# packages: ecce-server on one container, ecce-client on two others (accounts
# alice and bob), one network. Everything is checked by DELIVERY: Mosquitto
# acknowledges subscriptions it then serves nothing on.
#
#   tests/hosts/run_tests.sh <debs-dir> [mode2] [mode3]
#
# <debs-dir> holds ecce-client_*.deb and ecce-server_*.deb. Needs
# build-cmake/mqtt_test (or $MQTT_TEST): the real messaging library, used as
# the publisher so the topics and the refusal path are ECCE's own. No
# synthetic keyboard or pointer input anywhere: a session is started with
# `ecce -remote` and ended by a WM_DELETE_WINDOW on its login dialog (what a
# window manager's close button sends). What needs a real login is listed at
# the end for a human.
#
# Prints PASS/FAIL per check; exits nonzero if any failed.

set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
DEBS="${1:?usage: run_tests.sh <debs-dir> [mode2] [mode3]}"; shift
MODES="${*:-mode2 mode3}"
MQTT_TEST="${MQTT_TEST:-$REPO/build-cmake/mqtt_test}"
NET=ecce-hosts-net
IMG_C=localhost/ecce-hosts-client IMG_S=localhost/ecce-hosts-server
# What a Debian user's PATH holds: no /usr/sbin, where mosquitto lives.
DPATH=/usr/local/bin:/usr/bin:/bin:/usr/local/games:/usr/games
CTX="$(mktemp -d "${TMPDIR:-/tmp}/ecce-hosts.XXXXXX")"
PASS=0 FAIL=0 FAILED=()

cleanup() {
  podman rm -f -t 0 ecce-hosts-server ecce-hosts-alice ecce-hosts-bob >/dev/null 2>&1
  podman network rm "$NET" >/dev/null 2>&1
  [ "${ECCE_HOSTS_KEEP:-}" = 1 ] || podman rmi -f "$IMG_C" "$IMG_S" >/dev/null 2>&1
  rm -rf "$CTX"
}
trap cleanup EXIT

# --- helpers --------------------------------------------------------------

# check NAME COMMAND...: PASS when the command succeeds. Its output is shown
# on FAIL, which is what tells a package bug from a test bug.
check() {
  local name="$1" out; shift
  if out="$("$@" 2>&1)"; then
    echo "PASS  $name"; PASS=$((PASS+1))
  else
    echo "FAIL  $name"; FAIL=$((FAIL+1)); FAILED+=("$name")
    [ -n "$out" ] && printf '%s\n' "$out" | tail -n 6 | cut -c1-220 | sed 's/^/        /'
  fi
}
# ok NAME CONDITION-STRING: same, for a shell condition.
ok() { check "$1" bash -c "$2"; }
note() { echo "-- $*"; }

# As root in a container / as one of its accounts (a Debian-style PATH,
# working directory $WD).
R() { local c="ecce-hosts-$1"; shift; podman exec "$c" "$@"; }
A() {
  local c="ecce-hosts-$1" u="$2"; shift 2
  podman exec -w "${WD:-/}" "$c" runuser -u "$u" -- \
    env HOME="/home/$u" PATH="${APATH:-$DPATH}" "$@"
}

mk_net() {
  podman network rm "$NET" >/dev/null 2>&1
  podman network create "$NET" >/dev/null
}

# new_server: systemd as init, plus the capability the unit's own sandboxing
# (namespaces) needs inside a rootless container; the unit runs unmodified.
new_server() {
  podman run -d --name ecce-hosts-server --network "$NET" --hostname server \
    --systemd=always --cap-add=SYS_ADMIN --security-opt unmask=all \
    "$IMG_S" >/dev/null
  for _ in $(seq 30); do
    R server systemctl is-system-running 2>/dev/null | grep -qE 'running|degraded' && return 0
    sleep 1
  done
  return 1
}

# new_client NAME: a host with one account of that name.
new_client() {
  podman run -d --name "ecce-hosts-$1" --network "$NET" --hostname "$1-host" \
    "$IMG_C" >/dev/null
  podman cp "$HERE/client-session.sh" "ecce-hosts-$1:/usr/local/bin/client-session.sh"
  podman cp "$HERE/closewin.py" "ecce-hosts-$1:/usr/local/bin/closewin.py"
  podman cp "$MQTT_TEST" "ecce-hosts-$1:/usr/local/bin/mqtt_test"
  R "$1" useradd -m -s /bin/bash "$1"
}

PW_alice=alicepw PW_bob=bobpw PW_carol=carolpw
pw() { local v="PW_$1"; echo "${!v}"; }

# The environment of a session of USER, for the library probe.
sess_env() { echo "ECCE_HOME=/opt/ecce ECCE_REALUSERHOME=/home/$1 HOST=$1-host DISPLAY=:9"; }

PUB="mosquitto_pub -h server -p 8088 -V mqttv5 -q 1"
# pub HOST ACCOUNT PASSWORD TOPIC PAYLOAD: a plain MQTT publish from HOST's
# account of that name, PUBACK reason shown (v5 "Not authorized" = refused).
pub() { A "$1" "$1" $PUB -u "$2" -P "$3" -t "$4" -m "$5"; }

# sub_start HOST ACCOUNT FILE TOPIC...: mosquitto_sub as ACCOUNT, run on
# HOST; returns once the broker has acknowledged the subscription.
sub_start() {
  local h="$1" u="$2" f="$3" t="" x; shift 3
  for x in "$@"; do t="$t -t '$x'"; done
  podman exec -d "ecce-hosts-$h" runuser -u "$h" -- bash -c \
    "stdbuf -oL mosquitto_sub -h server -p 8088 -u $u -P $(pw "$u") -V mqttv5 -d -v $t -W 60 >$f 2>&1" >/dev/null
  for _ in $(seq 30); do
    A "$h" "$h" grep -q SUBACK "$f" 2>/dev/null && return 0
    sleep 0.5
  done
  return 1
}
# in HOST FILE PATTERN: does the subscriber's file hold a message matching?
in_() { A "$1" "$1" grep -Eq "^ecce/.*($3)" "$2"; }
none() { ! in_ "$@"; }
wait_for() { for _ in $(seq 40); do "$@" && return 0; sleep 0.5; done; return 1; }
stop_subs() { local u; for u in alice bob; do R "$u" pkill mosquitto_sub; done 2>/dev/null; true; }

# lib_pub HOST ACCOUNT PASSWORD SPEC...: the real messaging library, as a
# process of HOST's session, logging in as ACCOUNT.
lib_pub() {
  local h="$1" a="$2" p="$3"; shift 3
  A "$h" "$h" env $(sess_env "$h") MQTT_TEST_USER="$a" MQTT_TEST_PASS="$p" mqtt_test child "$@"
}

# --- the checks common to both modes -----------------------------------------

start_sessions() {
  local mode="$1" u
  for u in alice bob; do
    check "[$mode] $u: 'ecce -remote' starts and reaches the login dialog (central server and broker found)" \
      A "$u" "$u" client-session.sh start
  done
  for u in alice bob; do
    check "[$mode] $u: the session found the broker on the server (broker file: host=server port=8088)" \
      A "$u" "$u" bash -c "grep -qx host=server ~/.ECCE/broker_$u-host__9 && grep -qx port=8088 ~/.ECCE/broker_$u-host__9"
    check "[$mode] $u: no broker of the client's own (no mosquitto process, no pidfile)" \
      A "$u" "$u" bash -c '! pgrep -x mosquitto && [ ! -e ~/.ECCE/mosquitto.pid ]'
  done
}

two_subs() {
  sub_start alice alice /tmp/alice.sub "ecce/alice/#" "ecce/+/ecce_machreg_changed" &&
  sub_start bob bob /tmp/bob.sub "ecce/#" "ecce/alice/#" "ecce/+/ecce_url_created" "ecce/+/ecce_machreg_changed"
}
bob_attacks() {
  ATTACK="$({ pub bob bob bobpw ecce/alice/ecce_ejs_kill x_kill
              pub bob bob bobpw ecce/alice/ecce_url_created x_url
              pub bob bob bobpw ecce/alice/session/alice-host_:9/ecce_quit x_quit; } 2>&1)"
  [ "$(grep -c 'Not authorized' <<<"$ATTACK")" = 3 ] || { echo "$ATTACK"; return 1; }
}
alice_got_own() { in_ alice /tmp/alice.sub a_url && in_ alice /tmp/alice.sub a_kill && in_ alice /tmp/alice.sub a_poll; }
machreg_both() { in_ bob /tmp/bob.sub a_machreg && in_ alice /tmp/alice.sub b_machreg; }
wrong_refused() {
  local out; out="$(pub alice alice wrong ecce/alice/x x 2>&1)"; local rc=$?
  [ $rc -ne 0 ] && grep -q 'Not authorized' <<<"$out"
}
anon_refused() { ! A alice alice $PUB -t ecce/alice/x -m x; }
lib_refused() {
  local err; err="$(lib_pub alice alice wrongpw ecce_poll:w 2>&1)"
  echo "$err" | head -2
  grep -q 'refused' <<<"$err" && grep -q "'alice'" <<<"$err"
}

isolation() {
  local mode="$1"
  note "$mode: isolation by delivery"
  check "[$mode] alice and bob subscribe (SUBACK received)" two_subs
  # Alice's own processes; the site-wide message goes LAST, so bob getting
  # it shows the earlier ones were withheld rather than slow.
  check "[$mode] a process of alice published ecce_url_created, ecce_ejs_kill, a session message, ecce_machreg_changed through the real library" \
    lib_pub alice alice alicepw ecce_url_created:a_url ecce_ejs_kill:a_kill \
      ecce_poll:a_poll:alice-host_:9 ecce_machreg_changed:a_machreg
  check "[$mode] bob's publish into ecce/alice/ (ecce_ejs_kill, ecce_url_created, a session topic) is refused" bob_attacks
  check "[$mode] bob's publish into his own ecce/bob/ is accepted" \
    pub bob bob bobpw ecce/bob/ecce_machreg_changed b_machreg
  wait_for in_ alice /tmp/alice.sub b_machreg; wait_for in_ bob /tmp/bob.sub a_machreg; sleep 2
  check "[$mode] alice receives her own ecce_url_created, ecce_ejs_kill and session message" alice_got_own
  check "[$mode] ecce_machreg_changed reaches both (alice's at bob, bob's at alice)" machreg_both
  check "[$mode] bob receives none of alice's ecce_url_*, ecce_ejs_kill or session messages" \
    none bob /tmp/bob.sub 'a_url|a_kill|a_poll'
  check "[$mode] what bob tried to write into ecce/alice/ never reaches alice" \
    none alice /tmp/alice.sub 'x_kill|x_url|x_quit'
  stop_subs
  check "[$mode] a wrong password is refused at connect" wrong_refused
  check "[$mode] an anonymous client is refused on TCP" anon_refused
  check "[$mode] the library takes its refusal path on a wrong password (the broker refused 'alice')" lib_refused
}

carol_pub_refused() { pub alice carol carolpw ecce/alice/ecce_ejs_kill x 2>&1 | grep -q 'Not authorized'; }
carol_wrong() { ! pub alice carol nope ecce/carol/x x; }
carol_delivered() {
  sub_start alice carol /tmp/carol.sub "ecce/carol/#" &&
  lib_pub alice carol carolpw ecce_poll:c1:alice-host_:9 &&
  wait_for in_ alice /tmp/carol.sub c1
}
mk_carol_2() {
  A server ecce bash -c 'printf "carolpw\n" | htpasswd -i ~/.ECCE/dataserver/users carol 2>&1 &&
    grep -q "^carol:.apr1." ~/.ECCE/dataserver/users'
}
mk_carol_3() { printf 'carolpw\n' | podman exec -i ecce-hosts-server ecce-broker-setup --user carol; }

# An account made the 8.x way (data server's users file only; mode 2), or
# added to the running shared broker by ecce-broker-setup (mode 3).
late_account() {
  local mode="$1"
  note "$mode: an account added after the broker started"
  if [ "$mode" = mode2 ]; then
    check "[mode2] carol created the 8.x way: plain htpasswd on the data server's users file only" mk_carol_2
  else
    check "[mode3] carol added with ecce-broker-setup --user while the service runs" mk_carol_3
  fi
  check "[$mode] carol logs in to the broker through the real library and her message is delivered" carol_delivered
  check "[$mode] carol with a wrong password is refused" carol_wrong
  check "[$mode] the ACL stops carol writing into alice's topics" carol_pub_refused
  stop_subs
}

# Sessions end; the server's broker must not.
alive2() { R server pgrep -u ecce -x mosquitto >/dev/null; }
alive3() { R server systemctl is-active --quiet ecce-broker; }
no_assert() { ! A alice alice grep -E 'ASSERTION|Aborted|core dumped' /home/alice/ecce.log; }
bob_closes() { A bob bob client-session.sh close && "$ALIVE"; }
gw_stop() { A alice alice env $(sess_env alice) ECCE_REMOTE_SERVER=1 ecce-gateway-stop >/dev/null 2>&1; "$ALIVE"; }
reap2() { A server ecce ecce-gateway-reap --if-idle >/dev/null 2>&1; "$ALIVE"; }

session_end() {
  local mode="$1"
  ALIVE=alive2; [ "$mode" = mode3 ] && ALIVE=alive3
  note "$mode: session end and broker lifetime, broker on another host"
  check "[$mode] alice closes her login dialog: 'ecce' returns" A alice alice client-session.sh close
  check "[$mode] ...with no ASSERTION or abort on the way out" no_assert
  check "[$mode] the server's broker survives a client quitting" "$ALIVE"
  check "[$mode] the broker still answers bob's host" R bob nc -z server 8088
  check "[$mode] bob's session is unaffected by alice's quitting" A bob bob pgrep -u bob -f bin/gateway
  check "[$mode] bob quits too; the broker is still up" bob_closes
  check "[$mode] a client's ecce-gateway-stop (Quit and Stop Server) leaves the server's broker running" gw_stop
  if [ "$mode" = mode2 ]; then
    check "[mode2] the server account's own plain quit (reaper) leaves its marked broker running" reap2
    check "[mode2] the data server still answers" R alice nc -z server 8096
  else
    check "[mode3] the unit is still active (only systemctl stops it)" alive3
  fi
}

# --- mode 2 -----------------------------------------------------------------

adduser_all() { local u; for u in alice bob; do A server ecce ecce-dataserver-adduser -b "$u" "${u}pw" X Y || return 1; done; }
conf_listeners() {
  local c=/home/ecce/.ECCE/mosquitto.conf
  [ "$(R server grep -c '^listener 8088' $c)" = 1 ] && R server grep -qx 'listener 8088' $c ||
    { R server grep '^listener 8088' $c | head -3; return 1; }
}
copied() { R "$1" ecce-remote-setup server | grep -q "Copied the server's machine list"; }
no_curl_warning() { local o; o="$(R alice ecce-remote-setup server 2>&1)"; ! grep -q 'curl not found' <<<"$o" || { echo "$o" | grep -A1 curl; return 1; }; }

mode2() {
  note "=== mode 2: central server (ecce-remote-setup --server; the account's data server and broker) ==="
  mk_net
  new_server || { echo "FAIL  [mode2] server container did not boot"; FAIL=$((FAIL+1)); return; }
  new_client alice; new_client bob
  # A fresh dedicated server account, the documented steps in order.
  check "[mode2] ecce-remote-setup --server all on a fresh server account" \
    A server ecce ecce-remote-setup --server all
  A server ecce mkdir -p /home/ecce/work
  A server ecce touch /home/ecce/work/x
  A server ecce ecce-remote-setup --server all >/dev/null 2>&1
  check "[mode2] ecce-dataserver-start" A server ecce ecce-dataserver-start
  check "[mode2] ecce-dataserver-adduser alice and bob" adduser_all
  # ecce-gateway-start as a user runs it: Debian PATH, in a directory with files.
  local out
  out="$(WD=/home/ecce/work A server ecce ecce-gateway-start 2>&1)"
  check "[mode2] ecce-gateway-start finds mosquitto on a Debian user's PATH (it lives in /usr/sbin)" \
    bash -c "! grep -q 'no mosquitto broker installed' <<<\"\$1\"" _ "$out"
  check "[mode2] with listen 'all' the broker's config has ONE listener on 8088 (the '*' is not glob-expanded)" conf_listeners
  check "[mode2] the server's broker answers on 8088 from alice's host" wait_for R alice nc -z server 8088
  check "[mode2] the data server answers on 8096 from bob's host" wait_for R bob nc -z server 8096

  # Clients, as the docs say: root, ecce-remote-setup <server>.
  check "[mode2] ecce-remote-setup <server> copies the server's machine list on a stock client (needs curl)" no_curl_warning
  for u in alice bob; do
    check "[mode2] $u's host: ecce-remote-setup server (with curl installed)" copied "$u"
  done
  start_sessions mode2
  isolation mode2
  late_account mode2
  session_end mode2
}

# --- mode 3 -----------------------------------------------------------------

broker_up3() { R server systemctl is-active --quiet ecce-broker && R server ss -ltn | grep -q ':8088 '; }
users3() { local u; for u in alice bob; do printf '%spw\n' "$u" | podman exec -i ecce-hosts-server ecce-broker-setup --user "$u" >/dev/null || return 1; done; }
data3() {
  A server ecce ecce-dataserver-start >/dev/null 2>&1 && adduser_all >/dev/null
}
client3() { R "$1" ecce-remote-setup server >/dev/null && R "$1" ecce-broker-setup server:8088 >/dev/null; }
used_shared() { A "$1" "$1" grep -q "using the site's shared broker server:8088" "/home/$1/ecce.log"; }

mode3() {
  note "=== mode 3: shared broker, ecce-broker.service under systemd ==="
  mk_net
  new_server || { echo "FAIL  [mode3] server container did not boot"; FAIL=$((FAIL+1)); return; }
  new_client alice; new_client bob
  # The data server stays central, on the ecce account; the broker is the unit.
  A server ecce mkdir -p /home/ecce/.ECCE/dataserver
  A server ecce bash -c 'echo all > ~/.ECCE/dataserver/listen'
  check "[mode3] ecce-dataserver-start; accounts alice and bob" data3
  check "[mode3] ecce-broker-setup server:8088" R server ecce-broker-setup server:8088
  check "[mode3] ecce-broker-setup --user alice, --user bob" users3
  R server systemctl link /opt/ecce/server/systemd/ecce-broker.service >/dev/null 2>&1
  R server systemctl enable --now ecce-broker >/dev/null 2>&1
  sleep 3
  check "[mode3] ecce-broker.service is active and listening on 8088 (as shipped, non-loopback declaration)" broker_up3
  check "[mode3] the broker answers on 8088 from alice's host" wait_for R alice nc -z server 8088
  for u in alice bob; do
    check "[mode3] $u's host: ecce-remote-setup server; ecce-broker-setup server:8088" client3 "$u"
  done
  start_sessions mode3
  for u in alice bob; do
    check "[mode3] $u: the session used the site's shared broker (SharedBroker wins over -remote)" used_shared "$u"
  done
  isolation mode3
  late_account mode3
  session_end mode3
}

# --- run ----------------------------------------------------------------------

command -v podman >/dev/null || { echo "podman is not installed (apt install podman)"; exit 2; }
[ -x "$MQTT_TEST" ] || { echo "no $MQTT_TEST: build it (ninja mqtt_test in build-cmake) or set MQTT_TEST"; exit 2; }
cp "$DEBS"/ecce-client_*.deb "$CTX/ecce-client.deb" && cp "$DEBS"/ecce-server_*.deb "$CTX/ecce-server.deb" ||
  { echo "no ecce-client_*.deb / ecce-server_*.deb in $DEBS"; exit 2; }
note "building images (the shipped packages: $(dpkg-deb -f "$CTX/ecce-server.deb" Version))"
podman build -q -t "$IMG_C" -f "$HERE/Containerfile.client" "$CTX" >/dev/null || exit 2
podman build -q -t "$IMG_S" -f "$HERE/Containerfile.server" "$CTX" >/dev/null || exit 2

for m in $MODES; do "$m"; podman rm -f -t 0 ecce-hosts-server ecce-hosts-alice ecce-hosts-bob >/dev/null 2>&1; done

cat <<'EOF'

NOT COVERED (needs a login at the gateway's dialog, i.e. a human):
  - the gateway's own "Message broker refused the login" dialog (only the
    library's refusal path is exercised here)
  - a full session past login: Organizer opening, alice's Organizer updating
    on a job she launched and bob's not
  - Quit and Stop Server from the Organizer's menu
EOF
echo
echo "$PASS passed, $FAIL failed"
for f in "${FAILED[@]}"; do echo "  FAILED: $f"; done
[ "$FAIL" = 0 ]
