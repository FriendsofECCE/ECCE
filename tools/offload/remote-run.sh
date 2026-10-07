#!/bin/bash
# Runs on radium (sent by run-on-radium.sh).  $1 = ref, $2... = command.
set -u
ref=$1; shift
cmd=$1; shift
# Set by run-offload.sh: OFF (work root), MODE (container|native), SLOTS, JOBS.
OFF=${OFF:-$HOME/ecce-offload}
MODE=${MODE:-container}
REPO_URL=https://github.com/FriendsofECCE/ECCE.git
IMAGE=ecce-trixie-build
SLOTS=${SLOTS:-2}
JOBS=${JOBS:-6}
mkdir -p "$OFF"/{src,wt,build,logs,locks,home,cache,tmp}

safe=$(printf %s "$ref" | tr -c 'A-Za-z0-9._-' '_')
stamp=$(date +%Y%m%d-%H%M%S)
log=$OFF/logs/$stamp-$safe-$$.log
exec 3>&1
# Everything below goes to the log; the summary is printed on fd 3.
exec >"$log" 2>&1
say() { echo "$@" >&3; }
fail() { say "FAIL: $*  (log: $(hostname):$log)"; exit 1; }

# Per-ref lock: two jobs on one ref would share one build directory.
exec 8>"$OFF/locks/ref-$safe"
if ! flock -n 8; then say "waiting: another job is using ref $ref"; flock 8; fi

# Slot lock: at most $SLOTS jobs at once.
got=
while [ -z "$got" ]; do
    for i in $(seq 1 $SLOTS); do
        exec 9>"$OFF/locks/slot$i"
        if flock -n 9; then got=$i; break; fi
    done
    [ -n "$got" ] || { [ -n "${told:-}" ] || say "waiting: all $SLOTS slots busy"; told=1; sleep 15; }
done

# Source: one clone, one detached worktree per ref.
(
    flock 7
    [ -d "$OFF/src/.git" ] || git clone --depth 50 "$REPO_URL" "$OFF/src"
    cd "$OFF/src" || exit 1
    # Shallow, and only the branches asked for, to keep downloads small.
    name=${ref#origin/}
    git fetch --depth 50 origin "+refs/heads/main:refs/remotes/origin/main"
    case $name in
      main|HEAD) ;;
      *[!0-9a-f]*|?|??|???) git fetch --depth 50 origin "+refs/heads/$name:refs/remotes/origin/$name" || git fetch --depth 50 origin "$name" ;;
      *) git fetch --depth 50 origin "$name" || true ;;
    esac
) 7>"$OFF/locks/git" || fail "git fetch"
cd "$OFF/src"
sha=$(git rev-parse --verify -q "$ref^{commit}" || git rev-parse --verify -q "origin/$ref^{commit}") \
    || fail "ref $ref not found on GitHub (push it first)"
wt=$OFF/wt/$safe
(
    flock 7
    if [ -d "$wt" ]; then git -C "$wt" checkout -q --detach -f "$sha"
    else git worktree add -q --detach "$wt" "$sha"; fi
) 7>"$OFF/locks/git" || fail "checkout"
bdir=$OFF/build/$safe
mkdir -p "$bdir" "$OFF/home/$safe"
echo "ref $ref = $sha, command: $cmd $*"
say "$(hostname): $ref = ${sha:0:10}, $cmd $* (slot $got)"

# $SRC is the checkout, $BLD the persistent per-ref build directory.
case $cmd in
  build)    inner='configure; ninja -C $BLD/b -j'$JOBS' -k 0' ;;
  ctest)    inner='configure; ninja -C $BLD/b -j'$JOBS' -k 0 && cd $BLD/b && ctest -j${CTEST_JOBS:-2} --output-on-failure "$@"' ;;
  teaching) inner='configure; ninja -C $BLD/b -j'$JOBS' -k 0 && cd $SRC && tests/teaching/run_tests.py --build $BLD/b --jobs 4 "$@"' ;;
  apps)     inner='install_tree; cd $SRC && ECCE_TEST_HOME=$BLD/install ECCE_TEST_WRAPPERS=$BLD/install-bin tests/apps/run_tests.py "$@"' ;;
  run)      inner='configure; cd $SRC && "$@"' ;;
  *) fail "unknown command '$cmd' (build|ctest|teaching|apps|run)" ;;
esac
prelude='
configure() {
  [ -f $BLD/b/build.ninja ] || cmake -G Ninja -S $SRC -B $BLD/b -DECCE_REQUIRE_LIBSSH=ON || return 1
}
install_tree() {
  [ -f $BLD/bi/build.ninja ] || cmake -G Ninja -S $SRC -B $BLD/bi \
    -DCMAKE_INSTALL_PREFIX=$BLD/install -DECCE_HOME_DIR=$BLD/install \
    -DECCE_WRAPPER_DESTINATION=$BLD/install-bin || return 1
  ninja -C $BLD/bi -j'$JOBS' -k 0 && ninja -C $BLD/bi install
}
set -e'

start=$(date +%s)
# Inside the container the checkout, build dir and home are /work/*; native
# hosts use the real paths.  Only the work root is ever written to.
if [ "$MODE" = container ]; then
    podman run --rm --memory 12g --memory-swap 12g --userns=keep-id --shm-size 1g \
        -e HOME=/work/home -e USER="$(id -un)" -e LOGNAME="$(id -un)" -e SRC=/work/src -e BLD=/work/build -e TERM=dumb \
        -e CTEST_OUTPUT_ON_FAILURE=1 \
        -v "$wt":/work/src -v "$bdir":/work/build -v "$OFF/home/$safe":/work/home \
        -w /work/src "$IMAGE" bash -c "$prelude
$inner" bash "$@" </dev/null
else
    PATH=$PATH:/usr/sbin:/sbin HOME=$OFF/home/$safe SRC=$wt BLD=$bdir XDG_CACHE_HOME=$OFF/cache TMPDIR=$OFF/tmp \
        TERM=dumb CTEST_OUTPUT_ON_FAILURE=1 \
        bash -c "$prelude
$inner" bash "$@" </dev/null
fi
rc=$?
secs=$(( $(date +%s) - start ))

say "---- last lines of the log ----"
tail -n 25 "$log" >&3
if [ $rc -eq 0 ]; then verdict=PASS; elif [ $rc -eq 77 ]; then verdict=SKIP; else verdict=FAIL; fi
say "$verdict: $cmd $* on $ref (${sha:0:10}) exit $rc, ${secs}s  (log: $(hostname):$log)"
exit $rc
