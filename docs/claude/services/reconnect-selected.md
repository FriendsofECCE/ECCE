---
type: map
title: Organizer Reconnect Selected runs the single-job reconnect per eligible calculation
area: services
paths: [src/comm/commxt/JobStore.C, tests/launch/reconnect_race_test.py, include/comm/ReconnectRules.H, src/comm/commxt/JobCatchUp.C, src/apps/organizer/ReconnectSelected.C, src/apps/organizer/CalcMgr.C, tests/progress/reconnect_rules_test.C, tests/launch/reconnect_selected_test.py]
issues: [601, 599]
---
Run Mgmt > Reconnect Selected... (also the calculation context menu and the
multi-select Run Mgmt menu) takes the same selection as Launch Selected
(`CalcMgr::launchSelection()`: the project table's rows when several, else the
tree item). A selected project is not expanded; its table rows are.

Eligibility is `ReconnectRules::problem()` (wx-free, plain values in): only
Submitted, Running and Waiting qualify, and only with a machine registered
here, a job id and a run directory. Everything else is listed and skipped with
the reason. `JobCatchUp::plan()` fills the values from the calculation.

Each eligible row runs `JobCatchUp::reconnect()` -- the call behind the
single Reconnect -- on the GUI thread, one after the other, rows of one
machine together. A non-Waiting row first gets `ecce_ejs_kill`, as the single
path does. The Organizer opens its own login in `Launch::validateRemoteLogin`
for each job; with `ECCE_SSH_CONTROL_DIR` the first one makes the master and
the later ones reuse it, and the job stores that `startJobStore` spawns meet
on the per-host lock (#599, owned-ssh-master.md). Without an owned master
(libssh, Windows) the later jobs rely on AuthCache having the password.

Hooks: `reconnectselectedlist URL...` (what the dialog shows, `;`-joined
`name|machine|state|ok or reason`), `reconnectselectedshot PNG URL...`,
`reconnectselected URL...`.

**Pitfall: the old store must not touch the remote on `ecce_ejs_kill`
(#601).** The per-host login lock (and the ssh master behind it) is per
session, so a store left over from an earlier session logs in again by its
own means when it runs remote commands, and its cleanup's `rm -f
eccejobmonitor eccejobmonitor.conf ...` can land in the middle of the new
store's copy of those files. On `ecce_ejs_kill` a store therefore only closes
its own stream (`gKilledLocally` in `JobStore.C`): no Ctrl-C, no remote `rm`,
no state change and no `WaitingJobs::remove` (the new store owns them). The
old monitor ends on SIGPIPE at its next write.

Both Reconnect paths then wait in `CalcMgr::stopOldMonitor` (at most 15 s,
100 ms steps) until `WaitingJobs::watched(url)` is false -- the old
`eccejobmaster` holds that flock for its store's life -- and start nothing on
timeout ("previous monitor still running"). A Waiting row has no old store and
skips the wait.

A monitor FATAL before the store has seen any job status (missing or
half-copied configuration, Init errors) goes to `restartSystem()`: the job is
parked as Waiting (exit 4), never Failed. FATALs after the first status keep
`restart()`.

Hooks: `reconnectjob URL` (the single Reconnect; answers `reconnected: ...`,
`failed: ...` or `previous monitor still running`);
`ECCE_TEST_EJS_KILL_DELAY_MS` delays the old store's exit. Test:
`tests/launch/reconnect_race_test.py`.
