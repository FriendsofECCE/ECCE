---
type: map
title: "`ecce_url_progress`: the job store relays progress on the owner's own topic and stores the last snapshot as `ecce:progress` (#284)"
area: services
section: "Pitfalls"
paths: ["src/comm/commxt/JobStore.C", "data/client/config/ecce_messages", "include/util/JobProgress.H", "src/util/genutil/JobProgress.C", "src/apps/organizer/CalcMgrApp.C", "src/apps/organizer/CalcMgr.C", "tests/apps/progress_backfill_test.py", "src/apps/organizer/CalculationContextPanel.C", "src/apps/organizer/ProgressBox.C", "tests/launch/launchjob.C", "tests/launch/ecceqm_test.py", "tests/apps/progress_box_test.py", "tests/progress/progress_test.C"]
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
  the time left, the box only). Once the state is completed or later it keeps
  the same blocks and plots (`running(age, finished=true)`: no time left, no
  "updated") and adds `JobProgress::summary()` below them. A progress message never queues a tree or summary
  update (`counts` stays 0) and never rebuilds the panel (#254); only a
  change of the box's size re-lays out the panel.
- **No wx in the rules.** `JobProgress` (util) parses the text and makes
  the lines; `tests/progress/progress_test.C` (ctest `progress`) checks
  them; the box only paints them in system colours and `PlotPalette`.
- **Contact.** The same topic carries the job store's out-of-contact
  notice, property `contact` (see out-of-contact-284.md); its `progress`
  may be empty and then must not clear the shown snapshot.
- **Tests.** `launchjob listen <topic> <s> <file>` records messages as a
  subscriber; `launchjob progress <url>` prints the stored snapshot.
  Organizer hook commands `progress <url> <file>` (inject as the broker
  does) and `progressbox` (what the box draws).
- **Backfill.** A finished calculation with an output and no `ecce:progress`
  (run before the record existed, or imported by an older ECCE) gets one when
  the summary is first shown: `CalcMgr::backfillProgress` copies the stored
  parse and primary outputs to temp files and runs `eccejobmonitor` in post
  mode (`-parseTypes NONE -progress 1`, stdin closed) over them with the
  code's `.desc`, as a `wxProcess`; the last `jmPROG` of its message file is
  stored with `addProps` and shown (age 0, so no "updated"). Once per
  calculation per session, and nothing is stored for a code with no
  `[PROGRESS]` entries or no stored stdout (an imported NWChem `.eprint`).
  `ECCE_NO_PROGRESS_BACKFILL=1` turns it off. The monitor reads stdin in
  `stdio` mode and hangs on an open one.
