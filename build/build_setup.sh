##############################################################################
# FILENAME:  build_setup.sh
#
# PURPOSE:
#   SH/BASH script for ECCE build environment
#
# DESCRIPTION:
#   This script sets up aliases and environment variables needed for compiling
#   ECCE
#
########
# PLEASE follow these rules
########
#
# 1. when adding to PATH or LD_LIBRARY_PATH always check for the
# directories existence first and make sure it's not already included.
#
# 2. Be careful about setting variables like JAVA_HOME that may be used
# by packages besides ECCE.
#
################################################################################

################################################################################
# If not already set, set $ECCE_HOME to two levels up from the path to this
# script.  The $BASH_SOURCE variable is a clever way to allow sh/bash people
# not to have to set $ECCE_HOME prior to sourcing this script.  I couldn't
# find a reliable way to do this for csh so in that case it must be set first!
################################################################################
if [ -z "$ECCE_HOME" ]; then
  SCRIPT=`readlink -f $BASH_SOURCE`
  SCRIPTPATH=`dirname $SCRIPT`
  export ECCE_HOME=`dirname $SCRIPTPATH`
fi


################################################################################
# sanity check...
################################################################################
if [ ! -d "${ECCE_HOME}" ]; then
  echo "ERROR: no ECCE source code tree at ${ECCE_HOME}"
  echo ""
  exit -1
fi


################################################################################
# Miscellaneous build environment variables
################################################################################
if [ -z "$ECCE_COMPILE_MODE" ]; then
  export ECCE_COMPILE_MODE="-DINSTALL"
  #export ECCE_COMPILE_MODE="-g"
fi

# Default is to build with options to allow creation of .so
if [ -z "$ECCE_SHARED_LIBS" ]; then
  export ECCE_SHARED_LIBS="true"
fi


################################################################################
# This gets you some extra Organizer features like showing all data server files
# and Builder features like type-in command entry
################################################################################
#export ECCE_DEVELOPER="true"


################################################################################
# sysdir stuff - set a variable to keep from hitting the env all the time
################################################################################
export ECCE_SYSDIR=`${ECCE_HOME}/scripts/sysdir`


################################################################################
# DialogBlocks wxWidgets GUI Builder
################################################################################
if [ -d "/msrc/proj/ecce/3rdparty/DialogBlocks_aw3" ]; then
  export DIALOGBLOCKSDIR="/msrc/proj/ecce/3rdparty/DialogBlocks_aw3"
  if [ -z "$LD_LIBRARY_PATH" ]; then
    if [ `echo $LD_LIBRARY_PATH | grep -c ${DIALOGBLOCKSDIR}` -eq 0 ]; then
      export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${DIALOGBLOCKSDIR}"
    fi
  else
    export LD_LIBRARY_PATH="${DIALOGBLOCKSDIR}"
  fi

  if [ `echo $PATH | grep -c ${DIALOGBLOCKSDIR}` -eq 0 ]; then
    export PATH="${DIALOGBLOCKSDIR}:$PATH"
  fi
fi

# Set variable that determines whether to do wxWidgets dialogblocks
# code generation during build
which dialogblocks >& /dev/null
if [ $? -eq 0 ]; then
  export ECCE_HAVE_BLOCKS="true"
else
  unset ECCE_HAVE_BLOCKS
fi


###############################################################################
# Java environment for Apache ActiveMQ JMS
# Don't override a previous setting for JAVA_HOME!
###############################################################################
if [ -z "$JAVA_HOME" ]; then
  if [ -d "$ECCE_HOME/${ECCE_SYSDIR}3rdparty/jre" ]; then
    export JAVA_HOME="$ECCE_HOME/${ECCE_SYSDIR}3rdparty/jre"
  fi
fi

if [ ! -z "$JAVA_HOME" ]; then
  if [ `echo $path | grep -c ${JAVA_HOME}/bin` -eq 0 ]; then
    export PATH="${JAVA_HOME}/bin:$PATH"
  fi
fi

