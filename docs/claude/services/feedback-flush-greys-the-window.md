---
type: pitfall
title: A WxFeedback message with flush greys every window, the whole main frame in one-process mode
area: services
paths: [src/wxgui/wxtools/WxFeedback.C, src/apps/calced/SetupArea.C, src/apps/launcher/LaunchPanel.C]
issues: [1266, 1268]
---
`WxFeedback::setMessage(..., flush=true)` calls `wxSafeYield()`, which puts a
`wxWindowDisabler` over every top-level window for the length of the yield.
With tools as tabs that window is the main frame, so each flushed message
greys the whole tab and brings it back: a launch that logged one line per
step flashed the editor about eight times.

Messages written while something already yields (the launch loop in
`LaunchPanel::runLaunchStep` pumps every 20 ms) need no flush. Where a message
must be drawn in the middle of synchronous work, use
`wxEventLoopBase::GetActive()->YieldFor(wxEVT_CATEGORY_UI)` or `Update()` on the box rather than
`wxSafeYield`. Test: `tests/apps/one_process_test.py --launcher-only`
(`setup launch get greyed` must be 0).

Since #1268 the flush itself is `Update()` + the active loop's `YieldFor(wxEVT_CATEGORY_UI)` (wxApp has no YieldFor)
after the text is written, so a flushed message no longer greys anything;
`greywatch on` / `greywatch` count the main window's disables for any step.
