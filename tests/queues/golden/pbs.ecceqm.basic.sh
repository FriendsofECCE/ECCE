#!/bin/sh
#  ECCE Submit Script
#  Generated <date> with ECCE Version <version>.
# 
#  Machine: goldhost
#  QueueManager: PBS
#  Queue: debug
#  Code: ECCE-QM
#  Account: proj1
#  Wall Time: 0:30:00
#  # Nodes: 1
#  # TotalProcessors: 1
#  Run Directory: /qtest/run
#  Input File: ecceqm.qmin
#  Output File: ecceqm.qmout
#  Memory: 2000
# 
#  Config Files Used:
# <ECCE_HOME>/siteconfig/submit.site
# <TMP>/user/.ECCE/CONFIG.goldhost
#PBS -N gold
#PBS -q debug
#PBS -l select=1:ncpus=1
#PBS -l walltime=0:30:00
#PBS -l mem=2000mb
#PBS -A proj1
#PBS -j oe
#PBS -o pbs.out
#PBS -S /bin/sh


#  Handle interrupts
canceljob() {
  echo $X_INTERRUPTED > /qtest/run/.ecce.status
  exit 1
}
trap canceljob INT

#  Change to the run directory
cd /qtest/run || exit 1

#  Remove any left-over ecce files from previous runs
rm -f /qtest/run/.ecce.status core
rm -f /qtest/run/ecce.submit.log
touch /qtest/run/ecce.submit.log

#  Exit status and ECCE states...
X_INTERRUPTED=302       # caught interrupt - kill 
X_FILE_EXIST=211        # in/out file does not exist
X_UNEXPECTED_CORE=221   # code exit 0 but core exists
X_NOT_NORMAL=231        # code exit 0 but no normal-termination line

#  Setting environment variables...

#  Keep Open MPI shared memory files with the job, not in /dev/shm
export OMPI_MCA_btl_sm_backing_directory=/qtest/run

printf "Starting Job: " >> /qtest/run/ecce.submit.log
date >> /qtest/run/ecce.submit.log

#  - code/queue manager section...
ecceqm=<ECCE_HOME>/bin/ecce-qm
if [ -n "${ECCE_ECCEQM+set}" ]; then
  ecceqm=$ECCE_ECCEQM
fi
echo "Using ecce-qm path: $ecceqm" >> /qtest/run/ecce.submit.log
OMP_NUM_THREADS=1
export OMP_NUM_THREADS
ECCE_BASIS_DIR="<ECCE_HOME>/data/admin/basissets"
export ECCE_BASIS_DIR
"$ecceqm" ecceqm.qmin > ecceqm.qmout 2>&1


#  - determine state...
exitStatus=$?
echo "ecce-qm exit status = $exitStatus" >> /qtest/run/ecce.submit.log

#  look for suspicious core
if [ -e core ]; then
  echo "Unexpected core - setting status to X_UNEXPECTED_CORE" >> /qtest/run/ecce.submit.log
  exitStatus=$X_UNEXPECTED_CORE
fi

if [ ! -e /qtest/run/ecceqm.qmout ]; then
  echo "Output file ecceqm.qmout does not exist - setting status to failed" >> /qtest/run/ecce.submit.log
  exitStatus=$X_FILE_EXIST
fi

if [ "$exitStatus" = 0 ]; then
  if ! grep -q "end_of_output" /qtest/run/ecceqm.qmout; then
    echo "No 'end_of_output' in ecceqm.qmout - setting status to failed" >> /qtest/run/ecce.submit.log
    exitStatus=$X_NOT_NORMAL
  fi
fi
echo "Final exit status = $exitStatus" >> /qtest/run/ecce.submit.log

printf "Completed Job: " >> /qtest/run/ecce.submit.log
date >> /qtest/run/ecce.submit.log

if [ ! -e /qtest/run/.ecce.status ]; then
  echo $exitStatus > /qtest/run/.ecce.status
fi

#  Adding ecce log info to output file for debugging...
if [ ! -e ecceqm.qmout ]; then touch /qtest/run/ecceqm.qmout; fi
echo "" >> /qtest/run/ecceqm.qmout 
echo "-----ECCE Log Information-----" >> /qtest/run/ecceqm.qmout 
cat /qtest/run/ecce.submit.log >> /qtest/run/ecceqm.qmout 
rm -f /qtest/run/ecce.submit.log

exit 0
