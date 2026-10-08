#!/bin/bash
# ECCE.app's executable: the same session start as the `ecce` command,
# with ECCE_HOME inside the bundle and nothing taken from Homebrew.
here=$(cd "$(dirname "$0")/.." && pwd)
export ECCE_HOME="$here/Resources/ecce"
# The bundled Python (with wxPython) comes first: the Details dialogs and
# the first-start window run `python3`.  libexec holds the broker; Finder's
# own PATH has neither it nor the wrappers.
export ECCE_PYTHON="$here/Resources/python/bin/python3"
export PATH="$here/Resources/python/bin:$here/Resources/bin:$ECCE_HOME/libexec:/usr/bin:/bin:/usr/sbin:/sbin"
# ECCE_APP_RUN=1 ecce COMMAND...: run COMMAND in the environment ECCE's own
# programs get (tests, diagnostics).
if [ -n "${ECCE_APP_RUN:-}" ] && [ $# -gt 0 ]; then exec "$@"; fi
exec /bin/bash "$here/Resources/bin/ecce" "$@"
