#!/bin/bash
# A bare Mac (no Homebrew, Xcode, python3 or lldb) is what run.sh handles
# now; this name is kept for existing callers.
exec "$(dirname "$0")/run.sh" "$@"
