---
type: pitfall
title: "Builder Save is only refreshed by updateSave(); a wxGrid in a sizer needs its own minimum size or it is one column wide"
area: wx-viewer
paths: ["src/apps/builder/Builder.C", "src/apps/calced/GeomConstraints.C", "src/apps/builder/BuilderPanels.C", "tests/apps/builder_edit_test.py"]
issues: []
---
Save in the Builder reads the same `p_dirty` flag as the close prompt, but
the menu item and toolbar button change only when `updateSave()` runs.
`setDirty(true)` alone leaves Save disabled while closing still asks;
call `updateSave()` wherever the flag is set (`eventMCB`).

A `WxGridView` reports a one-column best size, so in a sizer it is one
column wide with a horizontal scroll bar. `GeomConstraints::setTableSize`
autosizes the columns and sets the minimum size from the column widths and
up to three rows.

The Open structures pane is a fixed pane: its height is its minimum size.
`updateContextPane()` sets it (three to eight rows) whenever the list
changes. The blank structure the Builder starts with is closed when the
first other structure opens (`p_startupContext`).

Test: `tests/apps/builder_edit_test.py`.
