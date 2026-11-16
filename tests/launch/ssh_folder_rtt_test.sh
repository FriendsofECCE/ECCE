#!/bin/bash
# Round trips of one click in an SSH-folder tree (#1082): runs
# ssh_folder_rtt_test.py against the test sshd from tests/transport/sshd.
# OpenSSH takes ~/.ssh from the passwd home, which $HOME cannot override, so
# the test runs in a private mount namespace with its own ~/.ssh bound over
# the real one; nothing outside the namespace is touched.  SKIPs (77)
# without podman or unshare.
# Usage: ssh_folder_rtt_test.sh [--build DIR]
set -u
here=$(cd "$(dirname "$0")" && pwd)
# ssh reads ~/.ssh from the passwd home, which differs from $HOME in an
# offload job, so the keys are bound over that one.
PH=$(getent passwd "$(id -un)" | cut -d: -f6)
sshd=$here/../transport/sshd
command -v podman >/dev/null && command -v unshare >/dev/null || { echo "SKIP: no podman or unshare"; exit 77; }
export ECCE_TEST_SSH_NAME=ecce-sshrtt-sshd ECCE_TEST_SSH_PORT=${ECCE_TEST_SSH_PORT:-2236}
state=${XDG_RUNTIME_DIR:-/tmp}/ecce-test-sshd
tmp=$(mktemp -d)
trap 'podman rm -f $ECCE_TEST_SSH_NAME >/dev/null 2>&1; rm -rf "$tmp"' EXIT
"$sshd/run.sh" >/dev/null || { echo "SKIP: cannot start the test sshd"; exit 77; }
mkdir -p "$tmp/ssh" "$tmp/empty"; chmod 700 "$tmp/ssh"
install -m 600 "$state/id_ed25519" "$tmp/ssh/good_key"
ssh-keygen -q -t ed25519 -N '' -f "$tmp/ssh/bad_key"
hostblock() {
  printf "Host $1\n  HostName 127.0.0.1\n${3-  Port $ECCE_TEST_SSH_PORT\n}  IdentityFile $PH/.ssh/$2\n  IdentitiesOnly yes\n  PasswordAuthentication no\n  KbdInteractiveAuthentication no\n  StrictHostKeyChecking no\n  UserKnownHostsFile $PH/.ssh/known_hosts\n  ConnectTimeout 10\n" >> "$tmp/ssh/config"
}
# No Port line: the port comes from the folder's address only.
hostblock sshport good_key ""
unshare --user --mount --map-user=$(id -u) --map-group=$(id -g) --keep-caps sh -c \
  "mkdir -p '$PH/.ssh' && mount --bind '$tmp/ssh' '$PH/.ssh' && mount --bind '$tmp/empty' /etc/ssh/ssh_config.d && exec python3 '$here/ssh_folder_rtt_test.py' \"\$@\"" sh "$@"
