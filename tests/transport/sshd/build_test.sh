#!/bin/bash
# Builds and runs testSshTransport in a container that has libssh-dev, against
# the sshd started by run.sh.  Usage: build_test.sh [repeat-count]
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
state=${XDG_RUNTIME_DIR:-/tmp}/ecce-test-sshd
if ! podman image exists localhost/ecce-sshdev; then
  ctx=$(mktemp -d)
  printf 'FROM docker.io/library/debian:trixie\nRUN apt-get update && apt-get install -y g++ libssh-dev make\n' > "$ctx/Containerfile"
  podman build -q -t localhost/ecce-sshdev "$ctx" >/dev/null
  rm -rf "$ctx"
fi
podman run --rm --network host -v "$root:/src:Z" -v "$state:/state:Z,ro" \
  -e N="${1:-1}" localhost/ecce-sshdev bash -c '
  set -e
  g++ -std=c++17 -Wall -Wextra -O1 -I/src/include -o /tmp/testSsh \
    /src/tests/transport/testSshTransport.C /src/src/comm/rcommand/SshTransport.C \
    /src/src/comm/rcommand/Transport.C -lssh
  for i in $(seq $N); do /tmp/testSsh /state/id_ed25519; done'
