#!/bin/sh
#  ECCE Submit Script
#  Generated <date> with ECCE Version <version>.
# 
#  Machine: goldhost
#  QueueManager: SGE
#  Queue: normal
#  Code: NWChem
#  Wall Time: 26:0:00
#  # Nodes: 2
#  # TotalProcessors: 8
#  Run Directory: /qtest/run
#  Input File: nwch.nw
#  Output File: nwch.nwout
# 
#  Config Files Used:
# <ECCE_HOME>/siteconfig/submit.site
# <TMP>/user/.ECCE/CONFIG.goldhost
#$ -N gold
#$ -q normal
#$ -pe smp 8
#$ -l h_rt=26:0:00
#$ -S /bin/sh
#$ -cwd
#$ -v SHELL
#$ -j y
#$ -o sge.out


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

#  Removing files specified by PrelimFilesToRemove
rm -f *.q  *.trj  *.prp  *[0-9][0-9][0-9]*.rst *.file30 meta*.dat >/dev/null 2>&1

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
nwchem=/opt/codes/nwchem
if [ -n "${ECCE_NWCHEM+set}" ]; then
  nwchem=$ECCE_NWCHEM
fi

#  - Generating submit file for NWChem built with MPI

#  - Path to mpirun if running an ECCE deployed NWChem
mpibindir=${nwchem%/*}/../../system/bin
if [ -d $mpibindir ]; then
  PATH="$mpibindir:$PATH"; export PATH
fi
mpirun -np 8 "$nwchem" /qtest/run/nwch.nw > /qtest/run/nwch.nwout 2>&1 < /dev/null


#  - determine state...
exitStatus=$?
echo "nwchem exit status = $exitStatus" >> /qtest/run/ecce.submit.log

#  look for suspicious core
if [ -e core ]; then
  echo "Unexpected core - setting status to X_UNEXPECTED_CORE" >> /qtest/run/ecce.submit.log
  exitStatus=$X_UNEXPECTED_CORE
fi

if [ ! -e /qtest/run/nwch.nwout ]; then
  echo "Output file nwch.nwout does not exist - setting status to failed" >> /qtest/run/ecce.submit.log
  exitStatus=$X_FILE_EXIST
fi
echo "Final exit status = $exitStatus" >> /qtest/run/ecce.submit.log

printf "Completed Job: " >> /qtest/run/ecce.submit.log
date >> /qtest/run/ecce.submit.log


#  Removing files specified by FilesToRemove
rm -f core  *.aoints.* >/dev/null 2>&1

if [ ! -e /qtest/run/.ecce.status ]; then
  echo $exitStatus > /qtest/run/.ecce.status
fi

#  Adding ecce log info to output file for debugging...
if [ ! -e nwch.nwout ]; then touch /qtest/run/nwch.nwout; fi
echo "" >> /qtest/run/nwch.nwout 
echo "-----ECCE Log Information-----" >> /qtest/run/nwch.nwout 
cat /qtest/run/ecce.submit.log >> /qtest/run/nwch.nwout 
rm -f /qtest/run/ecce.submit.log

exit 0
