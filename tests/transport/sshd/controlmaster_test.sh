#!/bin/bash
# The shared-connection case (#204): the client's ssh config for host `cm`
# has ControlMaster/ControlPath/ControlPersist.  A master per account is
# opened with the key, then the key is moved away, so any new connection
# (libssh's, or an ssh without the master) cannot log in and only the master
# gets in.  With ECCE_TRANSPORT=ssh ECCE picks the OpenSSH client by itself.
#   1. testRCommandSsh: the RCommand operations through that connection, the
#      pty ssh path to host `oracle` (an ordinary login) as the oracle.
#   2. testControlMasterLoss: the master dies mid-session.
# Own sshd container and port (ECCE_CM_NAME, ECCE_CM_PORT); builds in
# build-ssh in the worktree, untracked.  Usage: controlmaster_test.sh
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
state=${XDG_RUNTIME_DIR:-/tmp}/ecce-cm-sshd
port=${ECCE_CM_PORT:-2250}
name=${ECCE_CM_NAME:-ecce-cm-sshd}
mkdir -p "$state"
[ -f "$state/id_ed25519" ] || ssh-keygen -q -t ed25519 -N '' -f "$state/id_ed25519"

ctx=$(mktemp -d)
cp "$here/Containerfile" "$ctx/"; cp "$state/id_ed25519.pub" "$ctx/authorized_keys"
podman build -q -t localhost/ecce-cm-sshd "$ctx" >/dev/null
rm -rf "$ctx"
podman image exists localhost/ecce-sshbuild ||
  podman build -q -t localhost/ecce-sshbuild -f "$here/Containerfile.build" "$here" >/dev/null
podman rm -f $name >/dev/null 2>&1 || true
trap 'podman rm -f $name >/dev/null 2>&1 || true' EXIT
podman run -d --name $name -p 127.0.0.1:$port:22 localhost/ecce-cm-sshd >/dev/null
for i in $(seq 40); do (exec 3<>/dev/tcp/127.0.0.1/$port) 2>/dev/null && break; sleep 0.5; done

podman run --rm --network host -v "$root:/src:Z" -v "$state:/state:Z,ro" \
  -e PORT="$port" localhost/ecce-sshbuild bash -c '
  set -e
  install -d -m 700 /root/.ssh
  install -m 600 /state/id_ed25519 /root/.ssh/ecce_test_key
  install -m 600 /state/id_ed25519 /root/.ssh/ecce_oracle_key
  ssh-keyscan -p $PORT 127.0.0.1 2>/dev/null > /root/.ssh/known_hosts
  cat > /root/.ssh/config <<CFG
Host oracle
  HostName 127.0.0.1
  Port $PORT
  IdentityFile /root/.ssh/ecce_oracle_key
  IdentitiesOnly yes
Host cm
  HostName 127.0.0.1
  Port $PORT
  IdentityFile /root/.ssh/ecce_test_key
  IdentitiesOnly yes
  ControlMaster auto
  ControlPath /root/.ssh/cm-%C
  ControlPersist 10m
CFG
  export ECCE_HOME=/src
  cd /src
  [ -f build-ssh/build.ninja ] || cmake -G Ninja -B build-ssh >/dev/null
  ninja -C build-ssh testRCommandSsh testControlMasterLoss testOpenSshTransport >/dev/null

  mastersup() {
    for u in cshuser bashuser; do
      ssh -fN -o BatchMode=yes -l $u cm
      ssh -O check -l $u cm 2>&1
    done
  }
  mastersup
  mv /root/.ssh/ecce_test_key /root/.ssh/ecce_test_key.away
  for u in cshuser bashuser; do
    if ssh -o ControlPath=none -o BatchMode=yes -l $u cm true 2>/dev/null; then
      echo "FAIL: a new connection still logs in without the key"; exit 1
    fi
    ssh -o BatchMode=yes -l $u cm true
  done
  echo "--- only the shared connection gets in; testRCommandSsh"
  rc=0
  ECCE_TEST_CONTROLMASTER=1 ECCE_TEST_EXPECT_BACKEND=openssh build-ssh/testRCommandSsh || rc=1
  echo "--- ECCE_SSH_BACKEND=libssh cannot get in (no key): the message says why"
  ECCE_SSH_BACKEND=libssh ECCE_TRANSPORT=ssh ECCE_REALUSER=root ECCE_REALUSERHOME=$HOME \
    ECCE_AUTHCACHE_NO_BROADCAST=1 build-ssh/testControlMasterLoss --libssh-refused bashuser || rc=1
  echo "--- the shared connection dies mid-session"
  KEY=/root/.ssh/ecce_test_key KEYAWAY=/root/.ssh/ecce_test_key.away \
    build-ssh/testControlMasterLoss bashuser || rc=1
  exit $rc'
