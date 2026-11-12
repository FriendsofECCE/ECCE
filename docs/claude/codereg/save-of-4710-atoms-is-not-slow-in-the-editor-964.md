---
type: map
title: "Where a Save of a 4710-atom Vina calculation spends its time (#964)"
area: codereg
paths: [src/apps/calced/CalcEd.C, src/dsm/edsiimpl/DeckWriter.C, include/dsm/DeckWriter.H, tests/apps/vina_save_timing_test.py]
issues: [964, 940]
---
`ECCE_SAVE_TIMING=1` makes the editor print the milliseconds of each Save
step to stderr (`DeckWriter::StepTimer`). With the 4710-atom 1IEP entry
(`tests/dock/fixtures/1iep.pdb`, local data, `apps_vina_save_timing`) the
editor's whole `doSave` takes about 0.26 s: store settings 38, fetch + write
`.frag` 98, `.param` 27, `ai.vina` (also writes `vina.vinafrag`) 50, store
deck and `vina.vinafrag` about 1 each, `verifyInput` 39, state/launch/notify
under 1. No step scales badly with atom count, so nothing was changed there.

So a Save that takes much longer on this size is not in these steps on a local
install. What is left is outside the measurement: WebDAV storage against a
central data server, process starts on Windows (#940), and other apps
reacting to the `ecce_url_*` messages the Save publishes (the Builder
subscribes to `ecce_url_details` and `ecce_url_state`, the Organizer to all of
them). Run the test against a central server with `ECCE_SAVE_TIMING=1`
before changing anything else; the stage names the dialog shows
("Storing the structure and settings", "Writing the structure and basis
set", "Running the ... input generator", "Storing vina.vinafrag", "Checking the
input file") mark where a long step sits.
