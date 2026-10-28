#!/bin/bash
# Needs AppArmor active with Ubuntu 26.04's mosquitto profile, and sudo; CI
# only (it writes /etc/apparmor.d/local/mosquitto and uses the real $HOME).
# Proves the profile stops ecce-gateway-start's per-user broker, and that
# the block from ecce-apparmor-mosquitto lets it start and serve a client.
set -u
[ "${GITHUB_ACTIONS:-}" = true ] || { echo "CI only"; exit 77; }
repo="$(cd "$(dirname "$0")/../.." && pwd)"
export ECCE_HOME="$(mktemp -d)"
mkdir -p "$ECCE_HOME/bin" "$ECCE_HOME/siteconfig"
for f in ecce-gateway-start ecce-session-lib.sh ecce-find-mosquitto; do
  cp "$repo/packaging/gateway/$f" "$ECCE_HOME/bin/$f"
done
export ECCE_REALUSERHOME="$HOME" ECCE_SESSION_ID=apparmortest1
STATE="$HOME/.ECCE"

sudo systemctl stop mosquitto 2>/dev/null || true
sudo aa-status 2>/dev/null | grep -i mosquitto || echo "note: mosquitto profile not listed by aa-status"

stop_broker() {
  pkill -u "$(id -u)" -x mosquitto 2>/dev/null; sleep 1
  rm -rf "$STATE"
}

echo "=== without the block (must fail)"
sudo rm -f /etc/apparmor.d/local/mosquitto
sudo apparmor_parser -r /etc/apparmor.d/mosquitto
if "$ECCE_HOME/bin/ecce-gateway-start"; then
  echo "FAIL: the broker started without the block, so the profile is not what stops it"
  exit 1
fi
echo "--- denials:"; sudo dmesg 2>/dev/null | grep -i 'apparmor.*DENIED.*mosquitto' | tail -5
stop_broker

echo "=== with the block (must work)"
sudo ECCE_HOME=/opt/ecce "$repo/packaging/gateway/ecce-apparmor-mosquitto" install
sudo cat /etc/apparmor.d/local/mosquitto
"$ECCE_HOME/bin/ecce-gateway-start" || {
  echo "FAIL: broker did not start with the block"
  sudo dmesg 2>/dev/null | grep -i 'apparmor.*DENIED.*mosquitto' | tail -10
  cat "$STATE/mosquitto.out" 2>/dev/null
  exit 1
}
SOCK="$STATE/mosquitto.sock"
[ -S "$SOCK" ] || { echo "FAIL: no socket"; exit 1; }
out="$(mktemp)"
timeout 10 mosquitto_sub --unix "$SOCK" -t ecce/646 -C 1 >"$out" &
sleep 1
mosquitto_pub --unix "$SOCK" -t ecce/646 -m hello || { echo "FAIL: publish"; exit 1; }
wait
[ "$(cat "$out")" = hello ] || { echo "FAIL: message not received"; exit 1; }
echo "PASS: broker started under the profile and relayed a message"
stop_broker
echo "=== removal"
sudo "$repo/packaging/gateway/ecce-apparmor-mosquitto" remove
[ ! -e /etc/apparmor.d/local/mosquitto ] || { echo "FAIL: local file remains"; exit 1; }
exit 0
