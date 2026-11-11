---
type: pitfall
title: "The workflow folder's table was single-select and forced the stale cursor row; its boxes did not reach the tools (#951)"
area: wx-viewer
paths: [src/wxgui/wxtools/WxGridView.C, src/apps/organizer/SessionProjectContextPanel.C, src/apps/organizer/WorkflowPanel.C, src/apps/organizer/CalcMgr.C, tests/apps/organizer_table_selection_test.py]
issues: [951, 925, 931]
---
`SessionProjectContextPanel` (workflow, MD study, reaction study) turned the
table's multi-select off. `WxGridView::gridRangeSelect` then "corrected" a
Ctrl+click or a Shift block by selecting `getCursorRow()`, but in wx 3.2 the
cursor has not moved yet when RANGE_SELECTED arrives on a Ctrl+click and is
the anchor on a Shift+click: each click selected the previous row (row 2 for
a click on row 3), and Shift+click re-selected the anchor. The #925 fixes
were never reached, since they only apply in multi-select mode. The row
order (sorted, not creation order) was not the cause.

Now a workflow's table is multi-select like a project's; the single-select
tables (MD/reaction studies) select the row clicked.

Boxes: `WorkflowCanvas::select` sends all picked steps, with the panel as
event object; `CalcMgr::OnPanelSelection` selects the same rows in the table,
and a table selection sets the picked boxes (`WorkflowPanel::setPicked`).
The tools (Energy Calculator, Compare) read `getSelection()`, i.e. the table.
Shift+click on a box picks the boxes between the anchor and it, in box order.

The workflow view's Back to Table button is gone (#931): selecting the folder
in the tree leaves the view.

Tests send real X clicks with xdotool on the test's own Xvfb (hooks
`tablerow ROW`, `boxpoint STEP`, `toolselection`; answers also go to
`ECCE_TEST_ORGANIZER_ANSWERS` so a running test can read positions).
The `tableclick` hook calls the grid's handlers directly and did not see this.
