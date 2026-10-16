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
# The Intel build on Apple silicon without Rosetta fails in every program
# with "Bad CPU type" and no window; say which download is needed instead.
if ! "$ECCE_PYTHON" -c '' 2>/dev/null; then
  built=$(file -b "$ECCE_PYTHON" 2>/dev/null)
  case "$(uname -m):$built" in
    arm64:*x86_64*)
      msg="This copy of ECCE is the Intel (x86_64) build, and this Mac has Apple silicon. Install the arm64 .dmg from the ECCE releases page instead, or install Rosetta with: softwareupdate --install-rosetta" ;;
    x86_64:*arm64*)
      msg="This copy of ECCE is the Apple silicon (arm64) build, and this Mac has an Intel processor. Install the x86_64 .dmg from the ECCE releases page instead." ;;
    *)
      msg="ECCE's bundled programs do not run on this Mac ($(uname -m), macOS $(sw_vers -productVersion 2>/dev/null)). Please report this at https://github.com/FriendsofECCE/ECCE/issues" ;;
  esac
  echo "ECCE: $msg" >&2
  osascript -e 'on run argv' -e 'display alert "ECCE cannot start" message (item 1 of argv) as critical' -e 'end run' "$msg" >/dev/null 2>&1
  exit 1
fi
exec /bin/bash "$here/Resources/bin/ecce" "$@"
