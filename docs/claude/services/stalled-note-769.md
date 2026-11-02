---
type: map
title: "Stalled (#769): a note on a running job whose output stopped growing, judged by the job store from the monitor's idle= and the step rhythm"
area: services
section: "Pitfalls"
paths: ["scripts/eccejobmonitor", "src/comm/commxt/JobStore.C", "include/util/StalledRule.H", "include/util/JobProgress.H", "src/util/genutil/JobProgress.C", "src/apps/organizer/ProgressBox.C", "src/apps/organizer/CalcMgr.C", "tests/launch/contact_test.py", "tests/jobstore/testStalledRule.C"]
issues: [769, 284]
---
**Not a run state.** The job stays Running; nothing is blocked or terminated.

- **Monitor.** `$gLastOut` is the time of the last line read. While the
  output is silent for 60 s the monitor marks the snapshot dirty once a minute
  and the `# seq= sent=` header carries `idle=<s>` (0 once a line arrives).
  Only a code with `[PROGRESS]` entries sends snapshots, so only such codes
  are judged.
- **Job store.** `interactProgress` counts a step each time the `step=`,
  `cycle=` or `iter=` values of the stage lines change, and asks
  `StalledRule`: stalled when idle >= max(3 x median step gap, floor 30 min);
  fewer than 3 steps: 2 h. `ECCE_JOB_STALL_SECONDS` and
  `ECCE_JOB_STALL_FEWSTEPS_SECONDS` override the two. Out of contact is never
  stalled (the note is stripped from the snapshot when contact is lost).
  One log line when it becomes stalled, one when output resumes. The note
  travels as `stall=<s>` in the snapshot header (relayed and stored as
  `ecce:progress`), so a restarted Organizer shows it.
- **Display.** `JobProgress::stalled()`; the Progress cell and a "Status"
  block of the box show "Running (stalled: no output for 2 h 10 min)". The
  State column's icon is unchanged. CLI: `launchjob status`.
- Tests: `tests/jobstore` (rule), `launch_contact` case d (ecce-qm alone
  stopped).
