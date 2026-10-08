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
# The Theory/Runtype dialogs are wxPython scripts run as `python3 <script>`;
# a bare Mac has only the Xcode shim, so E2E_PYBIN holds a python3 with wx.
[ -n "$E2E_PYBIN" ] && PATH=$E2E_PYBIN:$PATH
