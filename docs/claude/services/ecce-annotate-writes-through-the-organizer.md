---
type: map
title: ecce-annotate goes through the Organizer, else FileEDSI; never a hand-edited .ecce-meta
area: services
paths: [src/apps/launch/ecce-annotate.C, src/apps/organizer/CalcMgr.C, src/dsm/edsiimpl/FileEDSI.C, tests/launch/ecce_launch_test.py, tests/launch/launchjob.C]
issues: [742]
---
`ecce-annotate` publishes `ecce_annotate_request` like ecce-launch; the Organizer
(`CalcMgr::annotateForCommand`) does what `processEditCompletion` does:
`addProp`, `updateUrl`, `notifyProperty`. Any tree node works, so MD study
tasks need no special case. With no session and a local data folder the command
calls `addProp` itself (FileEDSI: `DirLock` on `.ecce-meta.lock`, whole-file
rename); a data server is refused because the login lives in the Organizer.
A direct write to `.ecce-meta` without the lock can lose a monitor's state or
progress update, since the file holds every sibling's properties.
`launchjob create|setup|mdsetup annotation=TEXT` (and an `annotation:` line in
the setup `.param`) store the same property.
