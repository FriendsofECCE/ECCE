#!/bin/bash
# The owned-master login (#487) from an INSTALLED tree: cmake --install into
# a staging directory (DESTDIR), then a real ssh -f master against a password-only sshd
# (keyboard-interactive, as on most clusters), through the installed
# ecce-askpass and the installed passdialog (ECCE_TEST_AUTH_ANSWER answers
# for it, so its real exit status is what askpass sees).  A stub passdialog
# hid that the real one exited 255 after OK, which askpass took as cancel.
#   1. the right password: one dialog, a live master, a command over it
#   2. cancel: no master, exactly one dialog
# Needs a configured build directory (default build-cmake) and Xvfb.
# Usage: installed_askpass_test.sh [build-dir]
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
build=${1:-$root/build-cmake}
port=${ECCE_ASKPASS_PORT:-2253}
name=ecce-askpass-sshd
work=$(mktemp -d "${TMPDIR:-/tmp}/ecce-askpass.XXXXXX")
xpid=
cleanup() {
  [ -z "$xpid" ] || kill "$xpid" 2>/dev/null || true
  [ ! -S "$work/cd/m" ] || ssh -F "$work/home/.ssh/config" -o ControlPath="$work/cd/m" -O exit t 2>/dev/null || true
  podman rm -f $name >/dev/null 2>&1 || true
  rm -rf "$work"
}
trap cleanup EXIT

cmake --build "$build" >/dev/null
# Staged as the package lays it out (some rules install to /usr/bin).
DESTDIR=$work/stage cmake --install "$build" --prefix /opt/ecce --component client >/dev/null
prefix=$work/stage/opt/ecce
[ -x "$prefix/scripts/ecce-askpass" ] || { echo "FAIL ecce-askpass not installed"; exit 1; }
[ -x "$prefix/bin/passdialog" ] || { echo "FAIL passdialog not installed next to ecce-askpass"; exit 1; }

podman image exists localhost/ecce-test-sshd || {
  ECCE_TEST_SSH_PORT=$port ECCE_TEST_SSH_NAME=$name "$here/run.sh" >/dev/null; }
podman rm -f $name >/dev/null 2>&1 || true
podman run -d --name $name -p 127.0.0.1:$port:22 localhost/ecce-test-sshd \
  /usr/sbin/sshd -D -e -o PubkeyAuthentication=no >/dev/null
for i in $(seq 40); do (exec 3<>/dev/tcp/127.0.0.1/$port) 2>/dev/null && break; sleep 0.5; done

for d in $(seq 90 120); do [ -e /tmp/.X11-unix/X$d ] || break; done
Xvfb :$d -screen 0 800x600x24 >/dev/null 2>&1 & xpid=$!
sleep 1

install -d -m 700 "$work/home" "$work/home/.ssh" "$work/cd" "$work/st"
ssh-keyscan -p $port 127.0.0.1 2>/dev/null > "$work/home/.ssh/known_hosts"
cat > "$work/home/.ssh/config" <<CFG
Host t
  HostName 127.0.0.1
  Port $port
  User bashuser
  PubkeyAuthentication no
  UserKnownHostsFile $work/home/.ssh/known_hosts
CFG
cat > "$work/log-askpass" <<SH
#!/bin/sh
[ -e $work/st/cancelled ] || echo ask >> $work/asks
exec "$prefix/scripts/ecce-askpass" "\$@"
SH
chmod +x "$work/log-askpass"

rc=0
master() {  # answer -> status of ssh -f -N as OpenSshTransport::openMaster runs it
  : > "$work/asks"; rm -f "$work/st/cancelled"
  env -i PATH=/usr/bin:/bin HOME="$work/home" ECCE_REALUSERHOME="$work/home" \
    ECCE_REALUSER="$(id -un)" XDG_RUNTIME_DIR="$work" DISPLAY=:$d GDK_BACKEND=x11 \
    ECCE_HOME="$prefix" ECCE_TEST_AUTH_ANSWER="$1" \
    SSH_ASKPASS="$work/log-askpass" SSH_ASKPASS_REQUIRE=force \
    ECCE_ASKPASS_STATE="$work/st/cancelled" ECCE_ASKPASS_HOST=t ECCE_ASKPASS_USER=bashuser \
    timeout 60 ssh -F "$work/home/.ssh/config" -f -N -x -o BatchMode=no \
      -o ControlMaster=auto -o ControlPath="$work/cd/m" -o ControlPersist=60 t \
      </dev/null >/dev/null 2>"$work/err"
}

echo "=== right password"
master ecce-test || { echo "FAIL master did not open:"; grep -v bind "$work/err" || true; rc=1; }
[ "$(wc -l < "$work/asks")" = 1 ] || { echo "FAIL $(wc -l < "$work/asks") dialogs, want 1"; rc=1; }
out=$(ssh -F "$work/home/.ssh/config" -o BatchMode=yes -o ControlPath="$work/cd/m" t echo over-master 2>&1) || true
[ "$out" = over-master ] || { echo "FAIL command over the master: $out"; rc=1; }
ssh -F "$work/home/.ssh/config" -o ControlPath="$work/cd/m" -O exit t 2>/dev/null || true

echo "=== cancel"
if master cancel; then echo "FAIL master opened after a cancel"; rc=1; fi
[ "$(wc -l < "$work/asks")" -le 1 ] || { echo "FAIL $(wc -l < "$work/asks") dialogs after a cancel, want 1"; rc=1; }

[ $rc = 0 ] && echo PASS
exit $rc
