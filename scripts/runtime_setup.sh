################################################################################
# FILENAME:  runtime_setup.sh
#
#
# USAGE:
#   source from ~/.profile or ~/.bashrc
#
# DESCRIPTION:
#   This sh script sets up the minimal environment needed to bootstrap ECCE.
#   It is modified during an install to insert the value of ECCE_HOME.
#   This script is the sh/bash equivalent of runtime_setup.
#
################################################################################

# set ECCE_HOME -------------------------------------------------------
export ECCE_HOME=##REPLACE-WITH-ECCE_HOME##

# set PATH ------------------------------------------------------------
if [ ${PATH:-none} != none ]; then
  export PATH=${ECCE_HOME}/scripts:${PATH}
else
  export PATH=${ECCE_HOME}/scripts
fi

