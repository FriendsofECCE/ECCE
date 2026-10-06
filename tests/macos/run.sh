#!/bin/bash
# First answer to "does ECCE run on macOS?" (#133), for CI: start the
# installed apps on the runner's logged-in desktop, take screenshots, and
# collect logs and crash reports.  Never fails the job; the verdict is in
# the summary.  Usage: run.sh <stage-dir> <out-dir>
#   <stage-dir>/ecce is ECCE_HOME, <stage-dir>/bin holds the wrappers.
# With ECCE_APP=<ECCE.app> the session is started through the app's own
# launcher, with no Homebrew on PATH (give Contents/Resources as <stage-dir>).
STAGE=$(cd "$1" && pwd)
mkdir -p "$2"
OUT=$(cd "$2" && pwd)
mkdir -p "$OUT/shots" "$OUT/logs" "$OUT/crashes"
HERE=$(cd "$(dirname "$0")" && pwd)
SUMMARY=$OUT/summary.txt
: > "$SUMMARY"
say() { echo "$@" | tee -a "$SUMMARY"; }

export ECCE_HOME=$STAGE/ecce
if [ -n "$ECCE_APP" ]; then
  export PATH=$STAGE/bin:/usr/bin:/bin:/usr/sbin:/sbin
else
  export PATH=$STAGE/bin:$(brew --prefix)/bin:$PATH
fi
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

# No ReportCrash on the runner: rerun a crashed app under lldb for the stack.
backtrace() {
  local bin=$ECCE_HOME/bin/$1
  [ -x "$bin" ] || return
  say "   rerunning under lldb for a backtrace"
  perl -e 'alarm 90; exec @ARGV' lldb -b -o run -k "bt 25" -k quit -- "$bin" > "$OUT/crashes/$1.lldb.txt" 2>&1
  grep -A28 -e "stop reason" "$OUT/crashes/$1.lldb.txt" | head -32 | sed 's/^/   | /' | tee -a "$SUMMARY"
}

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
    # Window owners are the app's own name (organizer, builder, ...), so
    # count any owner that is not part of the desktop itself.
    if windows | grep -v -e "Window Server" -e "Control Center" -e "	Dock	" \
         -e "	Finder	" -e "SystemUIServer" -e "Notification" -e "Spotlight" \
         -e "TextInputMenuAgent" -e "WindowManager" | grep -q .; then found=1; break; fi
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
  case "$rc" in 132|133|134|136|138|139) backtrace "${name%-hook}" ;; esac
  tail -n 6 "$OUT/logs/$name.log" | sed 's/^/   | /' | tee -a "$SUMMARY"
  sleep 3   # let ReportCrash write
}

# 1. The real thing: `ecce` starts the gateway and broker; on macOS the
#    data live in a local folder by default (#216), so no data server.
BASH4=$(brew --prefix)/bin/bash
if [ -n "$ECCE_APP" ]; then
  run_app ecce-session 60 "$ECCE_APP/Contents/MacOS/ecce"
else
  run_app ecce-session 60 "$BASH4" "$STAGE/bin/ecce"
fi
{ echo "ecce-localdata: $("$ECCE_HOME/bin/ecce-localdata")"
  find "$ECCE_REALUSERHOME/.ECCE-local" -maxdepth 4 2>&1 | head -40
  ls -la "$ECCE_REALUSERHOME/.ECCE" 2>&1; } > "$OUT/logs/localdata.txt"
say "   data folder: $(head -1 "$OUT/logs/localdata.txt")"
"$STAGE/bin/ecce-gateway-stop" > "$OUT/logs/gateway-stop.log" 2>&1
pkill -f "$ECCE_HOME/bin/" 2>/dev/null
pkill -f "$OUT/home" 2>/dev/null
sleep 2

# 2. Each app on its own with no broker and no data server, so a window
#    (or a crash at start-up) shows even where the services cannot run.
export ECCE_NO_MESSAGING=1 ECCE_NO_DATASERVER=1
for app in organizer machregister machbrowser builder pertable basistool calced; do
  if [ -x "$STAGE/bin/ecce-$app" ]; then
    run_app "$app" 30 "$STAGE/bin/ecce-$app"
  else
    say "== $app: no wrapper ecce-$app"
  fi
  pkill -f "$ECCE_HOME/bin/" 2>/dev/null
done

# The Builder opens a molecule (a PDB file, no data store needed) and
# renders it through the scene hook, which writes PPMs and exits.
mkdir -p "$OUT/shots/scene"
printf 'style Ball And Stick\nviewall\nsnap builder-glycine\n' > "$OUT/glycine.scene"
ECCE_VIEWER_SCENE="$OUT/glycine.scene" ECCE_VIEWER_SCENE_OUT="$OUT/shots/scene" \
  run_app builder-scene 60 "$STAGE/bin/ecce-builder" "$HERE/../fragreaders/data/glycine.pdb"
pkill -f "$ECCE_HOME/bin/" 2>/dev/null
python3 - "$OUT/shots/scene" <<'PY'
import os, struct, sys, zlib
d = sys.argv[1]
for f in os.listdir(d):
    if not f.endswith(".ppm"): continue
    raw = open(os.path.join(d, f), "rb").read()
    parts, pos = [], 0
    while len(parts) < 4:
        while raw[pos:pos+1].isspace(): pos += 1
        if raw[pos:pos+1] == b"#":
            pos = raw.index(b"\n", pos); continue
        end = pos
        while not raw[end:end+1].isspace(): end += 1
        parts.append(raw[pos:end]); pos = end
    w, h = int(parts[1]), int(parts[2]); pos += 1
    px = raw[pos:pos + 3*w*h]
    rows = b"".join(b"\0" + px[y*3*w:(y+1)*3*w] for y in range(h))
    chunk = lambda t, b: struct.pack(">I", len(b)) + t + b + struct.pack(">I", zlib.crc32(t + b))
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)) \
        + chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b"")
    open(os.path.join(d, f[:-4] + ".png"), "wb").write(png)
    print("scene:", f[:-4] + ".png", w, "x", h)
PY
[ -f "$OUT/shots/scene/FAILED" ] && say "   scene FAILED: $(cat "$OUT/shots/scene/FAILED")"
ls "$OUT/shots/scene"/*.png >/dev/null 2>&1 && say "   scene rendered: $(cd "$OUT/shots/scene" && ls *.png)"

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
