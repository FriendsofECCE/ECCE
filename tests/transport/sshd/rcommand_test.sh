#!/bin/bash
# Builds RCommand with libssh in a container (Containerfile.build) and runs
# testRCommandSsh against the sshd started by run.sh, in a fresh ~/.ssh with
# the test key, known_hosts and config.  Usage: rcommand_test.sh [repeat-count]
# The build goes to build-ssh in the worktree, untracked.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
state=${XDG_RUNTIME_DIR:-/tmp}/ecce-test-sshd
port=${ECCE_TEST_SSH_PORT:-2222}
if ! podman image exists localhost/ecce-sshbuild; then
  podman build -q -t localhost/ecce-sshbuild -f "$here/Containerfile.build" "$here" >/dev/null
fi
podman run --rm --network host -v "$root:/src:Z" -v "$state:/state:Z,ro" \
  -e N="${1:-1}" -e PORT="$port" localhost/ecce-sshbuild bash -c '
  set -e
  install -d -m 700 /root/.ssh
  install -m 600 /state/id_ed25519 /root/.ssh/ecce_test_key
  ssh-keyscan -p $PORT 127.0.0.1 2>/dev/null > /root/.ssh/known_hosts
  # pwhost has no IdentityFile: it is the password-login case.
  printf "Host 127.0.0.1\n  Port $PORT\n  IdentityFile /root/.ssh/ecce_test_key\n  IdentitiesOnly yes\nHost pwhost\n  HostName 127.0.0.1\n  Port $PORT\n  IdentitiesOnly yes\n" > /root/.ssh/config
  cd /src
  [ -f build-ssh/build.ninja ] || cmake -G Ninja -B build-ssh >/dev/null
  ninja -C build-ssh testRCommandSsh
  for i in $(seq $N); do build-ssh/testRCommandSsh; done'
