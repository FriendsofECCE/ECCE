#!/bin/bash
# ECCE's own ssh master (#487) against a real sshd that takes a password
# only (no key) and allows 3 sessions per connection (MaxSessions 3). The
# session directory is made by ecce-session-lib.sh and swept by
# ecce-gateway-reap, as in a real session. Every password dialog is one
# line in ASKLOG; the sshd's own log is the second count.
#   1. 5 monitor streams at once: exactly 2 logins (master + one overflow)
#   2. the master killed, 4 openers at once: exactly 1 new login (the flock)
#   3. a known password: 0 dialogs, 1 login
#   4. after ecce-gateway-reap: no socket, no directory, no master process
# Own sshd container and port (ECCE_OWNED_PORT); builds in build-ssh in the
# worktree, untracked. Usage: owned_master_test.sh
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
state=${XDG_RUNTIME_DIR:-/tmp}/ecce-test-sshd
port=${ECCE_OWNED_PORT:-2251}
name=ecce-owned-sshd

podman image exists localhost/ecce-test-sshd || {
  ECCE_TEST_SSH_PORT=$port ECCE_TEST_SSH_NAME=$name "$here/run.sh" >/dev/null
  podman rm -f $name >/dev/null; }
podman image exists localhost/ecce-sshbuild ||
  podman build -q -t localhost/ecce-sshbuild -f "$here/Containerfile.build" "$here" >/dev/null

logs=$(mktemp -d "${XDG_RUNTIME_DIR:-/tmp}/ecce-owned.XXXXXX")
chmod 777 "$logs"
cleanup() { podman rm -f $name >/dev/null 2>&1 || true; rm -rf "$logs"; }
trap cleanup EXIT
podman rm -f $name >/dev/null 2>&1 || true
podman run -d --name $name -p 127.0.0.1:$port:22 -v "$logs:/log:Z" \
  localhost/ecce-test-sshd /usr/sbin/sshd -D -E /log/sshd.log \
  -o KbdInteractiveAuthentication=no -o MaxSessions=3 >/dev/null
for i in $(seq 40); do (exec 3<>/dev/tcp/127.0.0.1/$port) 2>/dev/null && break; sleep 0.5; done

podman run --rm --network host -v "$root:/src:Z" -v "$logs:/log:Z,ro" \
  -e PORT=$port localhost/ecce-sshbuild bash -c '
  set -e
  export HOME=/tmp/ownhome ECCE_REALUSERHOME=/tmp/ownhome
  rm -rf $HOME; install -d -m 700 $HOME/.ssh $HOME/.ECCE
  ssh-keyscan -p $PORT 127.0.0.1 2>/dev/null > $HOME/.ssh/known_hosts
  cat > $HOME/.ssh/config <<CFG
Host own
  HostName 127.0.0.1
  Port $PORT
  IdentitiesOnly yes
  PubkeyAuthentication no
  UserKnownHostsFile /tmp/ownhome/.ssh/known_hosts
  PreferredAuthentications password
CFG
  export ECCE_TEST_SSH_CONFIG=$HOME/.ssh/config
  cd /src
  cmake -G Ninja -B build-ssh >/dev/null 2>&1
  ninja -C build-ssh testOwnedMaster >/dev/null

  fh=/tmp/fakehome; rm -rf $fh; mkdir -p $fh/bin
  cat > $fh/bin/passdialog <<STUB
#!/bin/sh
echo "passdialog \$*" >> \$ASKLOG
echo ecce-test
STUB
  chmod +x $fh/bin/passdialog
  export ECCE_HOME=$fh ECCE_ASKPASS=/src/scripts/ecce-askpass ASKLOG=/tmp/ask.log
  . /src/packaging/gateway/ecce-session-lib.sh
  STATEDIR=$HOME/.ECCE

  logins() { grep -c "Accepted password for bashuser" /log/sshd.log || true; }
  rc=0
  scenario() {  # name expected-logins driver-args...
    local n=$1 want=$2; shift 2
    : > $ASKLOG
    export XDG_RUNTIME_DIR=/tmp/xdg-$n
    rm -rf $XDG_RUNTIME_DIR; install -d -m 700 $XDG_RUNTIME_DIR
    ECCE_SESSION_ID=$(ecce_new_session_id); export ECCE_SESSION_ID
    ecce_ssh_control_setup
    echo "=== $n (control dir $ECCE_SSH_CONTROL_DIR)"
    [ -d "$ECCE_SSH_CONTROL_DIR" ] || { echo "FAIL no control directory made"; rc=1; return; }
    [ "$(stat -c %a "$ECCE_SSH_CONTROL_DIR")" = 700 ] || { echo "FAIL mode of the control directory"; rc=1; }
    before=$(logins)
    build-ssh/testOwnedMaster "$@" || rc=1
    got=$(( $(logins) - before ))
    if [ "$got" = "$want" ]; then echo "ok   sshd saw $got password logins"
    else echo "FAIL sshd saw $got password logins, expected $want"; rc=1; fi
    socks=$(find "$ECCE_SSH_CONTROL_DIR" -type s | wc -l)
    echo "     sockets left by the run: $socks"
    [ "$socks" -ge 1 ] || { echo "FAIL no master socket"; rc=1; }
    # the session is over: no program of it lives, so the reaper sweeps it
    touch -d "10 minutes ago" "$ECCE_SSH_CONTROL_DIR/.born"
    d=$ECCE_SSH_CONTROL_DIR
    unset ECCE_SSH_CONTROL_DIR
    bash /src/packaging/gateway/ecce-gateway-reap || true
    if [ -e "$d" ]; then echo "FAIL the reaper left $d"; rc=1; else echo "ok   reaper removed the directory"; fi
    sleep 0.5
    left=0
    for f in /proc/[0-9]*/cmdline; do
      tr "\\0" " " < $f 2>/dev/null | grep -q "^ssh.*$d" && left=$((left + 1))
    done
    if [ "$left" = 0 ]; then echo "ok   no master process left"
    else echo "FAIL $left processes still name $d"; rc=1; fi
  }
  scenario streams 2 streams 5
  scenario kill 2 kill 4
  scenario prefill 1 prefill
  exit $rc'
