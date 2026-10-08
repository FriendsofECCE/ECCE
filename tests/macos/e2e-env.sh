# Environment for tests/macos/e2e.sh: the installed ECCE.app, an isolated
# home and data folder, services started by the app wrappers themselves.
APP=${ECCE_APP:-$HOME/Applications/ECCE.app}
A=$APP/Contents/Resources
export ECCE_HOME=$A/ecce
export PATH=$A/bin:$ECCE_HOME/libexec:/usr/bin:/bin:/usr/sbin:/sbin
export ECCE_REALUSERHOME=$E2E/home
export ECCE_LOCAL_DATA=$E2E/data
export ECCE_SESSION_LIVENESS=lease
export ECCE_NO_REAP=1
export ECCE_TRANSPARENCY_FALLBACK_MS=0
# The Theory/Runtype dialogs and the first-start window run on the Python in
# the app; its bin goes first, as ecce-home.sh does for the wrappers.
PATH=$A/python/bin:$PATH
