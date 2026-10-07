#!/bin/bash
# The Tail window on a local machine (DirectTransport), on a private Xvfb.
#   tail_window_test.sh TAILWINDOW-BINARY SOURCE-DIR [PNG]
# Exit 77 (skip) without xvfb-run.
set -u
bin=$1 src=$2 png=${3:-}
command -v xvfb-run >/dev/null || { echo "SKIP: no xvfb-run"; exit 77; }
tmp=$(mktemp -d /tmp/ecce-tailwin.XXXXXX)
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/home/.ECCE" "$tmp/ecce"
ln -s "$src/siteconfig" "$tmp/ecce/siteconfig"
ln -s "$src/data" "$tmp/ecce/data"
printf 'tlocal\tlocalhost\tt\tt\tt\t1:1\tssh\tna\tna\n' > "$tmp/home/.ECCE/MyMachines"
out="$tmp/run dir/h2o.out"
mkdir -p "$tmp/run dir"
printf 'line one\nline two\nTAILLINE\n' > "$out"
export ECCE_HOME="$tmp/ecce" ECCE_REALUSERHOME="$tmp/home" \
       ECCE_REALUSER=${ECCE_REALUSER:-${USER:-nobody}} OUT="$out"
xvfb-run -a -s "-screen 0 1024x768x24" ${TAIL_GDB:-} \
  "$bin" tlocal - "$out" "${png:-$tmp/tail.png}" 'echo "$TAIL_MARKER" >> "$OUT"'
rc=$?
if pgrep -f -- "tail -n [0-9]* -F -- $out" >/dev/null; then
  echo "FAIL a tail is still running"; rc=1
fi
exit $rc
