---
type: pitfall
title: "The Properties list keeps eight rows under an open panel; a tall panel's opening height must not squeeze it"
area: wx-viewer
paths: ["src/apps/builder/BuilderPanels.C", "tests/apps/panel_layouts_test.py", "src/apps/builder/PropertyPanel.H", "tests/apps/property_list_click_test.py"]
issues: [1022]
---
In the list + detail layout `sizeIndexPane()` sets the Properties list's
height from the open panel: the panel's `openingHeight()` comes first and
the list takes what is left. It used to give way down to three rows, so a
panel with a large opening height (Diffraction, 450 px, docked since
#1013) left a 700-768 px screen four list entries and the others could not
be clicked (`apps_property_list_click` failed).

The list now keeps `INDEX_ROWS_KEPT` (8) rows, or as many as fit above
what the panel needs: its `minimumHeight()` floor, or for a panel with a
table (MOs) its controls plus three table rows (`tableRowsHeight`,
measured once the panel is laid out). Without that the MOs table in the
capped 1092x560 Builder showed 0 rows (`apps_panel_layouts`). The panel
gets the rest and shrinks below its opening height. A new panel that needs more room
than its floor should scroll or shrink its plot, not raise
`openingHeight()` to claim the list's rows.
