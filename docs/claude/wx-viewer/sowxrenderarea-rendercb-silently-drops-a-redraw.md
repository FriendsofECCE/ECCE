---
type: map
title: "`SoWxRenderArea::renderCB` silently drops a redraw"
area: wx-viewer
section: ""
issues: [99]
---
**`SoWxRenderArea::renderCB` silently drops a redraw** if Inventor's
scene-graph-touch notification fires while a paint is already in
flight (`p_inPaint`), with no retry — the sensor has already fired
and won't fire again on its own. Manifests as geometry-trace/
vibration step-through updating unreliably (works for the first
step or two, then stops) and looped animation never visibly
animating at all, while whatever reads the same step data via a pull
model (e.g. the atom table) stays perfectly in sync — a strong tell
that a symptom is this bug rather than a data problem. Fixed with a
pending-redraw flag (`p_redrawPending`) that `OnPaint()` checks and
acts on once the in-flight paint finishes.
**UPDATE 2026-09-22: that fix is correct but was not the cause of
#99.** The `p_redrawPending` retry path works; it simply never had a
callback to service, because the scene manager's redraw sensor is a
one-shot that re-arms on render and nothing re-armed it. See the
one-shot entry in the pitfall list above. Do not re-investigate
`p_inPaint`/`p_redrawPending` for a "viewer stops updating" symptom
without first checking whether `renderCB` is entered at all.
