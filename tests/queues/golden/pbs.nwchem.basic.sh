#!/bin/csh
#  ECCE Submit Script
#  Generated <date> with ECCE Version <version>.
# 
#  Machine: goldhost
#  QueueManager: PBS
#  Queue: debug
#  Code: NWChem
#  Account: proj1
#  Wall Time: 0:30:00
#  # Nodes: 1
#  # TotalProcessors: 1
#  Run Directory: /qtest/run
#  Input File: nwch.nw
#  Output File: nwch.nwout
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
#PBS -S /bin/csh


#  Handle interrupts
onintr canceljob

#  Change to the run directory
cd /qtest/run

#  Remove any left-over ecce files from previous runs
rm -f /qtest/run/.ecce.status core
rm -f /qtest/run/ecce.submit.log
touch /qtest/run/ecce.submit.log

#  Removing files specified by PrelimFilesToRemove
rm -f *.q  *.trj  *.prp  *[0-9][0-9][0-9]*.rst *.file30 meta*.dat >& /dev/null

#  Exit status and ECCE states...
set X_INTERRUPTED     = 302;    # caught interrupt - kill 
set X_FILE_EXIST      = 211;    # in/out file does not exist
set X_UNEXPECTED_CORE = 221;    # code exit 0 but core exists
set X_NOT_NORMAL      = 231;    # code exit 0 but no normal-termination line

#  Setting environment variables...

#  Keep Open MPI shared memory files with the job, not in /dev/shm
setenv OMPI_MCA_btl_sm_backing_directory /qtest/run

echo -n "Starting Job: " >> /qtest/run/ecce.submit.log
date >> /qtest/run/ecce.submit.log

#  - code/queue manager section...
set nwchem = /opt/codes/nwchem
if ($?ECCE_NWCHEM) then
  set nwchem = $ECCE_NWCHEM
endif
$nwchem nwch.nw >&! nwch.nwout


#  - determine state...
set exitStatus = $status
echo "nwchem exit status = $exitStatus" >> /qtest/run/ecce.submit.log

#  look for suspicious core
if ( -e 'core' ) then
  echo "Unexpected core - setting status to X_UNEXPECTED_CORE" >> /qtest/run/ecce.submit.log
  set exitStatus = $X_UNEXPECTED_CORE
endif

if ( ! -e /qtest/run/nwch.nwout) then
  echo "Output file nwch.nwout does not exist - setting status to failed" >> /qtest/run/ecce.submit.log
  set exitStatus = $X_FILE_EXIST
endif
echo "Final exit status = $exitStatus" >> /qtest/run/ecce.submit.log

echo -n "Completed Job: " >> /qtest/run/ecce.submit.log
date >> /qtest/run/ecce.submit.log


#  Removing files specified by FilesToRemove
rm -f core  *.aoints.* >& /dev/null

if ( ! -e /qtest/run/.ecce.status ) then 
  echo $exitStatus > /qtest/run/.ecce.status
endif

#  Adding ecce log info to output file for debugging...
if ( ! -e nwch.nwout) touch /qtest/run/nwch.nwout
echo "" >> /qtest/run/nwch.nwout 
echo "-----ECCE Log Information-----" >> /qtest/run/nwch.nwout 
cat /qtest/run/ecce.submit.log >> /qtest/run/nwch.nwout 
rm -f /qtest/run/ecce.submit.log

exit (0)

#  Handle interrupt here...
canceljob:
   echo $X_INTERRUPTED > /qtest/run/.ecce.status
   exit (1)
