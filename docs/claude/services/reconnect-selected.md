---
type: map
title: Organizer Reconnect Selected runs the single-job reconnect per eligible calculation
area: services
paths: [include/comm/ReconnectRules.H, src/comm/commxt/JobCatchUp.C, src/apps/organizer/ReconnectSelected.C, src/apps/organizer/CalcMgr.C, tests/progress/reconnect_rules_test.C, tests/launch/reconnect_selected_test.py]
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
