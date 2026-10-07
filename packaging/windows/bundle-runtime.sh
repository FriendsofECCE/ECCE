#!/bin/bash
# Copies the MinGW/UCRT64 runtime DLLs every installed program needs, and
# mosquitto, mosquitto_passwd (the session broker, ecce-broker-win) and the
# mosquitto_sub/pub clients (the tests use them), from
# on a machine without MSYS2.
#
#   packaging/windows/bundle-runtime.sh <install-dir> [ucrt64-bin]
#
# Run it from an MSYS2 UCRT64 shell after `cmake --install`.  Only DLLs found
# in the UCRT64 bin directory are copied; Windows' own DLLs stay where they are.
set -eu
dest="${1:?usage: bundle-runtime.sh <install-dir> [ucrt64-bin]}/bin"
src="${2:-/ucrt64/bin}"
src="$(cd "$src" && pwd)"

for p in mosquitto mosquitto_passwd mosquitto_sub mosquitto_pub; do
  [ -f "$src/$p.exe" ] || { echo "bundle-runtime: $src/$p.exe is missing" >&2; exit 1; }
  cp -f "$src/$p.exe" "$dest/"
done

# ldd lists a program's DLLs, but not always the DLLs' own; repeat on the
# newly copied files until nothing new turns up.
todo="$(ls "$dest"/*.exe)"
n=0
while [ -n "$todo" ]; do
  new=""
  for f in $todo; do
    for dll in $(ldd "$f" 2>/dev/null | awk -v s="$src/" 'index($3, s) == 1 {print $3}'); do
      if [ ! -f "$dest/${dll##*/}" ]; then cp -f "$dll" "$dest/"; new="$new $dest/${dll##*/}"; n=$((n+1)); fi
    done
  done
  todo="$new"
done
echo "bundled mosquitto and $n DLLs into $dest ($(du -sh "$dest" | cut -f1) in bin)"
