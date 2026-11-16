#!/bin/bash
# A remote job's run folder is a marked copy of the calculation (#1083): runs
# run_folder_copy_test.py against the test sshd from tests/transport/sshd, in
# a private mount namespace with its own ~/.ssh (as ssh_folder_test.sh).
# SKIPs (77) without podman or unshare.
# Usage: run_folder_copy_test.sh [--build DIR]
set -u
here=$(cd "$(dirname "$0")" && pwd)
# ssh reads ~/.ssh from the passwd home, which differs from $HOME in an
# offload job, so the keys are bound over that one.
PH=$(getent passwd "$(id -un)" | cut -d: -f6)
sshd=$here/../transport/sshd
command -v podman >/dev/null && command -v unshare >/dev/null || { echo "SKIP: no podman or unshare"; exit 77; }
export ECCE_TEST_SSH_NAME=ecce-runcopy-sshd ECCE_TEST_SSH_PORT=${ECCE_TEST_SSH_PORT:-2242}
state=${XDG_RUNTIME_DIR:-/tmp}/ecce-test-sshd
tmp=$(mktemp -d)
trap 'podman rm -f $ECCE_TEST_SSH_NAME >/dev/null 2>&1; rm -rf "$tmp"' EXIT
"$sshd/run.sh" >/dev/null || { echo "SKIP: cannot start the test sshd"; exit 77; }
mkdir -p "$tmp/ssh" "$tmp/empty"; chmod 700 "$tmp/ssh"
install -m 600 "$state/id_ed25519" "$tmp/ssh/good_key"
ssh-keyscan -p "$ECCE_TEST_SSH_PORT" 127.0.0.1 2>/dev/null > "$tmp/ssh/known_hosts"
# Host sshtest: the machine name the job runs on; sshport: the SSH folder.
for h in sshtest sshport; do
  printf "Host $h\n  HostName 127.0.0.1\n  Port $ECCE_TEST_SSH_PORT\n  IdentityFile $PH/.ssh/good_key\n  IdentitiesOnly yes\n  PasswordAuthentication no\n  KbdInteractiveAuthentication no\n  StrictHostKeyChecking no\n  UserKnownHostsFile $PH/.ssh/known_hosts\n  ConnectTimeout 10\n" >> "$tmp/ssh/config"
done
unshare --user --mount --map-user=$(id -u) --map-group=$(id -g) --keep-caps sh -c \
  "mount --bind '$tmp/ssh' '$PH/.ssh' && mount --bind '$tmp/empty' /etc/ssh/ssh_config.d && exec python3 '$here/run_folder_copy_test.py' \"\$@\"" sh "$@"
