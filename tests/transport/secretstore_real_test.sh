#!/bin/bash
# One real round trip through secret-tool and a private gnome-keyring (#777):
# store, lookup, clear. ECCE_SECRET_TOOL names the secret-tool program
# (default: the one on PATH). Usage: secretstore_real_test.sh BUILD_DIR
set -eu
build=${1:?build directory}
exec dbus-run-session -- bash -c '
  export XDG_RUNTIME_DIR=$(mktemp -d); export HOME=$XDG_RUNTIME_DIR
  eval "$(printf "\n" | gnome-keyring-daemon --unlock --components=secrets 2>/dev/null | sed "s/^/export /")"
  '"$build"'/testSecretStore real'
