#!/bin/bash
# SftpEDSI (#767) against the test sshd: the data-layer operations, two
# writers on one directory, and a dead server, once with the OpenSSH
# backend and once with libssh.  Builds in the same container as
# rcommand_test.sh (build-ssh, untracked); SKIPs (77) without podman.
# Usage: sftpedsi_test.sh
set -u
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
command -v podman >/dev/null || { echo "SKIP: no podman"; exit 77; }
export ECCE_TEST_SSH_NAME=ecce-sftpedsi-sshd ECCE_TEST_SSH_PORT=${ECCE_TEST_SSH_PORT:-2231}
state=${XDG_RUNTIME_DIR:-/tmp}/ecce-test-sshd
trap 'podman rm -f $ECCE_TEST_SSH_NAME >/dev/null 2>&1' EXIT
"$here/run.sh" >/dev/null || { echo "SKIP: cannot start the test sshd"; exit 77; }
if ! podman image exists localhost/ecce-sshbuild; then
  podman build -q -t localhost/ecce-sshbuild -f "$here/Containerfile.build" "$here" >/dev/null || exit 77
fi
podman run --rm --network host -v "$root:/src:Z" -v "$state:/state:Z,ro" \
  -e PORT="$ECCE_TEST_SSH_PORT" localhost/ecce-sshbuild bash -c '
  set -u
  install -d -m 700 /root/.ssh
  install -m 600 /state/id_ed25519 /root/.ssh/ecce_test_key
  ssh-keyscan -p $PORT 127.0.0.1 2>/dev/null > /root/.ssh/known_hosts
  printf "Host 127.0.0.1\n  Port $PORT\n  IdentityFile /root/.ssh/ecce_test_key\n  IdentitiesOnly yes\nHost deadhost\n  HostName 127.0.0.1\n  Port 1\n  IdentityFile /root/.ssh/ecce_test_key\n  IdentitiesOnly yes\n" > /root/.ssh/config
  cd /src
  [ -f build-ssh/build.ninja ] || cmake -G Ninja -B build-ssh >/dev/null || exit 1
  ninja -C build-ssh testSftpEDSI >/dev/null || { echo "FAIL build"; exit 1; }
  export ECCE_REALUSER=root ECCE_HOME=/src ECCE_REALUSERHOME=/tmp/ecceuser; mkdir -p /tmp/ecceuser
  T=build-ssh/testSftpEDSI
  rc=0
  for backend in openssh libssh; do
    export ECCE_SSH_BACKEND=$backend ECCE_SSH_CONNECT_TIMEOUT=10
    echo "== backend $backend"
    ssh bashuser@127.0.0.1 "rm -rf data && mkdir -p data/shared" || { echo "FAIL ssh"; exit 1; }
    B=sftp://bashuser@127.0.0.1/home/bashuser/data
    for mode in ops twowriters; do
      $T $mode $B > /tmp/$backend.$mode.log 2>&1 || { rc=1; tail -n 12 /tmp/$backend.$mode.log; }
      grep -E "^(FAIL|PASS)" /tmp/$backend.$mode.log | sort | uniq -c | awk "{print \$2}" | sort | uniq -c | sed "s/^/  $mode: /"
      grep FAIL /tmp/$backend.$mode.log
    done
    left=$(ssh bashuser@127.0.0.1 "find data -name \".ecce-meta.lockdir*\" -o -name \".ecce-put*\" -o -name \".ecce-app*\" -o -name \".ecce-new*\" -o -name \".ecce-meta.new*\"")
    [ -z "$left" ] && echo "  PASS no lock or temp file left on the server" || { echo "  FAIL left behind: $left"; rc=1; }
    rm -rf /tmp/copy; mkdir /tmp/copy; scp -q -r bashuser@127.0.0.1:data /tmp/copy/ || rc=1
    $T local /tmp/copy/data | grep -E "^(FAIL|PASS)" | sed "s/^/  local: /"
    $T local /tmp/copy/data | grep -q FAIL && rc=1
    start=$(date +%s)
    $T dead sftp://bashuser@deadhost/home/bashuser/data > /tmp/$backend.dead.log 2>&1 || rc=1
    grep -E "^(FAIL|PASS|message)" /tmp/$backend.dead.log | sed "s/^/  dead: /"
  done
  exit $rc'
exit $?
