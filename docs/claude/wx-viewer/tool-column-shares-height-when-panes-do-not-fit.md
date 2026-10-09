---
type: pitfall
title: "Tool panes that do not fit the right-hand column get no height and are drawn over the viewer; fitToolColumn shrinks their floors and their controls scroll"
area: wx-viewer
paths: [src/apps/builder/BuilderPanels.C, src/apps/builder/Builder.C, src/apps/builder/Builder.H, tests/apps/tool_column_test.py]
issues: []
---
The tools a calculation not yet run opens with (Open structures, Build,
Coordinates or Selection, Symmetry, Log) need more height than the capped
Builder window has on 1024x600 up to 1920x1080 screens. wxAUI lays out the
panes that fit at their `min_size` and gives the rest height 0. wxGTK keeps
a window at least its own min size (`SetSizeHints` in each panel's
`Create`), and AUI does not move a pane it gave no room. The window
then stays where it was created, so Symmetry was drawn over the viewer.
`makeRoomFor` only handles a pane opened from the Tools menu.

`addToolPanel` moves each tool panel's controls into a vertically scrolled
child (`scrollToolContent`; lists, tables and the Periodic Builder already
scroll) and records the pane's floor in `p_paneFloor`. After every layout
(`updatePanes`, and after a frame resize via `OnSize`), `fitToolColumn`
compares the sum of the right-hand column's floors with the height its
panes got. If they do not fit, it scales every floor by the same factor
(at least 40 px). If they do, it puts the floors back. `paneInfoForSave`
saves the full floor. `contentFixedHeight` looks through the scrolled
child for the content height.

Test: `tests/apps/tool_column_test.py` (`apps_tool_column`).
