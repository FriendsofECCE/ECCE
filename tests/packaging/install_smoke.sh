#!/bin/bash
# Install an ECCE package in a clean container, start the data server as an
# ordinary user and check that it answers WebDAV. Catches missing
# dependencies and distro differences (apache2 vs httpd) before a user does.
#   tests/packaging/install_smoke.sh <package.deb|package.rpm> <image>
# Needs rootless podman. Exit status 0 means the data server answered.
set -u
pkg=$(realpath "$1"); image=$2
case $pkg in
  *.deb) install="apt-get -qq update && DEBIAN_FRONTEND=noninteractive apt-get -qq -y install curl /pkg/$(basename $pkg)" ;;
  *.rpm) install="dnf -q -y install epel-release && dnf -q -y install curl-minimal /pkg/$(basename $pkg)" ;;
  *) echo "unknown package type: $pkg"; exit 2 ;;
esac
podman run --rm -v "$(dirname $pkg)":/pkg:ro,Z "$image" bash -c "
  set -e
  { $install ; } > /tmp/install.log 2>&1 || { tail -30 /tmp/install.log; echo 'FAIL: install'; exit 1; }
  echo 'ok: installed'
  useradd -m tester
  su - tester -c 'ecce-dataserver-start' > /tmp/start.log 2>&1 || { cat /tmp/start.log; tail -20 /home/tester/.ECCE/dataserver/logs/error_log 2>/dev/null; echo 'FAIL: ecce-dataserver-start'; exit 1; }
  echo 'ok: ecce-dataserver-start'
  code=\$(curl -s -o /tmp/prop.xml -w '%{http_code}' -X PROPFIND -H 'Depth: 1' http://localhost:8096/Ecce/ || true)
  if [ \"\$code\" != 207 ]; then
    echo \"FAIL: PROPFIND answered \$code\"; cat /tmp/start.log
    find /home/tester -name 'error_log*' -exec tail -20 {} \; 2>/dev/null
    exit 1
  fi
  echo 'ok: PROPFIND 207'
"
