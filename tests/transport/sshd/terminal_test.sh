#!/bin/bash
# EcceShell's local terminal for an ssh machine (#204), against the test
# sshd from run.sh, in the libssh build container.  The terminal is a stub
# that runs the -e command under script(1) with typed input.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
state=${XDG_RUNTIME_DIR:-/tmp}/ecce-test-sshd
port=${ECCE_TEST_SSH_PORT:-2222}
if ! podman image exists localhost/ecce-sshbuild; then
  podman build -q -t localhost/ecce-sshbuild -f "$here/Containerfile.build" "$here" >/dev/null
fi
podman run --rm --network host -v "$root:/src:Z" -v "$state:/state:Z,ro" \
  -e PORT="$port" -e SHELLCMD="${SHELLCMD:-}" localhost/ecce-sshbuild bash -c '
  set -e
  install -d -m 700 /root/.ssh
  install -m 600 /state/id_ed25519 /root/.ssh/ecce_test_key
  ssh-keyscan -p $PORT 127.0.0.1 2>/dev/null > /root/.ssh/known_hosts
  printf "Host 127.0.0.1\n  Port $PORT\n  IdentityFile /root/.ssh/ecce_test_key\n  IdentitiesOnly yes\n" > /root/.ssh/config
  cd /src
  [ -f build-ssh/build.ninja ] || cmake -G Ninja -B build-ssh >/dev/null
  ninja -C build-ssh testTerminal
  eh=/tmp/eh; rm -rf $eh; mkdir -p $eh /root/.ECCE
  ln -s /src/siteconfig $eh/siteconfig; ln -s /src/data $eh/data
  t=$(printf "\t")
  for m in tbash tcsh tbashsrc tcshsrc; do
    printf "$m${t}127.0.0.1${t}test${t}test${t}test${t}1:1${t}ssh${t}na${t}na\n"
  done > /root/.ECCE/MyMachines
  printf "shell: bash\nsourceFile: /tmp/ecce-tsrc.sh\n" > /root/.ECCE/CONFIG.tbashsrc
  printf "shell: csh\nsourceFile: /tmp/ecce-tsrc.csh\n" > /root/.ECCE/CONFIG.tcshsrc
  printf "shell: csh\n" > /root/.ECCE/CONFIG.tcsh
  # Remote fixtures, made by the bash account.
  ssh bashuser@127.0.0.1 "umask 000; mkdir -p /tmp/ecce-tdir; \
    echo \"export ECCE_TV=fromsh\" > /tmp/ecce-tsrc.sh; \
    echo \"setenv ECCE_TV fromcsh\" > /tmp/ecce-tsrc.csh; \
    printf \"A\nB\nTAILLINE\n\" > /tmp/ecce-ttail.txt"
  cat > /tmp/stubterm <<"STUB"
#!/bin/bash
printf "%s\n" "$@" > $STUB_OUT.argv
shift
cmd=$(printf "%q " "$@")
(sleep 3; printf "echo PWD=\`pwd\`; echo TV=\`printenv ECCE_TV\`; exit\n"; sleep 2) |
  timeout 25 script -qec "$cmd" /dev/null > $STUB_OUT.log 2>&1
echo done > $STUB_OUT.done
STUB
  chmod +x /tmp/stubterm
  export ECCE_HOME=$eh ECCE_REALUSER=root ECCE_REALUSERHOME=/root \
         ECCE_TERMINAL=/tmp/stubterm STUB_DIRS=1
  [ -z "${SHELLCMD:-}" ] || exec bash -c "$SHELLCMD"
  build-ssh/testTerminal'
