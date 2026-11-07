---
type: map
title: ecce-move / ecce-rename go through CalcMgr::moveForCommand; no offline mode
area: services
paths: [src/apps/launch/ecce-move.C, src/apps/organizer/CalcMgr.C, tests/launch/ecce_move_test.py, src/apps/launch/ecce-rerun.C, tests/launch/ecce_rerun_test.py]
issues: [824, 825, 700]
---
One source builds both tools (`ECCE_RENAME_COMMAND` selects ecce-rename). The
request `ecce_move_request` (verb, second, urls) is answered on
`ecce_move_reply` with `name\tok\tURL` / `name\tno\treason` lines.
`moveForCommand` uses `Resource::move`, then refreshes the old parent and the
target in the tree and calls `notifyRename`. The #700 refusal text is
`activeJobRefusal` (the dialog variant `refuseMoveOfActiveJob` calls it). The
name clash is checked against the target's fresh children before the move.
Unlike ecce-annotate there is no offline path: a move without the Organizer
leaves stale tree nodes, so no session means exit 2. A target in another tree goes through `copyAcrossTrees` (#890).

`ecce-rerun` uses the same request/reply pattern on `ecce_rerun_request` /
`ecce_rerun_reply`. `CalcMgr::rerunForCommand` takes the first free `_r2`,
`_r3`, ... name and calls `duplicateForRerun`, shared with New from This >
Rerun (`duplicate` then `TaskJob::resetForRerun`). Optional submission uses
`launchForCommand` on the copy; a failed launch identifies the retained copy.
Active jobs are refused before copying. No offline path; exit codes match move.