CP=""
for f in ${ECCE_HOME}/java/lib/*.jar
do
   CP="${f}:${CP}"
done

if [ -z "$CLASSPATH" ]; then
  export CLASSPATH="${CP}${ECCE_HOME}/siteconfig:."
else
  export CLASSPATH="${CP}${ECCE_HOME}/siteconfig:.:${CLASSPATH}"
fi


################################################################################
# Environment variables dealing with paths and such
################################################################################
export ECCE_MAKEINC_DIR="${ECCE_HOME}/build/make"

if [ `echo ${PATH} | grep -c "$ECCE_HOME/devtools"` -eq 0 ]; then
  export PATH="$ECCE_HOME/devtools:$PATH"
fi

if [ `echo ${PATH} | grep -c "$ECCE_HOME/build"` -eq 0 ]; then
  export PATH="$ECCE_HOME/build:$PATH"
fi

if [ `echo ${PATH} | grep -c "$ECCE_HOME/scripts"` -eq 0 ]; then
  export PATH="$ECCE_HOME/scripts:$PATH"
fi

################################################################################
# Make sure /sbin is in the path for ldconfig
################################################################################
if [ `echo ${PATH} | grep -c "/sbin"` -eq 0 ]; then
  export PATH="/sbin:$PATH"
fi

################################################################################
# Variables needed to build
################################################################################
if [ -z "$ECCE_SYSDIR" ]; then
  if [ `uname -m | grep -c 64` -ne 0 ]; then
    export ECCE_BUILD_BITS=64
  else
    export ECCE_BUILD_BITS=32
  fi
else
  if [ `echo $ECCE_SYSDIR | grep -c m32` -eq 0 ]; then
    export ECCE_BUILD_BITS=64
  else
    export ECCE_BUILD_BITS=32
  fi
fi

if [ `uname -m | grep -c 64` -ne 0 -a $ECCE_BUILD_BITS -eq 32 ]; then
  export ECCE_BUILD_CROSSCOMPILE=true
else
  unset ECCE_BUILD_CROSSCOMPILE
fi


################################################################################
# Useful cd alii for the ecce directory tree
################################################################################
alias cdtop='cd $ECCE_HOME'
alias cddevtls='cd $ECCE_HOME/devtools'
alias cdbuild='cd $ECCE_HOME/build'
alias cdbin='cd $ECCE_HOME/${ECCE_SYSDIR}bin'
alias cdlib='cd $ECCE_HOME/${ECCE_SYSDIR}lib'
alias cdapps='cd $ECCE_HOME/src/apps'
alias cd3rd='cd $ECCE_HOME/${ECCE_SYSDIR}3rdparty'
alias cdscripts='cd $ECCE_HOME/scripts'
alias cddata='cd $ECCE_HOME/data'
alias cdpars='cd $ECCE_HOME/scripts/parsers'
alias cddsm='cd $ECCE_HOME/src/dsm'
alias cdwxgui='cd $ECCE_HOME/src/wxgui'
alias cdwxviz='cd $ECCE_HOME/src/wxviz'
alias cdcomm='cd $ECCE_HOME/src/comm'
alias cdutil='cd $ECCE_HOME/src/util'
alias cdtdat='cd $ECCE_HOME/src/tdat'
alias cdviz='cd $ECCE_HOME/src/viz'
alias cdinv='cd $ECCE_HOME/src/inv'
alias cdsrc='cd $ECCE_HOME/src'
alias cdinc='cd $ECCE_HOME/include'
alias cdidsm='cd $ECCE_HOME/include/dsm'
alias cdiwxgui='cd $ECCE_HOME/include/wxgui'
alias cdiwxviz='cd $ECCE_HOME/include/wxviz'
alias cdicomm='cd $ECCE_HOME/include/comm'
alias cdiutil='cd $ECCE_HOME/include/util'
alias cditdat='cd $ECCE_HOME/include/tdat'
alias cdiviz='cd $ECCE_HOME/include/viz'
alias cdiinv='cd $ECCE_HOME/include/inv'
alias cdjsrc='cd $ECCE_HOME/java/src/gov/pnnl/emsl/ecce/jms'


################################################################################
# Other useful alii
################################################################################
alias debugon='export ECCE_COMPILE_MODE=-g'
alias debugoff='export ECCE_COMPILE_MODE=-DINSTALL'
alias sharedon='export ECCE_SHARED_LIBS=true'
alias sharedoff='export ECCE_SHARED_LIBS=false'
alias chhome='. $ECCE_HOME/build/shhome $1'

