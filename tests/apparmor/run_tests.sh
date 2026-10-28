#!/bin/sh
# Unit check of packaging/gateway/ecce-apparmor-mosquitto against a scratch
# /etc/apparmor.d: idempotent, keeps the site's lines, removal leaves them.
set -u
here="$(cd "$(dirname "$0")" && pwd)"
S="$here/../../packaging/gateway/ecce-apparmor-mosquitto"
T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT
export ECCE_APPARMOR_DIR="$T" ECCE_APPARMOR_NORELOAD=1 ECCE_HOME=/opt/ecce
L="$T/local/mosquitto"
fail=0
check() { if [ "$2" = "$3" ]; then echo "ok   $1"; else echo "FAIL $1 (got '$2', want '$3')"; fail=1; fi; }
count() { grep -c "$1" "$L" 2>/dev/null; }

"$S" install; check "no profile: nothing written" "$([ -e "$L" ] && echo yes || echo no)" no

echo "# the profile" >"$T/mosquitto"
"$S" install
check "created" "$(count '^# BEGIN ECCE')" 1
check "rule present" "$(count '^owner @{HOME}/.ECCE/\*\* rwk,')" 1
"$S" install
check "second run: one block" "$(count '^# BEGIN ECCE')" 1
check "second run: one END" "$(count '^# END ECCE')" 1

"$S" remove
check "remove of own-only file deletes it" "$([ -e "$L" ] && echo yes || echo no)" no

printf '/srv/site/** r,\n# site comment\n' >"$L"
"$S" install; "$S" install
check "site line kept" "$(count '^/srv/site/\*\* r,')" 1
check "block added once" "$(count '^# BEGIN ECCE')" 1
sed -i 's/^owner \/tmp\/ecce-\*\/\*\* rwk,/old rule,/' "$L"
"$S" install
check "stale block replaced" "$(count '^old rule,')" 0
check "replaced: rule back" "$(count '^owner /tmp/ecce-\*/\*\* rwk,')" 1
"$S" remove
check "site lines survive removal" "$(cat "$L")" "$(printf '/srv/site/** r,\n# site comment')"
"$S" remove
check "remove twice is harmless" "$(count '^/srv/site')" 1
exit $fail
