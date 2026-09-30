#!/bin/csh
#  ECCE Submit Script
#  Generated <date> with ECCE Version <version>.
# 
#  Machine: goldhost
#  QueueManager: Slurm
#  Queue: debug
#  Code: MOPAC
#  Account: proj1
#  Wall Time: 0:30:00
#  # Nodes: 1
#  # TotalProcessors: 1
#  Run Directory: /qtest/run
#  Input File: mopac.mop
#  Output File: mopac.mopout
#  Memory: 2000
# 
#  Config Files Used:
# <ECCE_HOME>/siteconfig/submit.site
# <TMP>/user/.ECCE/CONFIG.goldhost
#SBATCH --partition=debug
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --time=0:30:00
# The memory value carries whatever unit the machine's .Q file declares in
# memUnits (MB for the ones shipped here), so the suffix is spelled out
# rather than left to sbatch's default. Change it if your .Q uses GB.
# Both of these lines are dropped entirely when the field is unset, so a
# blank memory or account box does not emit "--mem=" and fail the submit.
#SBATCH --mem=2000M
#SBATCH --account=proj1
#SBATCH --output=slurm.out
#SBATCH --error=slurm.err


#  Handle interrupts
onintr canceljob

#  Change to the run directory
cd /qtest/run

#  Remove any left-over ecce files from previous runs
rm -f /qtest/run/.ecce.status core
rm -f /qtest/run/ecce.submit.log
touch /qtest/run/ecce.submit.log

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
set mopac = /opt/codes/mopac
if ($?ECCE_MOPAC) then
  set mopac = $ECCE_MOPAC
endif
echo "Using MOPAC path: $mopac" >> /qtest/run/ecce.submit.log

#  - MOPAC writes mopac.out itself; expose it under the
#  - declared output name so live monitoring can follow it.
if ( "mopac.out" != "mopac.mopout" ) then
  rm -f mopac.mopout
  ln -s mopac.out mopac.mopout
endif
$mopac mopac.mop >>& /qtest/run/ecce.submit.log

#  - Now the run is finished, replace that symlink with a
#  - real copy. The symlink is only needed DURING the run,
#  - so live monitoring has the declared name to follow;
#  - afterwards ECCE uploads the declared output file to the
#  - data server, and a symlink does not survive that -- the
#  - job's Outputs/ collection came back with only
#  - eccejobstorelog in it while every regular file (the
#  - input) and all 12 parsed properties uploaded fine.
if ( -l mopac.mopout ) then
  rm -f mopac.mopout
  cp mopac.out mopac.mopout
endif


#  - determine state...
set exitStatus = $status
echo "mopac exit status = $exitStatus" >> /qtest/run/ecce.submit.log

#  look for suspicious core
if ( -e 'core' ) then
  echo "Unexpected core - setting status to X_UNEXPECTED_CORE" >> /qtest/run/ecce.submit.log
  set exitStatus = $X_UNEXPECTED_CORE
endif

if ( ! -e /qtest/run/mopac.mopout) then
  echo "Output file mopac.mopout does not exist - setting status to failed" >> /qtest/run/ecce.submit.log
  set exitStatus = $X_FILE_EXIST
endif
echo "Final exit status = $exitStatus" >> /qtest/run/ecce.submit.log

echo -n "Completed Job: " >> /qtest/run/ecce.submit.log
date >> /qtest/run/ecce.submit.log

if ( ! -e /qtest/run/.ecce.status ) then 
  echo $exitStatus > /qtest/run/.ecce.status
endif

#  Adding ecce log info to output file for debugging...
if ( ! -e mopac.mopout) touch /qtest/run/mopac.mopout
echo "" >> /qtest/run/mopac.mopout 
echo "-----ECCE Log Information-----" >> /qtest/run/mopac.mopout 
cat /qtest/run/ecce.submit.log >> /qtest/run/mopac.mopout 
rm -f /qtest/run/ecce.submit.log

exit (0)

#  Handle interrupt here...
canceljob:
   echo $X_INTERRUPTED > /qtest/run/.ecce.status
   exit (1)
