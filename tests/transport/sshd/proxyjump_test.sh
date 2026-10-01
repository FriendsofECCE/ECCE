#!/bin/bash
# Hosts reached through ProxyJump/ProxyCommand in ~/.ssh/config (#204).  A
# jump sshd is published on 127.0.0.1:$ECCE_PJ_PORT; the target is on a
# private network behind it.  ECCE picks the OpenSSH client for such a host
# and supplies its own ControlMaster, since the user's config has none.
#   pjkey  ProxyJump, keys          pjcmd  ProxyCommand ssh -W, keys
#   pjpw   ProxyJump, password on the jump host and on the target
#   1. testRCommandSsh (commands, get/put, background job, streams) over pjkey, pjcmd.
#   2. testControlMasterLoss --askpass: pjpw asks (jump, target) once, not per command.
#   3. a user's own ControlPath is left alone.
# Own names and ports (ECCE_PJ_NAME, ECCE_PJ_PORT); builds in build-ssh.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
pfx=${ECCE_PJ_NAME:-ecce-pj}
state=${XDG_RUNTIME_DIR:-/tmp}/$pfx-sshd
port=${ECCE_PJ_PORT:-2271}
net=$pfx-net
mkdir -p "$state"
[ -f "$state/id_ed25519" ] || ssh-keygen -q -t ed25519 -N '' -f "$state/id_ed25519"

cleanup() { podman rm -f $pfx-jump $pfx-node >/dev/null 2>&1 || true
            podman network rm $net >/dev/null 2>&1 || true; }
trap cleanup EXIT
cleanup

ctx=$(mktemp -d)
cp "$here/Containerfile" "$ctx/"; cp "$state/id_ed25519.pub" "$ctx/authorized_keys"
podman build -q -t localhost/$pfx-sshd "$ctx" >/dev/null
rm -rf "$ctx"
podman image exists localhost/ecce-sshbuild ||
  podman build -q -t localhost/ecce-sshbuild -f "$here/Containerfile.build" "$here" >/dev/null
podman network create $net >/dev/null
podman run -d --name $pfx-node --hostname node --network $net --network-alias node \
  localhost/$pfx-sshd >/dev/null
podman run -d --name $pfx-jump --hostname jump --network $net --network-alias jump \
  -p 127.0.0.1:$port:22 localhost/$pfx-sshd >/dev/null
for i in $(seq 40); do (exec 3<>/dev/tcp/127.0.0.1/$port) 2>/dev/null && break; sleep 0.5; done
for i in $(seq 20); do
  podman exec $pfx-node test -s /etc/ssh/ssh_host_ed25519_key.pub && break; sleep 0.5
done
podman exec $pfx-node cat /etc/ssh/ssh_host_ed25519_key.pub | cut -d' ' -f1,2 |
  sed 's/^/node /' > "$state/node_known_hosts"

podman run --rm --network host -v "$root:/src:Z" -v "$state:/state:Z,ro" \
  -e PORT="$port" localhost/ecce-sshbuild bash -c '
  set -e
  install -d -m 700 /root/.ssh
  install -m 600 /state/id_ed25519 /root/.ssh/ecce_test_key
  { ssh-keyscan -p $PORT 127.0.0.1 2>/dev/null; cat /state/node_known_hosts; } > /root/.ssh/known_hosts
  cat > /root/.ssh/config <<CFG
Host jumpk
  HostName 127.0.0.1
  Port $PORT
  User bashuser
  IdentityFile /root/.ssh/ecce_test_key
  IdentitiesOnly yes
Host jumppw
  HostName 127.0.0.1
  Port $PORT
  User bashuser
  IdentitiesOnly yes
  PubkeyAuthentication no
  PreferredAuthentications password
Host pjkey
  HostName node
  IdentityFile /root/.ssh/ecce_test_key
  IdentitiesOnly yes
  ProxyJump jumpk
Host pjcmd
  HostName node
  IdentityFile /root/.ssh/ecce_test_key
  IdentitiesOnly yes
  ProxyCommand ssh -W %h:%p -F /root/.ssh/config jumpk
Host pjpw
  HostName node
  IdentitiesOnly yes
  PubkeyAuthentication no
  PreferredAuthentications password
  ProxyJump jumppw
Host pjown
  HostName node
  IdentityFile /root/.ssh/ecce_test_key
  IdentitiesOnly yes
  ProxyJump jumpk
  ControlMaster auto
  ControlPath /root/.ssh/own-%C
  ControlPersist 10m
CFG
  export ECCE_HOME=/src
  cd /src
  [ -f build-ssh/build.ninja ] || cmake -G Ninja -B build-ssh >/dev/null
  ninja -C build-ssh testRCommandSsh testControlMasterLoss >/dev/null
  rc=0
  for h in pjkey pjcmd; do
    echo "--- $h: RCommand over the OpenSSH client with ECCE'"'"'s own master"
    ECCE_TEST_CONTROLMASTER=1 ECCE_TEST_HOST=$h ECCE_TEST_EXPECT_BACKEND=openssh \
      build-ssh/testRCommandSsh || rc=1
    n=$(ls /root/.ECCE/cm 2>/dev/null | wc -l)
    [ "$n" -gt 0 ] && echo "ok   a control socket under ~/.ECCE/cm" ||
      { echo "FAIL no control socket under ~/.ECCE/cm"; rc=1; }
    for u in cshuser bashuser; do ssh -O exit -o ControlPath=/root/.ECCE/cm/%C -l $u $h 2>/dev/null || true; done
  done
  echo "--- a ControlPath of the user'"'"'s own is not replaced"
  ECCE_TEST_CONTROLMASTER=1 ECCE_TEST_HOST=pjown ECCE_TEST_EXPECT_BACKEND=openssh \
    build-ssh/testRCommandSsh >/dev/null || rc=1
  ls /root/.ssh/own-* >/dev/null 2>&1 && echo "ok   the user'"'"'s socket was used" ||
    { echo "FAIL the user'"'"'s ControlPath was not used"; rc=1; }
  for u in cshuser bashuser; do ssh -O exit -l $u pjown 2>/dev/null || true; done

  fh=/tmp/fakehome; rm -rf $fh; mkdir -p $fh/bin
  ln -s /src/siteconfig $fh/siteconfig; ln -s /src/data $fh/data
  cat > $fh/bin/passdialog <<STUB
#!/bin/sh
echo "passdialog \$*" >> \$ASKLOG
echo ecce-test
STUB
  cat > $fh/bin/hostkeydialog <<STUB
#!/bin/sh
echo "hostkeydialog \$*" >> \$ASKLOG
echo accept
STUB
  chmod +x $fh/bin/*
  export ECCE_ASKPASS=/src/scripts/ecce-askpass ASKLOG=/tmp/ask.log
  echo "--- pjpw: passwords asked once (jump, target) for several commands"
  rm -f $ASKLOG
  ECCE_TEST_SSH_OPTS="-o ControlPath=/root/.ECCE/cm/%C" ECCE_HOME=$fh build-ssh/testControlMasterLoss --askpass ok bashuser pjpw 2 || rc=1
  cat $ASKLOG
  ssh -O exit -o ControlPath=/root/.ECCE/cm/%C -l bashuser pjpw 2>/dev/null || true
  exit $rc' 
