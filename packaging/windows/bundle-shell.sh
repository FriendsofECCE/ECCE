#!/bin/bash
# Copies the minimal POSIX shell the Windows client runs job scripts with
# (sh.exe and the coreutils those scripts call) and the msys-*.dll files
# they need from an MSYS2 install into <ECCE install>/usr/bin, where
# DirectTransport looks first (<ECCE_HOME>\usr\bin\sh.exe).
#
#   packaging/windows/bundle-shell.sh <install-dir> [msys2-root]
#
# Run it from an MSYS2 shell.  The program list below is what the job
# scripts (gensub output), eccejobmonitor and the Shell queue commands use;
# add a name here when a script starts needing another tool.
set -eu
dest="${1:?usage: bundle-shell.sh <install-dir> [msys2-root]}/usr/bin"
root="${2:-/c/msys64}"
src="$root/usr/bin"

PROGRAMS="sh bash cat cp mv rm ln mkdir rmdir ls chmod touch date sleep
  grep egrep sed awk gawk tr cut head tail wc sort uniq tee basename dirname
  expr env nohup id uname hostname uptime df du ps kill pwd printf test
  readlink realpath cmp diff find xargs stat true false echo"

mkdir -p "$dest"
missing=""
for p in $PROGRAMS; do
  if [ -f "$src/$p.exe" ]; then cp -f "$src/$p.exe" "$dest/"
  else missing="$missing $p"; fi
done
[ "$missing" ] && echo "not in $src (skipped):$missing" >&2

# Every msys-*.dll the copied programs load, found with ldd.
for exe in "$dest"/*.exe; do
  ldd "$exe" 2>/dev/null | awk '/msys-.*\.dll/ {print $3}'
done | sort -u | while read -r dll; do cp -f "$dll" "$dest/"; done

# /etc/passwd-less runs read this; keeps MSYS from probing a domain.
mkdir -p "$dest/../../etc"
: > "$dest/../../etc/fstab"

echo "bundled $(ls "$dest" | wc -l) files into $dest:"
ls "$dest"
