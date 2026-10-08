#!/bin/bash
# Water single points end to end on a Mac with the installed ECCE.app and
# real chemistry codes (#133), through the apps' own test hooks (no
# clicks): Organizer makes the calculation, the Builder draws the molecule,
# Register Machines registers the codes, the Calculation Editor sets up,
# the Launcher runs it, the job monitor stores the results.  The same input
# is then run directly with the code and the energies compared.
#
# Usage: e2e.sh [case ...]   cases: mopac nwchem orca ecceqm (default: all)
# Needs ~/mopac, ~/orca/orca_6_1_1_macosx_intel_openmpi411 and ~/miniforge3
# (conda-forge nwchem, wxpython) as in docs "Running codes on macOS".
# Everything lands in $E2E (default ~/e2e); no python3 on this Mac is needed
# except the wxPython one the Theory dialogs use (E2E_PYBIN/python3).
E2E=${E2E:-$HOME/e2e}
export E2E
HERE=$(cd "$(dirname "$0")" && pwd)
[ -f "$E2E/env.sh" ] || { mkdir -p "$E2E"; cp "$HERE/e2e-env.sh" "$E2E/env.sh"; }
export E2E_PYBIN=${E2E_PYBIN:-$E2E/pybin}
if [ ! -x "$E2E_PYBIN/python3" ]; then
  mkdir -p "$E2E_PYBIN"; ln -sf "$HOME/miniforge3/bin/python3" "$E2E_PYBIN/python3"
fi
. "$E2E/env.sh"
MOPAC=$HOME/mopac/bin/mopac
ORCADIR=$HOME/orca/orca_6_1_1_macosx_intel_openmpi411
NWCHEM=$HOME/miniforge3/bin/nwchem
NWLIB=$HOME/miniforge3/share/nwchem/libraries/
CASES=${*:-mopac nwchem orca ecceqm}
OUT=$E2E/out; mkdir -p "$OUT/shots" "$E2E/direct" "$E2E/runs"
SUMMARY=$OUT/summary.txt; : > "$SUMMARY"
say() { echo "$@" | tee -a "$SUMMARY"; }
limit() { local s=$1; shift; perl -e 'alarm shift; exec @ARGV' "$s" "$@"; }

pkill -f "$ECCE_HOME/bin/" 2>/dev/null
rm -rf "$ECCE_LOCAL_DATA" "$E2E/runs"; mkdir -p "$ECCE_LOCAL_DATA/users/local" "$ECCE_REALUSERHOME"
rm -f "$ECCE_REALUSERHOME/.ECCE/CONFIG.localhost"

# --- Organizer, kept up and fed through ECCE_TEST_ORGANIZER
: > "$E2E/org.cmds"
ECCE_TEST_ORGANIZER=$E2E/org.cmds ecce-organizer > "$E2E/org.log" 2>&1 &
ORGPID=$!
org() {   # org COMMAND... -> the Organizer's answer
  echo "$*" >> "$E2E/org.cmds"
  local i
  for i in $(seq 1 120); do
    local a; a=$(grep -F "ECCE_TEST_ORGANIZER: $*: " "$E2E/org.log" | tail -1)
    [ -n "$a" ] && { echo "${a#*"$*: "}"; return; }
    sleep 1
  done
  echo "no answer"
}
for i in $(seq 1 60); do grep -q "starting mosquitto\|broker already" "$E2E/org.log" && break; sleep 1; done
sleep 25
say "organizer: $(org newproject e2e)"

# --- Register Machines: the codes, as a user fills the Codes tab
cat > "$E2E/login.sh" <<L
PATH=$HOME/mopac/bin:$ORCADIR:$HOME/miniforge3/bin:\$PATH; export PATH
L
cat > "$E2E/m.script" <<M
wait 3000
select localhost
tab connection
set sourcefile $E2E/login.sh
tab codes
code MOPAC
click code:find
code NWChem
click code:find
code ORCA
click code:find
dump
save
wait 2000
quit
M
ECCE_MACHREG_SCRIPT=$E2E/m.script limit 120 ecce-machregister > "$OUT/machreg.log" 2>&1
grep -e 'MACHREG\] alert' -e 'script done' -e FAIL "$OUT/machreg.log" | tee -a "$SUMMARY"
if ! grep -q '^MOPAC:' "$ECCE_REALUSERHOME/.ECCE/CONFIG.localhost" 2>/dev/null; then
  say "Find did not fill the paths; entering them by hand"
  cat > "$E2E/m2.script" <<M
wait 3000
select localhost
tab codes
code MOPAC
set code:mopac $MOPAC
code NWChem
set code:nwchem $NWCHEM
code ORCA
set code:orca $ORCADIR/orca
save
wait 2000
quit
M
  ECCE_MACHREG_SCRIPT=$E2E/m2.script limit 120 ecce-machregister > "$OUT/machreg2.log" 2>&1
fi
# NWChem finds its basis library through the environment conda activation sets.
grep -q NWCHEM_BASIS_LIBRARY "$ECCE_REALUSERHOME/.ECCE/CONFIG.localhost" || cat > "$E2E/m3.script" <<M
wait 3000
select localhost
tab codes
code NWChem
set blk:cenv "NWCHEM_BASIS_LIBRARY $NWLIB"
save
wait 2000
quit
M
[ -f "$E2E/m3.script" ] && ECCE_MACHREG_SCRIPT=$E2E/m3.script limit 120 ecce-machregister > "$OUT/machreg3.log" 2>&1
say "--- CONFIG.localhost"; grep -v '^$' "$ECCE_REALUSERHOME/.ECCE/CONFIG.localhost" | grep -v '^[#}]\|{$' | tee -a "$SUMMARY"

