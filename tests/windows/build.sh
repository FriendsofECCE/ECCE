#!/bin/bash
# usage: JOBS=8 build.sh [targets...]   (run from an MSYS2 UCRT64 shell, see b.bat)
B=wip/133-windows
cd ~/ECCE; git stash -q
git fetch -q origin "$B" && git checkout -q -B "$B" FETCH_HEAD || exit 1
git reset -q --hard FETCH_HEAD
git log --oneline -1
cmake --build build -- -k 0 -j"${JOBS:-8}" "$@" > ~/build.log 2>&1
echo "build rc=$?"
grep -c "^FAILED" ~/build.log
