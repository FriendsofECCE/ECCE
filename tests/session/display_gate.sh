#!/bin/bash
# DISPLAY is what X needs, never the session's identity (#233). Fails when
# C++ reads DISPLAY outside the allowed files, or when a session script
# (packaging/gateway) uses it at all.
#
#   tests/session/display_gate.sh <source dir>
SRC="${1:?usage: display_gate.sh <source dir>}"
cd "$SRC" || exit 1
if ! git rev-parse --git-dir >/dev/null 2>&1; then
  echo "SKIP: not a git checkout"
  exit 77
fi
# Whether a dialog can be shown at all (RCommand's host-key and askpass
# dialogs).
ALLOWED='^src/comm/rcommand/RCommand\.C:'
fail=0
found="$(git grep -n 'getenv("DISPLAY")' -- src include | grep -Ev "$ALLOWED")"
if [ -n "$found" ]; then
  echo "FAIL  getenv(\"DISPLAY\") outside the allowed files:"
  printf '%s\n' "$found" | sed 's/^/        /'
  fail=1
else
  echo "PASS  no getenv(\"DISPLAY\") in C++ outside the allowed files"
fi
found="$(git grep -nE '\$\{?DISPLAY|DISPLAY=' -- packaging/gateway)"
if [ -n "$found" ]; then
  echo "FAIL  the session scripts use DISPLAY:"
  printf '%s\n' "$found" | sed 's/^/        /'
  fail=1
else
  echo "PASS  the session scripts (packaging/gateway) do not use DISPLAY"
fi
exit $fail
