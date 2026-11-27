#!/bin/bash
# Detach a mounted dmg.  A fresh volume can still be busy (Spotlight), and
# hdiutil then exits 16 with nothing in the log (#1336): say what is being
# done, retry, then force.
mnt=$1
for i in 1 2 3 4 5; do
  echo "hdiutil detach $mnt (try $i)"
  hdiutil detach "$mnt" -quiet && exit 0
  sleep 3
done
echo "hdiutil detach $mnt still busy: forcing"
hdiutil detach "$mnt" -force -quiet || { echo "hdiutil detach -force $mnt failed rc=$?"; exit 1; }
