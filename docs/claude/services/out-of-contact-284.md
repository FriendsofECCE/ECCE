---
type: map
title: "Out of contact: silence from the job monitor is a display state (\"running?\"), never a final state (#284, #208)"
area: services
section: "Pitfalls"
paths: ["src/comm/commxt/JobStore.C", "src/apps/jobstore/eccejobmaster.C", "src/comm/commtools/RunMgmt.C", "include/comm/RunMgmt.H", "src/apps/organizer/CalcMgr.C", "src/apps/organizer/CalcMgrApp.C", "src/apps/organizer/ProgressBox.C", "src/apps/organizer/CalculationContextPanel.C", "src/apps/organizer/ProjectContextPanel.C", "src/wxgui/wxtools/WxState.C", "src/wxgui/wxtools/WxStateImageList.C", "src/wxgui/wxtools/WxResourceStateImageList.C", "src/wxgui/wxtools/WxResourceTreeCtrl.C", "src/wxgui/wxtools/GridCellPropImageRenderer.C", "src/wxgui/wxtools/GridPropElement.C", "data/client/config/ResourceDescriptor.xml", "data/client/config/ResourceDescriptorRxn.xml", "tests/launch/contact_test.py", "tests/apps/progress_contact_test.py", "tests/launch/launchjob.C"]
issues: [284, 208, 606]
---
**Detection is the job store's.** Only eccejobstore hears the monitor (jmUP
every sixth job check, one check per `timePauseReadLine`: about 12 s for a
local job, 60 s otherwise). jmUP is relayed only while the job is
Submitted: `contact` = `ok <epoch>` at most every 50 s, also stored, which
the queued box shows as "updated n s ago" (`CalcMgr::lastChecked`, #606);
older Organizers read anything but `lost ` as in contact). The Organizer cannot tell a
quiet job (a long SCF step, a code without `[PROGRESS]`) from a lost one.

- **Times** (`contactInit()`, from the local `eccejobmonitor.conf`): out of
  contact after `max(60, 3 heartbeats)` s of no bytes from the monitor;
  reconnect (the old READ_TIMEOUT) after `max(180, that + 120)` s.
  `ECCE_JOB_CONTACT_SECONDS` / `ECCE_JOB_RECONNECT_SECONDS` override both
  (tests; sites with slow schedulers). Monotonic clock: a suspended client
  does not count its sleep.
- **Notice**: `ecce_url_progress` with `contact` = `lost <epoch last heard>`
  or `ok`, plus the last snapshot as `progress` (may be empty: the Organizer
  then keeps the one it has, it must not "forget"). Stored as `ecce:contact`
  metadata (cleared on contact and by `progressClear()` at a new run, which
  also sends `ok`), so a restarted Organizer shows it.
- **Silence past the reconnect time** (`reconnectAfterSilence()`): the state
  is left as it is, the job recorded in WaitingJobs, exit 4. The restarted
  store keeps a running job running (only a non-running state goes back to
  submitted) and seeds "lost since" and the snapshot from the stored values.
  eccejobmaster exports `ECCE_JOB_TRIES_LEFT`; on the last try the store
  parks (Waiting) instead, since nothing watches after it. A silent run lasts
  at least the reconnect time, so the quick-failure rule never ends the loop
  right after one. Monitor died / protocol errors / login refused keep their
  old paths (park, restart).
- **Display** (Organizer, all from `CalcMgr::outOfContact()`, live notice else
  the stored value, only while the state is Running): `WxState::OUT_OF_CONTACT`
  (= NUMBER_OF_STATES, one past the last state in both image lists) is the
  amber ring, "running?"; the tree asks through
  `WxResourceTreeCtrl::setOutOfContactTest`; the table's State cell holds the
  text "running?" (renderer draws the ring) and the Progress cell
  `JobProgress::noContactText` in amber (`ProgressCellRenderer`, not in a
  selected row). The box greys the snapshot, says when the job was last
  heard from, and shows Reconnect / Check Job on Machine / Show Output So
  Far (the Tail window).
- **Check Job on Machine** (`RunMgmt::checkOnMachine`, also a Run Mgmt item
  in both ResourceDescriptor files): one script over ECCE's login: `ps -p`
  for Shell, the queue's `queryJobCommand` otherwise (id as a word, error
  lines ignored), output mtime/size by perl, `.ecce.status`, the machine's
  clock. It only reports; Reconnect (or the monitor reading on) sets the
  final state from the output. Output can arrive with CRLF: strip it.
- **The fold** is `Progress/Collapsed` in `wxorganizer.ini`, flushed at once.

Tests: `tests/launch/contact_test.py` (ctest `launch_contact`: SIGSTOP the
monitor, kill ecce-qm, reconnect), `tests/apps/progress_contact_test.py`
(`apps_progress_contact`; hook commands `contact`, `checklines`, `fold`,
`treestate`, `tablestates`), `launchjob checkjob|contact`.
