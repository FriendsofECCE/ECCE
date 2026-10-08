#!/bin/bash
# no-build-paths.sh <build-dir> <package or directory>...: fail when an
# installed text file names the directory the package was built in. Such a
# path works on the build machine and nowhere else (#133: the macOS
# wrappers defaulted ECCE_HOME to the CI runner's staging tree).
# Accepts .deb and .rpm files and unpacked trees (ECCE.app). Binaries are
# not checked: their __FILE__ strings are harmless.
set -u
builddir=${1:?usage: no-build-paths.sh <build-dir> <package or dir>...}
shift
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
bad=0
for p in "$@"; do
  case "$p" in
    *.deb) dir="$work/$(basename "$p")"; mkdir -p "$dir"
           dpkg-deb -x "$p" "$dir" && dpkg-deb -e "$p" "$dir/DEBIAN" || exit 2 ;;
    *.rpm) dir="$work/$(basename "$p")"; mkdir -p "$dir"
           p=$(cd "$(dirname "$p")" && pwd)/$(basename "$p")
           (cd "$dir" && rpm2cpio "$p" | cpio -idm --quiet) || exit 2 ;;
    *)     dir=$p ;;
  esac
  hits=$(grep -rIl -F -e "$builddir" "$dir" 2>/dev/null)
  if [ -n "$hits" ]; then
    echo "$(basename "$p"): $(printf '%s\n' "$hits" | wc -l | tr -d ' ') installed text files contain $builddir:"
    printf '%s\n' "$hits" | sed "s|^$dir/|  |"
    grep -rIn -F -e "$builddir" "$dir" 2>/dev/null | sed "s|^$dir/|  |" | head -n 5
    bad=1
  else
    echo "$(basename "$p"): no installed text file contains $builddir"
  fi
done
exit $bad
