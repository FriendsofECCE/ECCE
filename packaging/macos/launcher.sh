#!/bin/bash
# ECCE.app's executable: the same session start as the `ecce` command,
# with ECCE_HOME inside the bundle and nothing taken from Homebrew.
here=$(cd "$(dirname "$0")/.." && pwd)
export ECCE_HOME="$here/Resources/ecce"
# libexec holds the broker; Finder's own PATH has neither it nor the wrappers.
export PATH="$here/Resources/bin:$ECCE_HOME/libexec:/usr/bin:/bin:/usr/sbin:/sbin"
exec /bin/bash "$here/Resources/bin/ecce" "$@"
