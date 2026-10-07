#!/bin/bash
# Run Management's Tail over ssh logs in once (#204 follow-up): TailSource and
# the Tail window against the test sshd, once with keyboard-interactive and
# once with password-only login.  Each run starts with an empty AuthCache, so
# the login must ask exactly once (the passdialog stub counts) and sshd must
# see exactly one password login (its log counts); lines appended to the
# remote file afterwards must arrive.  The window runs on a private Xvfb of
# this host, reached from the build container through /tmp/.X11-unix.
#   tail_test.sh [PNG-DIR]
# TAIL_FIXTURE=<file under the source tree> fills the remote file with its
# first 900 lines instead, for a screenshot; only the window then runs.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
state=${XDG_RUNTIME_DIR:-/tmp}/ecce-test-sshd
kport=${ECCE_TAIL_KBD_PORT:-2241}
pport=${ECCE_TAIL_PW_PORT:-2242}
kname=ecce-tail-sshd-kbd pname=ecce-tail-sshd-pw
pngdir=${1:-}

ECCE_TEST_SSH_PORT=$kport ECCE_TEST_SSH_NAME=$kname "$here/run.sh" >/dev/null
podman rm -f $pname >/dev/null 2>&1 || true
podman run -d --name $pname -p 127.0.0.1:$pport:22 localhost/ecce-test-sshd \
  /usr/sbin/sshd -D -e -o KbdInteractiveAuthentication=no >/dev/null
for i in $(seq 20); do (exec 3<>/dev/tcp/127.0.0.1/$pport) 2>/dev/null && break; sleep 0.5; done

exec 9< <(Xvfb -displayfd 1 -nolisten tcp -screen 0 1024x768x24 2>/dev/null)
xpid=$!
read -r -u 9 disp
cleanup() { kill $xpid 2>/dev/null || true; podman rm -f $kname $pname >/dev/null 2>&1 || true; }
trap cleanup EXIT

if ! podman image exists localhost/ecce-sshbuild; then
  podman build -q -t localhost/ecce-sshbuild -f "$here/Containerfile.build" "$here" >/dev/null
fi

logins() {  # container, method: such logins of bashuser
  podman logs $1 2>&1 | grep -cE "Accepted $2 for bashuser" || true
}

incontainer() {  # what to run (tail|window) and the sshd's host alias
  podman run --rm --network host -v "$root:/src:Z" -v "$state:/state:Z,ro" \
    -v /tmp/.X11-unix:/tmp/.X11-unix -e DISPLAY=:$disp \
    -e KPORT=$kport -e PPORT=$pport -e WHAT=$1 -e HOSTALIAS=$2 \
    -e TAIL_FIXTURE="${TAIL_FIXTURE:-}" \
    localhost/ecce-sshbuild bash -c '
  set -e
  install -d -m 700 /root/.ssh
  install -m 600 /state/id_ed25519 /root/.ssh/ecce_test_key
  { ssh-keyscan -p $KPORT 127.0.0.1; ssh-keyscan -p $PPORT 127.0.0.1; } 2>/dev/null > /root/.ssh/known_hosts
  # kbd and pw log in by password; keyed reaches the same sshd with the
  # test key, to write the file by other means.
  [ $HOSTALIAS = kbd ] && port=$KPORT || port=$PPORT
  printf "Host kbd\n  HostName 127.0.0.1\n  Port $KPORT\n  IdentitiesOnly yes\nHost pw\n  HostName 127.0.0.1\n  Port $PPORT\n  IdentitiesOnly yes\nHost keyed\n  HostName 127.0.0.1\n  Port $port\n  User bashuser\n  IdentityFile /root/.ssh/ecce_test_key\n  IdentitiesOnly yes\n  BatchMode yes\n" > /root/.ssh/config
  cd /src
  # The image has no Coin3D; the viewer is not under test.
  cmake -G Ninja -B build-ssh -DECCE_USE_COIN=OFF >/dev/null
  ninja -j2 -C build-ssh testTail tailwindow >/dev/null
  eh=/tmp/eh; rm -rf $eh; mkdir -p $eh/bin /root/.ECCE
  ln -s /src/siteconfig $eh/siteconfig; ln -s /src/data $eh/data
  cat > $eh/bin/passdialog <<STUB
#!/bin/sh
echo "passdialog \$*" >> /tmp/ask.log
echo ecce-test
STUB
  chmod +x $eh/bin/passdialog
  rm -f /tmp/ask.log /root/.ECCE/*
  printf "ttail\t$HOSTALIAS\tt\tt\tt\t1:1\tssh\tna\tna\n" > /root/.ECCE/MyMachines
  f=/tmp/ecce-tail-$$.out
  if [ -n "${TAIL_FIXTURE:-}" ]; then
    # For a screenshot: the start of a real output file.
    head -n 900 "/src/$TAIL_FIXTURE" | ssh keyed "cat > $f"
    export TAIL_WAIT="$(head -n 900 "/src/$TAIL_FIXTURE" | tail -n 1)"
  else
    ssh keyed "printf \"A\nB\nTAILLINE\n\" > $f"
  fi
  export ECCE_HOME=$eh ECCE_REALUSER=root ECCE_REALUSERHOME=/root \
         ECCE_AUTHCACHE_NO_BROADCAST=1 F=$f
  append="ssh keyed \"echo \$TAIL_MARKER >> $f\""
  rc=0
  if [ $WHAT = tail ]; then
    build-ssh/testTail --remote ttail bashuser $f "$append" || rc=1
  else
    TAIL_WAIT="${TAIL_WAIT:-TAILLINE}" build-ssh/tailwindow ttail bashuser $f \
      /tmp/tail-$HOSTALIAS.png "$append" 2>&1 | grep -v Gtk- || rc=1
    [ -d /src/build-ssh/png ] || mkdir -p /src/build-ssh/png
    cp /tmp/tail-$HOSTALIAS.png /src/build-ssh/png/ 2>/dev/null || true
  fi
  n=$(grep -c . /tmp/ask.log 2>/dev/null || echo 0)
  cat /tmp/ask.log 2>/dev/null || true
  [ "$n" = 1 ] && echo "ok   asked for the password once" ||
    { echo "FAIL asked $n times"; rc=1; }
  sleep 1
  ssh keyed "pgrep -u bashuser -f \"tail -n [0-9]* -F -- $f\"" >/dev/null &&
    { echo "FAIL tail still runs on the machine"; rc=1; } ||
    echo "ok   no tail left on the machine"
  exit $rc'
}

rc=0
whats="tail window"; [ -z "${TAIL_FIXTURE:-}" ] || whats=window
for what in $whats; do
  for pair in "kbd $kname keyboard-interactive/pam" "pw $pname password"; do
    set -- $pair
    echo "--- $what over $1 login"
    before=$(logins $2 "(password|keyboard-interactive/pam)")
    one=$(logins $2 $3)
    incontainer $what $1 || rc=1
    after=$(logins $2 "(password|keyboard-interactive/pam)")
    if [ $((after - before)) = 1 ] && [ $(($(logins $2 $3) - one)) = 1 ]; then
      echo "ok   sshd saw one login, by $3"
    else
      echo "FAIL sshd saw $((after - before)) password logins"; rc=1
    fi
  done
done
if [ -n "$pngdir" ]; then
  mkdir -p "$pngdir"
  cp "$root"/build-ssh/png/*.png "$pngdir"/
fi
exit $rc
