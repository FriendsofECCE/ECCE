#!/bin/sh
# scripts/ecce-macos-terminal with a stub open(1) that runs the .command
# file as Terminal would: the command gets its arguments intact (space,
# quote), the title escape is written, and the wrapper returns only after
# the command has ended.  Runs anywhere; on macOS the real open is shadowed.
set -u
src=${1:?usage: macos_terminal_test.sh <source dir>}
tmp=$(mktemp -d "${TMPDIR:-/tmp}/ecce-mterm.XXXXXX")
trap 'rm -rf "$tmp"' EXIT
mkdir "$tmp/bin"
cat > "$tmp/bin/open" <<'STUB'
#!/bin/sh
[ "$1" = -a ] && [ "$2" = Terminal ] || { echo "stub open: $*" >&2; exit 2; }
( sleep 1; sh "$3" > "$OUT.run" 2>&1 ) &
STUB
chmod +x "$tmp/bin/open"
fail=0
check() { if [ "$2" = "$3" ]; then echo "ok   $1"; else echo "FAIL $1"; echo "     got  [$2]"; echo "     want [$3]"; fail=1; fi; }

OUT=$tmp/out PATH="$tmp/bin:$PATH" TMPDIR=$tmp \
  sh "$src/scripts/ecce-macos-terminal" -title "Calc it's" -geom 80x40 \
  -e sh -c 'printf "%s|" "$@"; echo; touch "$0.ran"' "$tmp/out" "a b" "c'd"
rc=$?
check "exit status" "$rc" 0
check "ran before return" "$(test -e "$tmp/out.ran" && echo yes)" yes
check "arguments" "$(tail -1 "$tmp/out.run" | cut -d"$(printf '\007')" -f2)" "a b|c'd|"
check "title" "$(head -c 7 "$tmp/out.run" | od -An -c | tr -s ' ')" " 033 ] 0 ; C a l"
check "temp files removed" "$(ls -d "$tmp"/ecce-term.* 2>/dev/null)" ""

out=$(ECCE_TERMINAL_DRYRUN=1 TMPDIR=$tmp sh "$src/scripts/ecce-macos-terminal" -e vi -R "/x y/f")
check "dry run command" "$(echo "$out" | sed -n 4p)" " 'vi' '-R' '/x y/f'"
check "dry run open" "$(echo "$out" | tail -1 | cut -d' ' -f1-3)" "open -a Terminal"
[ $fail = 0 ] && echo PASSED || echo FAILED
exit $fail
