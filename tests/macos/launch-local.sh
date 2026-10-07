#!/bin/bash
# A local job on the installed ECCE.app with no chemistry code (#133): the
# calculation store comes from tests/launch's launchjob (not shipped, so made
# on Linux), then on the Mac: gensub -> job script (stub MOPAC) -> eccejobmaster
# -> eccejobstore -> eccejobmonitor -> state and properties in the local folder.
# Usage: launch-local.sh complete|cancel   (~/ltest holds the unpacked store,
# stubfiles/ the stub's canned MOPAC output)
set -u
case=$1
T=$HOME/ltest
APP=${ECCE_APP:-$HOME/Applications/ECCE.app}
export ECCE_HOME=$APP/Contents/Resources/ecce
export PATH=$APP/Contents/Resources/bin:$ECCE_HOME/scripts:$ECCE_HOME/scripts/parsers:/usr/bin:/bin:/usr/sbin:/sbin
export ECCE_REALUSERHOME=$T/home ECCE_REALUSER=$(id -un) HOST=localhost
export ECCE_TMPDIR=$T/tmp ECCE_LOCAL_DATA=$T/localdata ECCE_SESSION_ID=lt$case ECCE_NO_MESSAGING=1
unset DISPLAY
if [ "$case" = complete ]; then calc=lt-ch4; delay=3; else calc=lt-kill; delay=300; fi
CALC=$T/localdata/users/local/$calc-project/$calc
URL=file://$CALC
RUN=$T/jobs/$calc
rm -rf "$RUN"; mkdir -p "$RUN" "$ECCE_TMPDIR" "$ECCE_REALUSERHOME/.ECCE"
cat > "$T/stubmopac" <<STUB
#!/bin/sh
echo "stub args: \$*" >> $T/stub.log
sleep $delay
cp $T/stubfiles/mopac.out $T/stubfiles/mopac.arc $T/stubfiles/mopac.mgf .
STUB
chmod +x "$T/stubmopac"
echo "MOPAC: $T/stubmopac" > "$ECCE_REALUSERHOME/.ECCE/CONFIG.localhost"
cp "$CALC/Inputs/mopac.mop" "$RUN/"
cd "$RUN" || exit 1
cat > subParams <<P
 -Q Shell
 -H localhost
 -d localhost
 -c MOPAC
 -n 1
 -N 1
 -r $RUN
 -i mopac.mop
 -o mopac.mopout
 -f $RUN/submit__$calc
P
echo "== gensub"; gensub -v -p subParams; echo "gensub rc=$?"
ls -l submit__$calc || exit 1
cp "$ECCE_HOME/scripts/eccejobmonitor" "$ECCE_HOME/scripts/parsers/mopac.desc" .
cat > eccejobmonitor.conf <<P
host localhost
calcName mopac
ecceVersion 9.0.0-alpha.7
monitoringMode live
jobQ Shell
timePauseFileExist 2
timePauseJobExist 2
timePauseReadLine 2
timePauseReadLinePartial 2
commType stdio
jobOutputFile mopac.mopout
jobOutputFile2 mopac.mopout
parseTypes ALL
mdTask no
mdPrepareTask no
mdBatchOutput no
mdPMFOutput no
logMode rmifok
parseDescriptorFile mopac.desc
P
LOCAL=$ECCE_TMPDIR/jobs/$calc; rm -rf "$LOCAL"; mkdir -p "$LOCAL"
cat > "$LOCAL/eccejobstore.conf" <<P
# eccejobstore configuration file
host: localhost
calcURL: $URL
codeName: MOPAC
remoteDir: $RUN
remoteShell: ssh
userName: $ECCE_REALUSER
importDir: 
mdStoreTrj: none
rxnMetadynamicsTask: false
P
echo "== launch"
nohup sh "./submit__$calc" > submit.out 2>&1 &
JOB=$!
echo "job id $JOB"
( cd "$ECCE_HOME/bin" && nohup ./eccejobmaster -pipe /dev/null -remoteDir "$RUN" -calcURL "$URL" -restartTries 0 -jobId $JOB -configFile "$LOCAL/eccejobstore.conf" > "$RUN/eccejobmaster.log" 2>&1 & )
if [ "$case" = cancel ]; then
  sleep 12
  echo "== cancel: killing the job's processes (killflag set in the store)"
  kill -KILL $JOB; pkill -KILL -f "$T/stubmopac"
fi
for i in $(seq 1 90); do
  sleep 2
  if grep -q -e Complete -e Killed -e Failed -e Interrupt "$CALC/Outputs/eccejobstorelog.ecce_run_log" 2>/dev/null; then break; fi
done
echo "== run log"; grep -o 'name="[^"]*"' "$CALC/Outputs/eccejobstorelog.ecce_run_log" 2>/dev/null
echo "== state in .ecce-meta"; grep -i -e state -e runstate "$CALC/.ecce-meta" | head
echo "== Props"; ls "$CALC/Props" "$CALC/Outputs"
echo "== TE"; cat "$CALC/Props/TE" 2>/dev/null | head -20
echo "== eccejobmaster.log"; tail -15 "$RUN/eccejobmaster.log" 2>&1
echo "== processes left"; ps ax | grep -e eccejob -e stubmopac -e submit__ | grep -v grep
