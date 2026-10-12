---
type: pitfall
title: "A property pane that gains controls needs Builder::propertyPaneRoom: its table is squeezed to 0 px by the sizer, and the table scroll resets on every relayout (#285)"
area: wx-viewer
paths: [src/apps/builder/MoPanel.C, src/apps/builder/BuilderPanels.C, tests/apps/orca_mo_test.py]
issues: [285]
---
A wxBoxSizer shrinks a proportional item (the MO table) below its minimum,
to 0, when the window is shorter than the sum of the minimums; the pane floor
is 80 px, so the ESP colour bar pushed the table out of sight. `MoPanel::reflow()`
asks `Builder::propertyPaneRoom(panel, need)` for the height with the table at
four rows; the pane then takes the column's spare height (proportion 300000) from
the Properties list, which keeps its own floor, and returns it when the bar
goes. A relayout also resets the grid scroll, so reflow re-arms
`scrollToSelection`. The composition line describes an MO and is hidden for
a density or ESP surface. A 1366x768 Builder (563 px client) has no room for
both; `orca_mo_test.py` checks the ESP case on a 1700x1000 screen with
ECCE_PANEL_FULLSCREEN (the window size is otherwise saved by earlier runs).
