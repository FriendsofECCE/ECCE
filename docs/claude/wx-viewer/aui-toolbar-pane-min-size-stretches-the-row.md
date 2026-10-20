---
type: pitfall
title: "A wxAUI toolbar pane with a MinSize is stretched: the row's spare width is shared among the toolbars"
area: wx-viewer
paths: [src/apps/builder/Builder.C]
issues: []
---
In a toolbar dock wxAUI (3.2) gives a fixed pane proportion 0, and so its best
width, only when the pane's `min_size` is `wxDefaultSize`. Any MinSize — even
one equal to the best size — leaves the pane its dock proportion, and the
row's spare width is divided among the toolbars: the Builder's File, View,
Style and Manipulator toolbars were each 241 px wide with their icons at the
left, which looks like large gaps between the groups.

`Builder::loadPaneLayout()` restores every pane from the saved layout (a fresh
start saves the default one first and reads it back), and it used to apply the
tool panels' content minimum width (at least `PANE_WIDTH_MIN`) to toolbar
panes too. It now resets a toolbar pane to its window's best size, no minimum,
and its creation index as Position. In a toolbar row Position is a pixel
offset that AUI only ever pushes right to stop overlaps, so ordinals pack the
bars and a saved offset would have left a gap.

Debug by dumping `pane.min_size`, `best_size`, `dock_pos` and `rect` of each
toolbar pane after the first layout; a rect wider than `best_size` is this.
