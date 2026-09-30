#!/bin/bash
# Front-end tests (#204): a `login` sshd published on 127.0.0.1:$PORT_FWD
# (TCP forwarding allowed), a `loginnf` on $PORT_NOFWD (forwarding off), and
# a `node` sshd on a private network, reachable only from those two.  Builds
# RCommand with libssh in a container and runs testRCommandFrontend twice:
# through a forwarded connection and through a nested ssh.
# Own names and ports, so it can run beside rcommand_test.sh.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
state=${XDG_RUNTIME_DIR:-/tmp}/ecce-fe-sshd
pfwd=${ECCE_FE_PORT_FWD:-2230}
pnofwd=${ECCE_FE_PORT_NOFWD:-2231}
net=ecce-fe-net
mkdir -p "$state"
[ -f "$state/id_ed25519" ] || ssh-keygen -q -t ed25519 -N '' -f "$state/id_ed25519"

cleanup() { podman rm -f ecce-fe-login ecce-fe-loginnf ecce-fe-node >/dev/null 2>&1 || true
            podman network rm $net >/dev/null 2>&1 || true; }
trap cleanup EXIT
cleanup
fail=0

ctx=$(mktemp -d)
cp "$here/Containerfile" "$ctx/"; cp "$state/id_ed25519.pub" "$ctx/authorized_keys"
podman build -q -t localhost/ecce-fe-sshd "$ctx" >/dev/null
rm -rf "$ctx"
if ! podman image exists localhost/ecce-sshbuild; then
  podman build -q -t localhost/ecce-sshbuild -f "$here/Containerfile.build" "$here" >/dev/null
fi
podman network create $net >/dev/null

# The front ends hold the users' private key for the nested case.
start() {  # name host extra-args sshd-conf
  podman run -d --name ecce-fe-$1 --hostname $1 --network $net \
    --network-alias $1 $3 -v "$state/id_ed25519:/keys/id:ro,Z" \
    localhost/ecce-fe-sshd sh -c "
      for u in cshuser bashuser; do
        install -m 600 -o \$u -g \$u /keys/id /home/\$u/.ssh/id_ed25519; done
      $4
      exec /usr/sbin/sshd -D -e" >/dev/null
}
start node node "" "printf 'PasswordAuthentication no\nKbdInteractiveAuthentication no\n' > /etc/ssh/sshd_config.d/00-keyonly.conf"
start login login "-p 127.0.0.1:$pfwd:22" ""
start loginnf loginnf "-p 127.0.0.1:$pnofwd:22" "echo 'AllowTcpForwarding no' > /etc/ssh/sshd_config.d/00-nofwd.conf"
for c in login loginnf; do
  for i in $(seq 20); do
    podman exec ecce-fe-$c sh -c 'ssh-keyscan node 2>/dev/null > /etc/ssh/ssh_known_hosts; test -s /etc/ssh/ssh_known_hosts' && break
    sleep 0.5
  done
done
nodekey=$(podman exec ecce-fe-node cat /etc/ssh/ssh_host_ed25519_key.pub | cut -d' ' -f1,2)
echo "node $nodekey" > "$state/node_known_hosts"

# SshTransport alone (g++ and libssh only), then RCommand on top of it.
podman run --rm --network host -v "$root:/src:Z" -v "$state:/state:Z,ro" \
  -e PFWD="$pfwd" -e PNOFWD="$pnofwd" localhost/ecce-sshbuild bash -c '
  set -e
  g++ -std=c++17 -Wall -Wextra -O1 -I/src/include -o /tmp/tsf \
    /src/tests/transport/testSshFrontend.C /src/src/comm/rcommand/SshTransport.C \
    /src/src/comm/rcommand/Transport.C -lssh
  /tmp/tsf /state/id_ed25519 $PFWD forward
  /tmp/tsf /state/id_ed25519 $PNOFWD nested
  ECCE_SSH_FRONTEND=nested /tmp/tsf /state/id_ed25519 $PFWD nested' || fail=1
# The oldest libssh we support (RHEL 9 has 0.10.4).
if [ -n "${ECCE_FE_ROCKY:-}" ]; then
  podman run --rm --network host -v "$root:/src:Z" -v "$state:/state:Z,ro" \
    -e PFWD="$pfwd" -e PNOFWD="$pnofwd" docker.io/rockylinux/rockylinux:9 bash -c '
    set -e
    dnf -q -y install gcc-c++ libssh-devel >/dev/null
    rpm -q libssh
    g++ -std=c++17 -Wall -Wextra -O1 -I/src/include -o /tmp/tsf \
      /src/tests/transport/testSshFrontend.C /src/src/comm/rcommand/SshTransport.C \
      /src/src/comm/rcommand/Transport.C -lssh
    /tmp/tsf /state/id_ed25519 $PFWD forward
    /tmp/tsf /state/id_ed25519 $PNOFWD nested
    ECCE_SSH_FRONTEND=nested /tmp/tsf /state/id_ed25519 $PFWD nested' || fail=1
fi
podman run --rm --network host -v "$root:/src:Z" -v "$state:/state:Z,ro" \
  -e PFWD="$pfwd" -e PNOFWD="$pnofwd" -e CMD="${ECCE_FE_CMD:-}" localhost/ecce-sshbuild bash -c '
  set -e
  install -d -m 700 /root/.ssh
  install -m 600 /state/id_ed25519 /root/.ssh/ecce_test_key
  { ssh-keyscan -p $PFWD 127.0.0.1; ssh-keyscan -p $PNOFWD 127.0.0.1; } 2>/dev/null > /root/.ssh/known_hosts
  cat /state/node_known_hosts >> /root/.ssh/known_hosts
  for h in login:$PFWD loginnf:$PNOFWD; do
    printf "Host ${h%%:*}\n  HostName 127.0.0.1\n  Port ${h##*:}\n  IdentityFile /root/.ssh/ecce_test_key\n  IdentitiesOnly yes\n" >> /root/.ssh/config
  done
  # The client'"'"'s own key is what a forwarded connection offers the node.
  printf "Host node\n  IdentityFile /root/.ssh/ecce_test_key\n  IdentitiesOnly yes\n" >> /root/.ssh/config
  export ECCE_HOME=/src
  cd /src
  [ -f build-ssh/build.ninja ] || cmake -G Ninja -B build-ssh >/dev/null
  ninja -C build-ssh testRCommandFrontend
  [ -z "${CMD:-}" ] || { eval "$CMD"; exit $?; }
  rc=0
  build-ssh/testRCommandFrontend login forward || rc=1
  build-ssh/testRCommandFrontend loginnf nested || rc=1
  ECCE_SSH_FRONTEND=nested build-ssh/testRCommandFrontend login nested || rc=1
  # The node takes only the front end'"'"'s key: the forward is refused at
  # authentication and the connection must fall back to the nested ssh.
  sed -i "/^Host node\$/,\$d" /root/.ssh/config
  build-ssh/testRCommandFrontend login nested || rc=1
  exit $rc' || fail=1
exit $fail
