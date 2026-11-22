---
type: pitfall
title: A WxFeedback message with flush greys every window, the whole main frame in one-process mode
area: services
paths: [src/wxgui/wxtools/WxFeedback.C, src/apps/calced/SetupArea.C, src/apps/launcher/LaunchPanel.C]
issues: [1266]
---
`WxFeedback::setMessage(..., flush=true)` calls `wxSafeYield()`, which puts a
`wxWindowDisabler` over every top-level window for the length of the yield.
With tools as tabs that window is the main frame, so each flushed message
greys the whole tab and brings it back: a launch that logged one line per
step flashed the editor about eight times.

Messages written while something already yields (the launch loop in
`LaunchPanel::runLaunchStep` pumps every 20 ms) need no flush. Where a message
must be drawn in the middle of synchronous work, use
`wxTheApp->YieldFor(wxEVT_CATEGORY_UI)` or `Update()` on the box rather than
`wxSafeYield`. Test: `tests/apps/one_process_test.py --launcher-only`
(`setup launch get greyed` must be 0).
