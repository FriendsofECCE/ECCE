---
type: pitfall
title: "`wxGrid::MakeCellVisible()` before the pane is laid out does nothing: `AutoSize()` has made every row visible"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
issues: []
paths: [src/apps/builder/MoPanel.C, src/apps/builder/Builder.C]
---
A property panel fills its grid in `Create()`/`initialize()`, while it is
still off screen. `wxGrid::AutoSize()` then sizes the grid window to hold
every row (benzene's 114 MOs: a 2845 px tall grid window), so
`MakeCellVisible(row, 0)` finds the row already visible and leaves the
scroll position at 0. When the AUI pane later cuts the grid down to a few
rows, it shows the top of the table. In the MOs panel that is the highest
virtual orbital, with the selected HOMO far out of view: the selection was
right (row and MO number), only the scroll was lost.

Scroll after layout, not at selection time: `MoPanel::selectMo()` sets a
pending flag and `CallAfter`s `scrollToSelection()`, which also runs from
the grid window's `wxEVT_SIZE`; it acts only once the grid is on screen and
shorter than its rows (`CellToRect` of the last row), centres the row and
clears the flag. `wxEVT_SHOW` is not a substitute: the grid reports shown
(`IsShown()` 1) long before it is on screen (`IsShownOnScreen()` 0).

Measured headlessly with the Builder scene commands `motable` (selected row,
`IsVisible`, scroll offset, the HOMO from ORBOCC) and `hold`. Any other
panel that selects a grid row during `initialize()` has the same shape.
