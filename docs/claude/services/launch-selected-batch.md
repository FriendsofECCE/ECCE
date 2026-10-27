---
type: map
title: Organizer Launch Selected submits through LaunchBatch, not the Launcher window
area: services
paths: [include/comm/LaunchBatch.H, src/comm/commxt/LaunchBatch.C, src/apps/organizer/LaunchSelected.C, tests/launch/launch_selected_test.py]
issues: [492, 516]
---
`LaunchBatch` is the wx-free copy of what `WxLauncher::saveJob/buildArgs`
do from a calculation's stored `launch_*` values; keep it in step with them.
A launch check (`Launch::launchWarning`) only appears while staging, so the
summary dialog shows static checks (machine registered, queue, cores, wall
time) and a check's text goes into the result list after the submit; the
batch never stops for it. Steps run on a worker thread as in
`WxLauncher::runLaunchStep`, so `CalcMgr::getAuthorization` forwards a login
to the GUI thread. Test hook: `launchselected URL...`.

Which calculations: `CalcMgr::launchSelection()` takes the project table's
rows when more than one is selected, because the Run Mgmt menu can fire while
`p_focusOnTree` still points at the tree and `getSelection()` then answers
with the single tree item. A wall time is required only where a queue manager
takes one (`LaunchBatch::wallTimeRequired`); a direct "Shell" machine such as
localhost is registered with Q:TL but passes no limit. Hook:
`launchselectionrows URL...`.

A failed submit (Launcher or batch) stays Ready, but `LaunchBatch::recordFailure`
stores the message in `runStatusReason` and appends it to the run log
(`TaskJob::appendJobLogCreating`, which starts the log when the calculation
has never run); `clearFailure` removes the reason after a successful submit.
The Organizer status line shows the reason for Ready calculations too.

Launch on: the summary's "Launch on:" choice (and `on=MACHINE`,
`profile=NAME` before the URLs in the hooks) makes `LaunchBatch::plan` carry
each calculation's settings over (`carryOver`): cores, nodes, memory and wall
time cut to the machine's and queue's limits, a queue the machine lacks
becomes its default. A Shell-manager machine lists no queues, so nothing is
cut or replaced there. `storeTarget` writes the changed settings just before
the submit, so a submit that then fails (or a staged machine) still leaves
them stored. `launchjob launchdata URL` prints them.