# --- one calculation per code
runcase() {
  local c=$1 type theory basis name url dir
  case $c in
    mopac)  type=MOPAC;   theory=;    basis= ;;
    nwchem) type=NWChem;  theory=RHF; basis=6-31G* ;;
    orca)   type=ORCA;    theory=RHF; basis=def2-svp ;;
    ecceqm) type=ECCE-QM; theory=;    basis=6-31G* ;;
  esac
  name=water-$c
  say; say "===== $c ($type)"
  say "newcalc: $(org newcalc e2e $name $type)"
  dir=$ECCE_LOCAL_DATA/users/local/e2e/$name
  url=file://$dir
  printf 'wait 4000\nadd O Bent 0 0 0\nwait 1500\ncmd addh\nwait 1500\ninfo\nexpect atoms 3\nsave\nwait 4000\nquit\n' > "$E2E/b.script"
  ECCE_BUILDER_SCRIPT=$E2E/b.script limit 150 ecce-builder -context "$url" > "$OUT/$c.builder.log" 2>&1
  say "builder: $(grep -c ': ok$' "$OUT/$c.builder.log") steps ok, $(grep BUILDER: "$OUT/$c.builder.log" | head -1)"
  { echo 'wait 3000'; echo ready
    [ -n "$theory" ] && echo "theory $theory"
    [ -n "$basis" ] && echo "basis $basis"
    echo 'wait 1500'; echo info; echo 'button save'; echo 'wait 5000'; echo info; echo quit; } > "$E2E/c.script"
  ECCE_CALCED_SCRIPT=$E2E/c.script limit 150 ecce-calced -context "$url" > "$OUT/$c.calced.log" 2>&1
  grep -e 'FAIL' -e 'CALCED: theory' "$OUT/$c.calced.log" | tail -3 | tee -a "$SUMMARY"
  ls "$dir/Inputs" | tr '\n' ' ' | tee -a "$SUMMARY"; echo | tee -a "$SUMMARY"
  mkdir -p "$E2E/runs/$name"
  printf 'wait 3000\nmachine localhost\nrundir %s\nwait 1500\nlaunch\nwait 5000\nquit\n' "$E2E/runs/$name" > "$E2E/l.script"
  ECCE_LAUNCHER_SCRIPT=$E2E/l.script limit 150 ecce-launcher -context "$url" > "$OUT/$c.launcher.log" 2>&1
  grep -e 'FAIL' -e 'script done' "$OUT/$c.launcher.log" | tee -a "$SUMMARY"
  local state= log=$dir/Outputs/eccejobstorelog.ecce_run_log
  for i in $(seq 1 150); do
    [ -f "$log" ] && state=$(grep -o 'Calculation State Change"[^>]*>[A-Za-z ]*<' "$log" | tail -1 | sed 's/.*>\(.*\)</\1/' | tr -d ' ')
    case $state in Complete|Failed|Killed|Unsuccessful) break;; esac
    sleep 3
  done
  say "final state: $state; Organizer says: $(org state "$url")"
  say "Props: $(ls "$dir/Props" 2>/dev/null | tr '\n' ' ')"
  say "TE: $(grep -o '>[^<]*</value' "$dir/Props/TE" 2>/dev/null)"
  say "summary: $(org summary "$url")"
  # the same input, run directly
  local d=$E2E/direct/$c; rm -rf "$d"; mkdir -p "$d"; cp "$dir"/Inputs/* "$d"/ 2>/dev/null
  ( cd "$d"
    case $c in
      mopac) "$MOPAC" mopac.mop >/dev/null 2>&1; grep -m1 'FINAL HEAT' mopac.out ;;
      nwchem) NWCHEM_BASIS_LIBRARY=$NWLIB "$NWCHEM" nwchem.nw > nwchem.out 2>&1; grep 'Total SCF energy' nwchem.out | tail -1 ;;
      orca) PATH=$ORCADIR:$PATH "$ORCADIR/orca" orca.inp > orca.out 2>&1; grep 'FINAL SINGLE' orca.out; tail -2 orca.out ;;
      ecceqm) f=$(ls *.qmin 2>/dev/null | head -1); say "direct input: $f"; "$ECCE_HOME/bin/ecce-qm" "$f" > ecceqm.out 2>&1; tail -3 ecceqm.out ;;
    esac ) 2>&1 | sed 's/^/direct: /' | tee -a "$SUMMARY"
  # the Builder shows the MOs
  local metrics=$OUT/$c.panes.txt; rm -f "$metrics"
  ECCE_OPEN_PANEL=MOs ECCE_PANEL_METRICS=$metrics \
    limit 60 ecce-builder -context "$url" > "$OUT/$c.mos.log" 2>&1 &
  local bp=$!
  sleep 35; screencapture -x "$OUT/shots/$c-mos.png" 2>/dev/null
  pkill -f "$ECCE_HOME/bin/builder" 2>/dev/null; wait $bp 2>/dev/null
  say "Builder MOs pane: $(grep '"MOs"' "$metrics" 2>/dev/null || echo 'not listed')"
}
for c in $CASES; do runcase "$c"; done
kill $ORGPID 2>/dev/null; pkill -f "$ECCE_HOME/bin/" 2>/dev/null
say; say "done"
