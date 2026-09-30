#!/bin/bash
# Build and start the test sshd on localhost:${ECCE_TEST_SSH_PORT:-2222}.
# Prints the key file to use; stop with: podman rm -f $ECCE_TEST_SSH_NAME
set -eu
export ECCE_TEST_SSH_NAME=${ECCE_TEST_SSH_NAME:-ecce-test-sshd}
here=$(cd "$(dirname "$0")" && pwd)
state=${XDG_RUNTIME_DIR:-/tmp}/ecce-test-sshd
port=${ECCE_TEST_SSH_PORT:-2222}
name=${ECCE_TEST_SSH_NAME:-ecce-test-sshd}
mkdir -p "$state"
[ -f "$state/id_ed25519" ] || ssh-keygen -q -t ed25519 -N '' -f "$state/id_ed25519"
ctx=$(mktemp -d); trap 'rm -rf "$ctx"' EXIT
cp "$here/Containerfile" "$ctx/"; cp "$state/id_ed25519.pub" "$ctx/authorized_keys"
podman build -q -t localhost/ecce-test-sshd "$ctx" >/dev/null
podman rm -f $name >/dev/null 2>&1 || true
podman run -d --name $name -p 127.0.0.1:$port:22 localhost/ecce-test-sshd >/dev/null
for i in $(seq 20); do (exec 3<>/dev/tcp/127.0.0.1/$port) 2>/dev/null && break; sleep 0.5; done
echo "$state/id_ed25519"
