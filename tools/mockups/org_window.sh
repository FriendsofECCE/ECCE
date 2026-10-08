#!/bin/bash
# MOCK-UP (wip/org-icons, #210): snap the real Organizer headlessly.
#   org_window.sh OUT.png light|dark old|new   (needs the worktree build-cmake/organizer
#   and an installed /opt/ecce for the rest of the data tree)
set -e
out=$1 theme=$2 mode=$3
here=$(cd "$(dirname "$0")/../.." && pwd)
w=$(mktemp -d /tmp/claude-1000/orgwin.XXXX)
H=$w/home; mkdir -p $H/data/client $H/bin
for f in /opt/ecce/*; do [ "$(basename $f)" = data ] || [ "$(basename $f)" = bin ] || ln -s $f $H/; done
for f in /opt/ecce/bin/*; do ln -s $f $H/bin/; done
rm $H/bin/organizer; cp -l $here/build-cmake/organizer $H/bin/organizer 2>/dev/null || cp $here/build-cmake/organizer $H/bin/organizer
for f in /opt/ecce/data/*; do [ "$(basename $f)" = client ] || ln -s $f $H/data/; done
for f in /opt/ecce/data/client/*; do [ "$(basename $f)" = pixmaps ] || ln -s $f $H/data/client/; done
ln -s $here/data/client/pixmaps $H/data/client/pixmaps
data=$w/ldata; proj=$data/users/local/summary-test; mkdir -p $proj
cp $here/tools/screenshots/data/project.ecce-meta $proj/.ecce-meta
cp -r $here/tests/apps/fixtures/calc-water-opt $proj/water-opt
printf 'summary file://%s\nsnap %s\n' $proj/water-opt $out > $w/cmds
export GTK_THEME=Adwaita; [ "$theme" = dark ] && export GTK_THEME=Adwaita:dark
Xvfb :78 -screen 0 1400x900x24 >/dev/null 2>&1 & xp=$!; sleep 2
cd $w
DISPLAY=:78 ECCE_HOME=$H ECCE_LOCAL_DATA=$data ECCE_TEST_ORGANIZER=$w/cmds ECCE_ORGANIZER_OPEN=file://$proj/water-opt \
  ECCE_REALUSER=$(id -un) ECCE_REALUSERHOME=$w ECCE_NWCHEM_DATA=/nonexistent HOME=$w timeout 90 $H/bin/organizer > $w/log 2>&1 &
for i in $(seq 60); do [ -s $out ] && break; sleep 1; done; sleep 1
kill $xp 2>/dev/null; pkill -f "$H/bin/organizer" || true
tail -5 $w/log; ls -la $out
