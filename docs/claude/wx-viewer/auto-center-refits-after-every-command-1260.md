---
type: pitfall
title: "Auto Center re-fits the camera after every Builder command not on its exempt list: a selection click zoomed out (#1260)"
area: wx-viewer
paths: [src/apps/builder/Builder.C]
issues: [1260, 409]
---
With Auto Center on (the default), `Builder::execute()` calls `center()`
(View All) after each command except an explicit list. Display commands were
on it, selection commands were not, so a click on empty space in Select
mode (`UnselectCmd`) re-fitted the camera after the user had zoomed in.
`SelectCmd`, `UnselectCmd` and `SelectAllCmd` are now exempt. A new command
that changes no coordinates belongs on that list too. Undo/Redo still
centre unconditionally (`OnUndoClick`/`OnRedoClick`).
