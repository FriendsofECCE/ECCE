---
type: map
title: ecce-* request tools stop waiting when the Organizer's session lease is gone (exit 5)
area: services
paths: [include/util/SessionLease.H, src/util/genutil/SessionLease.C, src/apps/launch/ecce-launch.C, src/apps/launch/ecce-move.C, tests/launch/ecce_move_test.py]
issues: [816, 897]
---
Exit codes of ecce-launch/rerun/terminate/reconnect/fetch/move/annotate: 2 ECCE not
running, 3 busy with a launch batch, 4 no answer in time (may have been carried
out), 5 the Organizer ended before answering (#816). The tools arm a
`SessionLease::ProgramWatch("organizer")` before publishing: it records the pid
of the Organizer's session lease (`~/.ECCE/leases/<key>/organizer.<pid>`, the
kernel drops the lock however it ends) and, once the ack has arrived, `gone()`
rescans every second. No lease seen at arm time (no lease support such as
Windows, or the Organizer is not one of `$ECCE_HOME/bin`'s) means the old wait
(batch timeout) stays. Local files only: broker, topics and the central-server
path are untouched, the Organizer always runs on the user's own machine.
`ecce-workflow` ignores the ack and is not covered.
