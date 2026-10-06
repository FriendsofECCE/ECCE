---
type: pitfall
title: "The redraw sensor must not be drained inside `schedule()`: `SoWxEventHandler::setUpCallbacks()` only wakes idle"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
issues: [99, 166]
paths: ["src/inv/wxinv/SoWxEventHandler.C", "src/inv/wxinv/SoWxRenderArea.C", "src/apps/builder/GeomTracePropertyPanel.C", "src/apps/builder/NModePanel.C"]
---
**The redraw sensor must not be drained inside `schedule()`.**
`SoWxEventHandler::setUpCallbacks()` is the sensor manager's
changed-callback. It runs inside `SoDelayQueueSensor::schedule()` before
that sets `scheduled`; draining the queue there synchronously
(`ProcessEvent(wxIdleEvent)`) fired the redraw sensor and then left it
marked scheduled for good, so no later scene change asked for a render
(#99: a geometry trace moved the atoms once, then never again). It calls
`wxWakeUpIdle()` instead. The scene command `redraws` (six trace steps, no
forced paint) checks it: 6/6 renders on both viewer builds.

`SGViewer::refreshRenderArea()` at the end of `processStep()` in
`GeomTracePropertyPanel` and `NModePanel` forces a wx paint independently
of the sensor and stays as a second line of defence.

If a viewer stops updating while the data demonstrably changes,
instrument `renderCB` first: `ECCE_DEBUG_GEOMTRACE=1` prints `[RENDERCB]`
lines alongside the step trace, and their *absence* is the finding. The
render cache and the `p_redrawPending` retry in `OnPaint` are not the
cause (both were ruled out).
