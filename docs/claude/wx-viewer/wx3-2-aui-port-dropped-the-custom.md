---
type: map
title: "The wx3.2 AUI port lost the ewxAUI caption buttons; `EVT_CHILD_FOCUS` on Builder now triggers `receiveFocus()`"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["src/apps/builder/EwxAuiCompat.H"]
issues: []
---
**wx3.2 AUI port dropped the custom "ewxAUI" pane-caption buttons
(take focus / pin / options / open) the original app was built
against** (`src/apps/builder/EwxAuiCompat.H` documents this), and
nothing replaced them as the trigger for `VizPropertyPanel::
receiveFocus()` — which is what activates essentially every 3-D
overlay in the viewer (vector/tensor arrows for dipole/quadrupole/
gradient, Mulliken charge coloring, geometry-trace and vibration-mode
animation, ...). Only the MOs panel had an independent workaround
(its own "Compute" button calls `setFocus(true)` directly); every
other `VizPropertyPanel` subclass was silently dead — correct
extraction, correct data, zero visual output, no error. Fixed by
binding `EVT_CHILD_FOCUS` on `Builder` (bubbles from any descendant
control to the top-level frame) and walking up to the owning
`VizPropertyPanel` — not `EVT_AUI_PANE_ACTIVATED`, which looks like
the obvious stock-wx3.2 replacement but only fires when the pane's
*own* bare window receives focus directly (`wxAuiManager::GetPane()`
requires an exact pointer match, no ancestor walk), never when focus
lands on a nested control inside it — the overwhelmingly common case.
If a new property panel's viz still doesn't show after this fix,
check whether it overrides `receiveFocus()`/`loseFocus()` at all
before assuming the trigger is broken again.
