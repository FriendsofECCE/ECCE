---
type: map
title: "`ecce_url_progress`: the job store relays progress on the owner's own topic and stores the last snapshot as `ecce:progress` (#284)"
area: services
section: "Pitfalls"
paths: ["src/comm/commxt/JobStore.C", "data/client/config/ecce_messages", "include/util/JobProgress.H", "src/util/genutil/JobProgress.C", "src/apps/organizer/CalcMgrApp.C", "src/apps/organizer/CalcMgr.C", "src/apps/organizer/CalculationContextPanel.C", "src/apps/organizer/ProgressBox.C", "tests/launch/launchjob.C", "tests/launch/ecceqm_test.py", "tests/apps/progress_box_test.py", "tests/progress/progress_test.C"]
issues: [284, 213, 254]
---
**Path.** monitor `jmPROG` -> `JobStore::interactProgress` (adds
`at=<epoch>` to the `# seq= sent=` header) -> broker `ecce_url_progress`,
properties `url` and `progress` (the snapshot text, `\n`-separated; empty
= forget, sent when a new run starts) -> `CalcMgrApp::msgProgressMCB` ->
`CalcMgr::progressUpdate` (newest per URL in memory) -> the shown
`CalculationContextPanel::updateProgress` -> `ProgressBox::set`.

- **Topic.** `FILTER BY: NONE` in `ecce_messages`, so
  `ecce/<user>/ecce_url_progress`: every session of the owner, nobody
  else's (central and shared brokers' ACL `ecce/%u/#`). The job store runs
  on the client in all three deployment modes and publishes with that
  session's broker login; nothing on the server changes.
- **Stored, not a property.** `ecce:progress` metadata on the calculation
  (`addProps`, the same as `runStatusReason`): at most every 10 s, once in
  `cleanup()` *before* the final state is set (the Organizer reads it when
  the state notice arrives), cleared when a run starts (not on a job store
  restart). It arrives with the calculation's PROPFIND/sidecar, so a
  restarted Organizer shows where the job got to with no extra request;
  about 0.5-1.5 kB per launched calculation. Nothing goes to `Props/`.
- **Display.** The box sits after the Launcher row, is hidden for a
  calculation not yet submitted or without a snapshot, shows the live
  lines while running (repainted once a second for "updated n s ago" and
  the time left, the box only) and `JobProgress::summary()` once the state
  is completed or later. A progress message never queues a tree or summary
  update (`counts` stays 0) and never rebuilds the panel (#254); only a
  change of the box's size re-lays out the panel.
- **No wx in the rules.** `JobProgress` (util) parses the text and makes
  the lines; `tests/progress/progress_test.C` (ctest `progress`) checks
  them; the box only paints them in system colours and `PlotPalette`.
- **Tests.** `launchjob listen <topic> <s> <file>` records messages as a
  subscriber; `launchjob progress <url>` prints the stored snapshot.
  Organizer hook commands `progress <url> <file>` (inject as the broker
  does) and `progressbox` (what the box draws).
