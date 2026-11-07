---
type: map
title: "Copy and move between trees (#890): TreeCopy through the EDSIs, verify before removing, ecce-copy"
area: services
paths: [src/dsm/edsiimpl/TreeCopy.C, include/dsm/TreeCopy.H, src/apps/organizer/CalcMgr.C, src/apps/launch/ecce-move.C, tests/launch/ssh_folder_launch_test.py]
issues: [890, 700]
---
`TreeCopy::copy` walks the source with `listCollection` and writes through the
target EDSI (`makeCollection`/`makeDataSet`, `putDataSet`, `putMetaData`);
files go through a temp file. Properties are copied except `DAV:*` and the
built-in `resourcetype`/`contenttype` (the target rebuilds them; the Ecce
namespace ones carry the calculation/project type). Dot files other than the
sidecar are not listed by the EDSIs and are not copied. `CalcMgr::copyAcrossTrees`
is the one entry for paste, drop and `moveForCommand` (verbs move/copy/rename):
#700 guard for a move, progress dialog after 0.5 s, `TreeCopy::verify`
(names, types, sizes) and only then `removeFromTree`. A failure or cancel runs
`removeTree` on the partial target. `ecce-copy` is `ecce-move.C` built with
`ECCE_COPY_COMMAND`; the Organizer sends an ack first because a transfer outlasts
the tool's 8 s ack window. Test hooks: `server xfer VERB SRC DEST`,
`server failafter N` (`ECCE_TEST_TREECOPY_FAIL_AFTER`).
