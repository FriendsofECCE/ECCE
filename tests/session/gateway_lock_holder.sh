#!/bin/bash
# #666: a stopped process holding .gateway.lock is named, with what to do,
# within a few seconds instead of a silent 90 s wait.
# Usage: gateway_lock_holder.sh SOURCE_DIR
set -u
src="${1:?source dir}"
lib="$src/packaging/gateway/ecce-session-lib.sh"
[ -d /proc/self/fd ] || { echo "skip: no /proc"; exit 0; }
command -v flock >/dev/null || { echo "skip: no flock"; exit 0; }
work="$(mktemp -d)"
lock="$work/.gateway.lock"
: > "$lock"
holder=""
cleanup() { [ -n "$holder" ] && kill -9 "$holder" 2>/dev/null; rm -rf "$work"; }
trap cleanup EXIT

# Takes the lock and stops itself, as the reaper's stopped helper did.
bash -c 'exec 9>>"$1"; flock 9; kill -STOP $$; exit 0' _ "$lock" &
holder=$!
for _ in $(seq 50); do
  grep -q '^State:[[:space:]]*T' "/proc/$holder/status" 2>/dev/null && break
  sleep 0.1
done
t0=$(date +%s)
out="$(timeout 8 bash -c '. "$1" && exec 9>>"$2" && ecce_lock_wait "$2" lock-test' _ "$lib" "$lock" 2>&1)"
dt=$(( $(date +%s) - t0 ))
echo "$out"
fail=0
case "$out" in
  *"held by pid $holder "*"stopped (Ctrl+Z?): run kill -CONT $holder or kill $holder"*) echo "ok   names the stopped holder" ;;
  *) echo "FAIL stopped holder $holder not named"; fail=1 ;;
esac
[ "$dt" -le 10 ] || { echo "FAIL took ${dt}s"; fail=1; }

# A free lock is taken at once, silently.
kill -9 "$holder"; wait "$holder" 2>/dev/null; holder=""
out="$(bash -c '. "$1" && exec 9>>"$2" && ecce_lock_wait "$2" lock-test && echo got' _ "$lib" "$lock" 2>&1)"
[ "$out" = got ] && echo "ok   free lock taken silently" || { echo "FAIL free lock: $out"; fail=1; }
exit $fail
