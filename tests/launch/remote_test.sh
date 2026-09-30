#!/bin/bash
# Launch a real MOPAC job on a REMOTE machine (the test sshd from
# tests/transport/sshd) through the pty ssh path and through libssh (#204,
# #107).  Usage: remote_test.sh [bashuser|cshuser ...]   (default: both)
#
# The client runs in a container (Containerfile.launch) so the test owns the
# ~/.ssh that ssh reads; OpenSSH takes it from the passwd home, which an
# isolated $HOME on the host cannot override.  RUN_ARGS overrides the
# arguments to run_tests.py (default: --transport both --drop --kill).  The build goes to build-ssh in
# the worktree, untracked.  To run beside another run: ECCE_TEST_SSH_PORT,
# ECCE_TEST_SSH_NAME (run.sh), ECCE_TEST_CLIENT_HOME, ECCE_TEST_STATE (a path
# inside the client home, e.g. /tmp/client/st), ECCE_DATASERVER_PORT, ECCE_BROKER_PORT.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
sshd=$root/tests/transport/sshd
state=${XDG_RUNTIME_DIR:-/tmp}/ecce-test-sshd
port=${ECCE_TEST_SSH_PORT:-2222}
users=${*:-bashuser cshuser}

podman image exists localhost/ecce-sshbuild ||
  podman build -q -t localhost/ecce-sshbuild -f "$sshd/Containerfile.build" "$sshd" >/dev/null
podman image exists localhost/ecce-launch ||
  podman build -q -t localhost/ecce-launch -f "$sshd/Containerfile.launch" "$sshd" >/dev/null
(exec 3<>/dev/tcp/127.0.0.1/$port) 2>/dev/null || "$sshd/run.sh" >/dev/null

# ssh takes its home from passwd, hence --passwd-entry.  The client's home persists between runs so the data server's seeded
# document root and basis-set library are built once.
chome=${ECCE_TEST_CLIENT_HOME:-${XDG_CACHE_HOME:-$HOME/.cache}/ecce-remote-launch-client}
mkdir -p "$chome"
podman run --rm --userns=keep-id --network host -e HOME=/tmp/client -e USER=ecce \
  --passwd-entry "ecce:*:$(id -u):$(id -g)::/tmp/client:/bin/bash" \
  -v "$root:/src:Z" -v "$state:/state:Z,ro" -v "$chome:/tmp/client:Z" \
  ${ECCE_TEST_STATE:+-e ECCE_TEST_STATE} ${ECCE_DATASERVER_PORT:+-e ECCE_DATASERVER_PORT} \
  ${ECCE_BROKER_PORT:+-e ECCE_BROKER_PORT} ${ECCE_SSH_KEEPALIVE:+-e ECCE_SSH_KEEPALIVE} \
  -e PORT="$port" -e USERS="$users" -e RUN_ARGS="${RUN_ARGS:---transport both --drop --kill}" localhost/ecce-launch bash -c '
  set -e
  install -d -m 700 ~/.ssh
  install -m 600 /state/id_ed25519 ~/.ssh/ecce_test_key
  ssh-keyscan -p $PORT 127.0.0.1 2>/dev/null > ~/.ssh/known_hosts
  printf "Host sshtest 127.0.0.1\n  HostName 127.0.0.1\n  Port $PORT\n  IdentityFile $HOME/.ssh/ecce_test_key\n  IdentitiesOnly yes\n  PasswordAuthentication no\n  KbdInteractiveAuthentication no\n" > ~/.ssh/config
  cd /src
  ninja -C build-ssh launchjob eccejobstore eccejobmaster ecmd >/dev/null
  rc=0
  for u in $USERS; do
    echo "=== remote user $u"
    python3 tests/launch/run_tests.py --build /src/build-ssh --machine sshtest \
      --remote-user $u $RUN_ARGS || rc=1
  done
  exit $rc'
