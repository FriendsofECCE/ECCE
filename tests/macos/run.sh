#!/bin/bash
# First answer to "does ECCE run on macOS?" (#133), for CI: start the
# installed apps on the runner's logged-in desktop, take screenshots, and
# collect logs and crash reports.  Never fails the job; the verdict is in
# the summary.  Usage: run.sh <stage-dir> <out-dir>
#   <stage-dir>/ecce is ECCE_HOME, <stage-dir>/bin holds the wrappers.
STAGE=$(cd "$1" && pwd)
mkdir -p "$2"
OUT=$(cd "$2" && pwd)
mkdir -p "$OUT/shots" "$OUT/logs" "$OUT/crashes"
HERE=$(cd "$(dirname "$0")" && pwd)
SUMMARY=$OUT/summary.txt
: > "$SUMMARY"
say() { echo "$@" | tee -a "$SUMMARY"; }

export ECCE_HOME=$STAGE/ecce
export PATH=$STAGE/bin:$(brew --prefix)/bin:$PATH
export ECCE_REALUSERHOME=$OUT/home
mkdir -p "$ECCE_REALUSERHOME"
export ECCE_SESSION_LIVENESS=lease
export ECCE_DEBUG_GL_VISUAL=1

DIAG=$HOME/Library/Logs/DiagnosticReports
BEFORE=$OUT/.reports-before
ls "$DIAG" 2>/dev/null | sort > "$BEFORE"

{ sw_vers; uname -a; system_profiler SPDisplaysDataType; bash --version | head -1
  ls -l "$STAGE/bin" "$ECCE_HOME/bin"; } > "$OUT/system.txt" 2>&1

swiftc -O "$HERE/windows.swift" -o "$OUT/windows" > "$OUT/logs/swiftc.log" 2>&1 \
  || say "note: swiftc failed, window detection falls back to screenshots only"
windows() { [ -x "$OUT/windows" ] && "$OUT/windows" 2>/dev/null; }

screencapture -x "$OUT/shots/00-desktop.png" 2>>"$OUT/logs/screencapture.log" \
  || say "note: screencapture failed (see logs/screencapture.log)"

# run_app NAME SECONDS CMD...: start, wait for a window (or SECONDS),
# screenshot twice, end it, report how it ended.
run_app() {
  local name=$1 secs=$2 pid i found=0 rc status
  shift 2
  say "== $name: $*"
  "$@" > "$OUT/logs/$name.log" 2>&1 < /dev/null &
  pid=$!
  for i in $(seq 1 "$secs"); do
    kill -0 $pid 2>/dev/null || break
    # A session spawns children, so any window whose owner name starts
    # "ecce" counts, besides the process itself.
    if windows | grep -i -q -e "^[0-9]*	ecce" -e "^$pid	"; then found=1; break; fi
    sleep 1
  done
  if kill -0 $pid 2>/dev/null; then
    sleep 4
    screencapture -x "$OUT/shots/$name-1.png" 2>>"$OUT/logs/screencapture.log"
    windows > "$OUT/logs/$name.windows.txt"
    sleep 8
    screencapture -x "$OUT/shots/$name-2.png" 2>>"$OUT/logs/screencapture.log"
    if kill -0 $pid 2>/dev/null; then
      status="alive, window=$found"
      kill $pid 2>/dev/null; sleep 2; kill -9 $pid 2>/dev/null
    else
      wait $pid; rc=$?
      status="exited rc=$rc (128+n is signal n), window=$found"
    fi
  else
    wait $pid; rc=$?
    status="exited early rc=$rc (128+n is signal n), window=$found"
    screencapture -x "$OUT/shots/$name-1.png" 2>>"$OUT/logs/screencapture.log"
  fi
  say "   $status"
  tail -n 6 "$OUT/logs/$name.log" | sed 's/^/   | /' | tee -a "$SUMMARY"
  sleep 3   # let ReportCrash write
}

# 1. The real thing: `ecce` starts the gateway, broker and data server.
#    macOS has no data-server package (the central server is Linux), so a
#    failure here shows where the client stops.
BASH4=$(brew --prefix)/bin/bash
run_app ecce-session 60 "$BASH4" "$STAGE/bin/ecce"
"$STAGE/bin/ecce-gateway-stop" > "$OUT/logs/gateway-stop.log" 2>&1
pkill -f "$ECCE_HOME/bin/" 2>/dev/null
pkill -f "$OUT/home" 2>/dev/null
sleep 2

# 2. Each app on its own with no broker and no data server, so a window
#    (or a crash at start-up) shows even where the services cannot run.
export ECCE_NO_MESSAGING=1 ECCE_NO_DATASERVER=1
for app in organizer machregister machbrowser builder ptable basistool; do
  if [ -x "$STAGE/bin/ecce-$app" ]; then
    run_app "$app" 30 "$STAGE/bin/ecce-$app"
  else
    say "== $app: no wrapper ecce-$app"
  fi
  pkill -f "$ECCE_HOME/bin/" 2>/dev/null
done

# Register Machines driven by its test hook (no clicks).
cat > "$OUT/machreg.script" <<SCRIPT
wait 2000
shot $OUT/shots/machreg-hook.png
quit
SCRIPT
ECCE_MACHREG_SCRIPT="$OUT/machreg.script" run_app machregister-hook 30 "$STAGE/bin/ecce-machregister"

# Crash reports written during the run.
sleep 5
ls "$DIAG" 2>/dev/null | sort | comm -13 "$BEFORE" - | grep -i -e ecce -e nwchem > "$OUT/.new"
if [ -s "$OUT/.new" ]; then
  say ""
  say "== crash reports"
  while read -r f; do cp "$DIAG/$f" "$OUT/crashes/"; done < "$OUT/.new"
  python3 "$HERE/crashframes.py" $(sed "s|^|$OUT/crashes/|" "$OUT/.new") | tee -a "$SUMMARY"
else
  say ""
  say "== no crash reports for ECCE processes"
fi

cp -R "$ECCE_REALUSERHOME/.ECCE" "$OUT/dot-ECCE" 2>/dev/null
find "$OUT/dot-ECCE" -name 'authcache*' -delete 2>/dev/null
rm -f "$OUT/windows" "$BEFORE" "$OUT/.new"
exit 0
