#!/bin/bash
# Builds ecmd and testBgcommand with libssh in the Containerfile.build image
# and runs RCommand::bgcommand against the sshd from run.sh, pty then ssh.
# Usage: [ECCE_TEST_SSH_PORT=2222] [ECCE_TEST_SSH_STATE=dir] bgcommand_test.sh
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
state=${XDG_RUNTIME_DIR:-/tmp}/ecce-test-sshd
port=${ECCE_TEST_SSH_PORT:-2222}
if ! podman image exists localhost/ecce-sshbuild; then
  podman build -q -t localhost/ecce-sshbuild -f "$here/Containerfile.build" "$here" >/dev/null
fi
podman run --rm --network host -v "$root:/src:Z" -v "$state:/state:Z,ro" \
  -e PORT="$port" -e MODES="${MODES:-}" -e LOG="${LOG:-}" -e SHELLCMD="${SHELLCMD:-}" localhost/ecce-sshbuild bash -c '
  set -e
  install -d -m 700 /root/.ssh
  install -m 600 /state/id_ed25519 /root/.ssh/ecce_test_key
  ssh-keyscan -p $PORT 127.0.0.1 2>/dev/null > /root/.ssh/known_hosts
  printf "Host 127.0.0.1\n  Port $PORT\n  IdentityFile /root/.ssh/ecce_test_key\n  IdentitiesOnly yes\nHost pwhost\n  HostName 127.0.0.1\n  Port $PORT\n  IdentitiesOnly yes\n" > /root/.ssh/config
  # pwhost has a known_hosts line under its own name as well
  sed "s/^\[127.0.0.1\]:$PORT/pwhost/" /root/.ssh/known_hosts >> /root/.ssh/known_hosts
  cd /src
  [ -f build-ssh/build.ninja ] || cmake -G Ninja -B build-ssh >/dev/null
  ninja -C build-ssh ecmd testBgcommand
  eh=/tmp/eh; rm -rf $eh; mkdir -p $eh/bin /root/.ECCE
  ln -s /src/build-ssh/ecmd $eh/bin/ecmd; ln -s /src/siteconfig $eh/siteconfig; ln -s /src/data $eh/data
  t=$(printf "\t")
  for m in sshbg:127.0.0.1 pwhost:pwhost; do
    printf "${m%%:*}${t}${m#*:}${t}test${t}test${t}test${t}1:1${t}ssh${t}na${t}na\n"
  done > /root/.ECCE/MyMachines
  export ECCE_HOME=$eh ECCE_REALUSER=root ECCE_REALUSERHOME=/root
  [ -z "$SHELLCMD" ] || exec bash -c "$SHELLCMD"
  rc=0
  [ -z "$LOG" ] || export ECCE_RCOM_LOGMODE=1
  for mode in ${MODES:-pty ssh}; do build-ssh/testBgcommand $mode || rc=1; done
  exit $rc'
