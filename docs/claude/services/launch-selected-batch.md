---
type: map
title: Organizer Launch Selected submits through LaunchBatch, not the Launcher window
area: services
paths: [include/comm/LaunchBatch.H, src/comm/commxt/LaunchBatch.C, src/apps/organizer/LaunchSelected.C, tests/launch/launch_selected_test.py]
issues: [492]
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
