---
type: pitfall
title: "Read a wxGrid's selection from the grid, not from a set kept from range events; a pushed mouse handler must pass modifier clicks on"
area: wx-viewer
paths: ["src/wxgui/wxtools/WxGridView.C", "include/wxgui/WxGridView.H", "src/apps/organizer/ProjectContextPanel.C", "tests/apps/organizer_table_selection_test.py"]
issues: [925, 939]
---
In wx 3.2 `wxGridSelection::ExtendCurrentBlock` (Shift+click, Shift+arrow)
reports only the new block, with `Selecting()` true, also when the block
shrinks; the rows dropped never get a deselect event. A set of rows kept
from `EVT_GRID_RANGE_SELECT` therefore keeps rows that are no longer
selected. `WxGridView::getSelectedRows()` reads `GetSelectedRowBlocks()`.
The grid has already updated its selection when `RANGE_SELECTED` arrives.

`GridEvtHandler` is pushed on the grid window and sees a press before
wxGrid. A press it does not `Skip()` (the start of a drag on a selected
row) never reaches wxGrid, and its left-up then selects that row alone.
Only an unmodified press may be held back; Shift and Ctrl go to wxGrid.

The cursor is the anchor of the next Shift+click. Code that rebuilds the
table and restores the selection (`selectResources`, auto-refresh #889)
sets the cursor on the first restored row; otherwise it stays at row 0.
`WxGridView::setCursor` also selects the row (`gridSelectCell`), and a
rebuilt grid selects its cursor row, so `ProjectContextPanel::refresh`
sets the selection explicitly (empty included) and puts the anchor back
with `moveCursor`, which leaves the selection alone (#939).

Hook: `tableclick ROW [ctrl] [shift]` sends a left down/up through the
grid window's handlers and answers the selected URLs.
