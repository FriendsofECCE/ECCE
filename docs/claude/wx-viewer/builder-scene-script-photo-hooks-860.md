---
type: map
title: "Photographing the Builder: scene-script hooks (measure, assigncharges, gtstep, panelfloat, panelsize, shot) and tools/screenshots/shots.py"
area: wx-viewer
paths: [src/apps/builder/BuilderShotHooks.C, src/apps/builder/BuilderPbcTest.C, src/apps/builder/BuilderTraceStress.C, src/wxviz/viewer/SceneScript.C, tools/screenshots/shots.py, tests/apps/shot_hooks_test.py, src/apps/builder/Builder.C]
issues: [860, 1056]
---
The scene script (`ECCE_VIEWER_SCENE`) has three layers: `SceneScript.C`
(select, style, labels, gtstep, snap), the Builder's extension lambda in
`Builder.C`, and the files it forwards to. Photo hooks live in
`BuilderShotHooks.C`, `BuilderPbcTest.C` (measure) and `BuilderTraceStress.C`.

- `measure dist|angle|torsion I J ..` takes 0-based atoms here (the
  `ECCE_BUILDER_SCRIPT` command of the same name is 1-based). It clears the
  selection afterwards and refreshes the status line; the label is drawn.
- `gtstep N` goes through the open Geometry Trace panel (`traceFrame`), so
  playback and plot marker follow; without the panel it steps the scene only.
- `assigncharges <panel>` presses Assign Charges of a `MullikenPanel` or
  `PartialCharges` panel and shows charge labels. `PartialCharges` uses the
  column the panel has chosen; only Mulliken is exercised by a test.
- `panelfloat`/`panelsize <panel> <w> <h>` size a pane.
- `shot NAME` writes `NAME.ready` and runs the event loop until `NAME.go`.
  A script with a `shot` line draws in the Builder's own canvas, as
  `ECCE_VIEWER_SCENE_HOLD` does. Without either, the script draws in a
  separate 480x480 "scene capture" frame at 0,0 with its own (black)
  background, which a screen photo shows over the Builder; with it open,
  the Builder's canvas also missed the first computed MO surface (#1056).

Do not photograph from inside the process: `screen` (a wxScreenDC blit)
returned the same stale picture after later changes, so the driver takes the
picture from outside while the script waits in `shot`. A floating panel is
a window of its own, hence `"capture": "screen"` in the shot list. The
driver crops the root window at 0,0 (the script sizes the Builder there)
rather than looking the window up: the lookup failed now and then.
