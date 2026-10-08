#!/bin/bash
# The Python inside ECCE.app (#133): with no other Python on the PATH, the
# first-start window opens and a Theory Details dialog builds.  Everything
# runs in the app's own environment (ECCE_APP_RUN, see launcher.sh) whose
# PATH holds only the bundled Python and /usr/bin and friends, so a Python
# or wxPython of the machine's own cannot be what answers.
# Usage: python-check.sh <ECCE.app> <out-dir>
APP=$(cd "$1" && pwd)
mkdir -p "$2"
OUT=$(cd "$2" && pwd)
RES=$APP/Contents/Resources
REPO=$(cd "$(dirname "$0")/../.." && pwd)
SUMMARY=$OUT/summary.txt
: > "$SUMMARY"
rc=0
say() { echo "$@" | tee -a "$SUMMARY"; }
fail() { say "FAIL: $*"; rc=1; }
inapp() { ECCE_APP_RUN=1 "$APP/Contents/MacOS/ecce" "$@"; }
limit() { local s=$1; shift; perl -e 'alarm shift; exec @ARGV' "$s" "$@"; }

export ECCE_REALUSERHOME=$OUT/home
rm -rf "$ECCE_REALUSERHOME"; mkdir -p "$ECCE_REALUSERHOME/.ECCE"

py=$(inapp /bin/sh -c 'command -v python3')
say "python3 in the app's environment: $py"
[ "$py" = "$RES/python/bin/python3" ] || fail "python3 is not the bundled one"
inapp python3 -c 'import sys, wx; print(sys.version.split()[0], "wxPython", wx.version(), sys.prefix)' \
  2>&1 | tee -a "$SUMMARY" | grep -q wxPython || fail "import wx"

# The first-start window: shot saved, answered, left.
say "first-start --check: $(ECCE_FIRST_START_ASK_ON_MAC=1 inapp "$RES/ecce/bin/ecce-first-start" --check 2>&1)"
shot=$OUT/first-start.png
ECCE_FIRST_START_ASK_ON_MAC=1 ECCE_FIRST_START_ANSWER=local ECCE_FIRST_START_SHOT=$shot \
  ECCE_APP_RUN=1 limit 90 "$APP/Contents/MacOS/ecce" "$RES/ecce/bin/ecce-first-start" > "$OUT/first-start.log" 2>&1
r=$?
say "first-start window: exit $r, picture $( [ -s "$shot" ] && echo saved || echo MISSING )"
[ "$r" = 0 ] && [ -s "$shot" ] || { fail "first-start window"; tail -5 "$OUT/first-start.log" | tee -a "$SUMMARY"; }

# A Theory Details dialog through the dialog suite's own introspection shim,
# which runs the real script with its real argument list.
export ECCE_HARNESS_SCRIPT=$RES/ecce/scripts/codereg/mopactheory.py
export ECCE_HARNESS_OUT=$OUT/inventory.json
export PYTHONPATH=$RES/ecce/scripts/codereg PYTHONDONTWRITEBYTECODE=1
: > "$OUT/restore.in"
# The dialog reports to a UDP port; the real listener keeps it from failing.
inapp python3 -c '
import socket, sys, time
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); s.bind(("127.0.0.1", 0))
open(sys.argv[1], "w").write(str(s.getsockname()[1])); time.sleep(150)' "$OUT/port" &
lis=$!
for _ in 1 2 3 4 5 6 7 8 9 10; do [ -s "$OUT/port" ] && break; sleep 1; done
( cd "$RES/ecce/scripts/codereg" && limit 120 env ECCE_APP_RUN=1 "$APP/Contents/MacOS/ecce" \
    python3 "$REPO/tests/dialogs/_introspect.py" "$OUT/restore.in" "$(cat "$OUT/port")" NO_GUIValues Writable DebugOff \
    SCF RHF Energy harness 0 C1 10 1 1 5 2 3 ) > "$OUT/theory.log" 2>&1
r=$?
kill $lis 2>/dev/null
n=$(python3 -c 'import json,sys; print(len(json.load(open(sys.argv[1]))["widgets"]))' "$OUT/inventory.json" 2>/dev/null)
say "Theory Details dialog (mopactheory.py): exit $r, widgets: ${n:-none}"
[ "${n:-0}" -gt 0 ] && [ "$r" = 0 ] || { fail "Theory Details dialog"; tail -15 "$OUT/theory.log" | tee -a "$SUMMARY"; }
exit $rc
