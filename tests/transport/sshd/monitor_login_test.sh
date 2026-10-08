#!/bin/bash
# The job monitor rides eccejobstore's one login (#204): jobs launched to a
# machine that ECCE reaches by password, once with keyboard-interactive and
# once with password-only login.  The sshd's own log is the count: each run
# of eccejobstore must log in exactly once (the monitor stream and the copy
# of the output files share that login), and a dropped connection costs one
# new login, in the restarted eccejobstore.  The test's own ssh commands log
# in by key, so every password login is ECCE's.
#   monitor_login_test.sh [JOBS]     (default 3, plus one dropped run)
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
state=${XDG_RUNTIME_DIR:-/tmp}/ecce-test-sshd
kport=${ECCE_MONLOGIN_KBD_PORT:-2243}
pport=${ECCE_MONLOGIN_PW_PORT:-2244}
kname=ecce-monlogin-sshd-kbd pname=ecce-monlogin-sshd-pw
jobs=${1:-3}

podman image exists localhost/ecce-test-sshd || {
  ECCE_TEST_SSH_PORT=$kport ECCE_TEST_SSH_NAME=$kname "$here/run.sh" >/dev/null
  podman rm -f $kname >/dev/null; }
podman image exists localhost/ecce-sshbuild ||
  podman build -q -t localhost/ecce-sshbuild -f "$here/Containerfile.build" "$here" >/dev/null
podman image exists localhost/ecce-launch ||
  podman build -q -t localhost/ecce-launch -f "$here/Containerfile.launch" "$here" >/dev/null

logs=$(mktemp -d "${XDG_RUNTIME_DIR:-/tmp}/ecce-monlogin.XXXXXX")
cleanup() { podman rm -f $kname $pname >/dev/null 2>&1 || true; rm -rf "$logs"; }
trap cleanup EXIT
podman rm -f $kname $pname >/dev/null 2>&1 || true
podman run -d --name $kname -p 127.0.0.1:$kport:22 -v "$logs:/log:Z" \
  localhost/ecce-test-sshd /usr/sbin/sshd -D -E /log/kbd.log >/dev/null
podman run -d --name $pname -p 127.0.0.1:$pport:22 -v "$logs:/log:Z" \
  localhost/ecce-test-sshd /usr/sbin/sshd -D -E /log/pw.log \
  -o KbdInteractiveAuthentication=no >/dev/null
for p in $kport $pport; do
  for i in $(seq 20); do (exec 3<>/dev/tcp/127.0.0.1/$p) 2>/dev/null && break; sleep 0.5; done
done

chome=${ECCE_TEST_CLIENT_HOME:-${XDG_CACHE_HOME:-$HOME/.cache}/ecce-remote-launch-client}
mkdir -p "$chome"
podman run --rm --userns=keep-id --network host -e HOME=/tmp/client -e USER=ecce \
  --passwd-entry "ecce:*:$(id -u):$(id -g)::/tmp/client:/bin/bash" \
  -v "$root:/src:Z" -v "$state:/state:Z,ro" -v "$chome:/tmp/client:Z" \
  -v "$logs:/log:Z,ro" \
  ${ECCE_DATASERVER_PORT:+-e ECCE_DATASERVER_PORT} ${ECCE_BROKER_PORT:+-e ECCE_BROKER_PORT} \
  -e KPORT=$kport -e PPORT=$pport -e JOBS=$jobs localhost/ecce-launch bash -c '
  set -e
  install -d -m 700 ~/.ssh
  install -m 600 /state/id_ed25519 ~/.ssh/ecce_test_key
  { ssh-keyscan -p $KPORT 127.0.0.1; ssh-keyscan -p $PPORT 127.0.0.1; } 2>/dev/null > ~/.ssh/known_hosts
  # kbd and pw: no key, so ECCE logs in by password; kbdkey and pwkey reach
  # the same sshds with the key, for the test itself.
  : > ~/.ssh/config
  for h in kbd:$KPORT pw:$PPORT; do
    printf "Host ${h%%:*}\n  HostName 127.0.0.1\n  Port ${h#*:}\n  IdentitiesOnly yes\n" >> ~/.ssh/config
    printf "Host ${h%%:*}key\n  HostName 127.0.0.1\n  Port ${h#*:}\n  IdentityFile $HOME/.ssh/ecce_test_key\n  IdentitiesOnly yes\n  BatchMode yes\n" >> ~/.ssh/config
  done
  # A key in a default place would let libssh in without a password.
  rm -f ~/.ssh/id_rsa ~/.ssh/id_ecdsa ~/.ssh/id_ed25519
  cd /src
  ninja -j2 -C build-ssh launchjob eccejobstore eccejobmaster ecmd >/dev/null
  rc=0
  for m in kbd pw; do
    echo "=== $m login"
    python3 tests/launch/run_tests.py --build /src/build-ssh --machine $m \
      --helper-host ${m}key --remote-user bashuser --password ecce-test \
      --sshd-log /log/$m.log --jobs $JOBS --drop || rc=1
  done
  exit $rc'
