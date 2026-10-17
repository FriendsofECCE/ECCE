---
type: pitfall
title: "Auto-centre re-fitted the view after every property command: an MO compute zoomed in and out"
area: wx-viewer
paths: [src/apps/builder/Builder.C, src/apps/builder/MoPanel.C]
issues: [360]
---
`Builder::execute` calls `center()` (view all) after each command while
auto-centre is on. An MO compute runs two commands, ComputeMoCmd and then
IsoSurfaceCmd; the first leaves the bare molecule, so the view zoomed in,
then out again to fit the lobes. Commands derived from PropCmd (MO grid and
surface, iso value, surface type, vectors, trace and normal-mode steps,
ShowFrag) no longer re-centre; the panels that want a re-fit (trajectory,
normal modes) call it themselves.
