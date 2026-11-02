#!/bin/bash
# An added SSH folder as an Organizer tree, with launching (#767, b2): runs
# ssh_folder_launch_test.py against the test sshd from tests/transport/sshd.
# OpenSSH takes ~/.ssh from the passwd home, which $HOME cannot override, so
# the test runs in a private mount namespace with its own ~/.ssh bound over
# the real one; nothing outside the namespace is touched.  SKIPs (77)
# without podman or unshare.
# Usage: ssh_folder_test.sh [--build DIR]
set -u
here=$(cd "$(dirname "$0")" && pwd)
sshd=$here/../transport/sshd
command -v podman >/dev/null && command -v unshare >/dev/null || { echo "SKIP: no podman or unshare"; exit 77; }
export ECCE_TEST_SSH_NAME=ecce-sshfolder-sshd ECCE_TEST_SSH_PORT=${ECCE_TEST_SSH_PORT:-2232}
# A second sshd on the next port: two SSH folders on one host that differ
# only in the port must reach two servers (#789).
export ECCE_TEST_SSH_PORT2=$((ECCE_TEST_SSH_PORT + 1))
state=${XDG_RUNTIME_DIR:-/tmp}/ecce-test-sshd
tmp=$(mktemp -d)
trap 'podman rm -f $ECCE_TEST_SSH_NAME ${ECCE_TEST_SSH_NAME}2 >/dev/null 2>&1; rm -rf "$tmp"' EXIT
"$sshd/run.sh" >/dev/null || { echo "SKIP: cannot start the test sshd"; exit 77; }
ECCE_TEST_SSH_NAME=${ECCE_TEST_SSH_NAME}2 ECCE_TEST_SSH_PORT=$ECCE_TEST_SSH_PORT2 "$sshd/run.sh" >/dev/null ||
  { echo "SKIP: cannot start the second test sshd"; exit 77; }
mkdir -p "$tmp/ssh" "$tmp/empty"; chmod 700 "$tmp/ssh"
install -m 600 "$state/id_ed25519" "$tmp/ssh/good_key"
ssh-keygen -q -t ed25519 -N '' -f "$tmp/ssh/bad_key"
hostblock() {
  printf "Host $1\n  HostName 127.0.0.1\n${3-  Port $ECCE_TEST_SSH_PORT\n}  IdentityFile $HOME/.ssh/$2\n  IdentitiesOnly yes\n  PasswordAuthentication no\n  KbdInteractiveAuthentication no\n  StrictHostKeyChecking no\n  UserKnownHostsFile $HOME/.ssh/known_hosts\n  ConnectTimeout 10\n" >> "$tmp/ssh/config"
}
hostblock sshtest good_key
hostblock sshbad bad_key
# No Port line: the port comes from the folder's address only.
hostblock sshport good_key ""
unshare --user --mount --map-user=$(id -u) --map-group=$(id -g) --keep-caps sh -c \
  "mount --bind '$tmp/ssh' \"\$HOME/.ssh\" && mount --bind '$tmp/empty' /etc/ssh/ssh_config.d && exec python3 '$here/ssh_folder_launch_test.py' \"\$@\"" sh "$@"
