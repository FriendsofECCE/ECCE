#!/bin/bash
# The shared-connection case (#204): the client's ssh config for host `cm`
# has ControlMaster/ControlPath/ControlPersist.  A master per account is
# opened with the key, then the key is moved away, so any new connection
# (libssh's, or an ssh without the master) cannot log in and only the master
# gets in.  With ECCE_TRANSPORT=ssh ECCE picks the OpenSSH client by itself.
#   1. testRCommandSsh: the RCommand operations through that connection, the
#      pty ssh path to host `oracle` (an ordinary login) as the oracle.
#   2. testControlMasterLoss: the master dies mid-session.
#   3. testControlMasterLoss --askpass: no master at all, so ECCE opens it
#      through an askpass stub, for a password, for a password and then a
#      keyboard-interactive prompt (cshuser), for an unknown host key, and
#      when the user cancels.
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

# cshuser may log in with the key alone (the masters above) or, for the
# askpass case, with a password and then a keyboard-interactive prompt.
podman exec $name sh -c 'printf "Match User cshuser\n  AuthenticationMethods publickey password,keyboard-interactive\n" > /etc/ssh/sshd_config.d/zz-two-prompts.conf; kill -HUP $(cat /run/sshd.pid 2>/dev/null || pgrep -o sshd)'

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
Host cmpw cmnew
  HostName 127.0.0.1
  Port $PORT
  IdentitiesOnly yes
  PubkeyAuthentication no
  PreferredAuthentications password,keyboard-interactive
  ControlMaster auto
  ControlPath /root/.ssh/cmpw-%C
  ControlPersist 10m
Host cmnew
  UserKnownHostsFile /root/.ssh/known_hosts.new
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
  # The real askpass script of ECCE, with the dialogs stubbed: each logs a line.
  fh=/tmp/fakehome; rm -rf $fh; mkdir -p $fh/bin
  ln -s /src/siteconfig $fh/siteconfig; ln -s /src/data $fh/data
  cat > $fh/bin/passdialog <<STUB
#!/bin/sh
echo "passdialog \$*" >> \$ASKLOG
[ "\$ASK_MODE" = cancel ] && exit 0
echo ecce-test
STUB
  cat > $fh/bin/hostkeydialog <<STUB
#!/bin/sh
echo "hostkeydialog \$*" >> \$ASKLOG
[ "\$ASK_MODE" = cancel ] && exit 1
echo accept
STUB
  chmod +x $fh/bin/*
  export ECCE_ASKPASS=/src/scripts/ecce-askpass ASKLOG=/tmp/ask.log
  ask() {  # mode user host dialogs
    rm -f $ASKLOG
    ECCE_HOME=$fh ASK_MODE=$1 build-ssh/testControlMasterLoss --askpass "$@" || rc=1
    cat $ASKLOG
    ssh -O exit -l $2 $3 2>/dev/null || true
  }
  echo "--- no master: ECCE opens it through askpass (password)"
  ask ok bashuser cmpw 1
  grep -q "^passdialog password 127.0.0.1 bashuser$" $ASKLOG || { echo "FAIL passdialog arguments"; rc=1; }
  echo "--- cancelled: one dialog although ssh asks up to six times"
  ask cancel bashuser cmpw 1
  echo "--- an unknown host key is confirmed through askpass"
  : > /root/.ssh/known_hosts.new
  ask ok bashuser cmnew 2
  grep -q "^hostkeydialog .127.0.0.1.:$PORT SHA256:.* ED25519$" $ASKLOG || { echo "FAIL hostkeydialog arguments"; rc=1; }
  [ -s /root/.ssh/known_hosts.new ] && echo "ok   the key was saved" ||
    { echo "FAIL the key was not saved"; rc=1; }
  echo "--- an unknown host key is refused in the dialog"
  : > /root/.ssh/known_hosts.new
  ask cancel bashuser cmnew 1
  [ ! -s /root/.ssh/known_hosts.new ] && echo "ok   the key was not saved" ||
    { echo "FAIL the key was saved"; rc=1; }
  echo "--- two prompts: password, then keyboard-interactive (cshuser)"
  ask ok cshuser cmpw 2
  exit $rc'
