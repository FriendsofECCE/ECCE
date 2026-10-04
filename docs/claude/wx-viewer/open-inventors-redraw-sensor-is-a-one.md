---
type: map
title: "Open Inventor's redraw sensor is a ONE-SHOT that re-arms on render, and with a render callback installed nothing re-arms"
area: wx-viewer
section: ""
issues: [99]
---
**Open Inventor's redraw sensor is a ONE-SHOT that re-arms on render,
and with a render callback installed nothing re-arms it.** This was
#99: stepping a geometry trace moved the atoms once and then never
again. `GTStepCmd` ran every step with changing coordinates,
`SGFragment::getAtomCoordinates()` reads `TAtm` live so the scene
always had fresh data, `touchChemDisplay()` and `sgfrag->touch()` were
both called — and `SoWxRenderArea::renderCB` was entered for step 0
and then *not once* for the thirteen steps after it. The redraw was
never requested. Fixed by calling `SGViewer::refreshRenderArea()` at
the end of `processStep()`, which forces a wx paint and does not
depend on that sensor; applied to `GeomTracePropertyPanel` and
`NModePanel`. **Two plausible theories were disproved on the way and
should not be revisited**: the render cache (disabling caching
process-wide changed nothing) and a stranded `p_redrawPending` in
`OnPaint` (that retry path is fine — it never had a callback to
service). If a viewer stops updating while the data demonstrably
changes, instrument `renderCB` first: `ECCE_DEBUG_GEOMTRACE=1` prints
`[RENDERCB]` lines alongside the step trace, and their *absence* is
the finding.
