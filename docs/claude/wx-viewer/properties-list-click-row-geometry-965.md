---
type: map
title: What a click in the Builder's Properties list can be measured against
area: wx-viewer
paths:
  - src/apps/builder/PropertyIndexPanel.C
  - src/apps/builder/BuilderPanels.C
  - tests/apps/property_list_click_test.py
issues: [965]
---
The Properties list is a generic `wxTreeCtrl` (GTK has no native one) inside
a pane whose height `Builder::sizeIndexPane` sets from
`PropertyIndexPanel::preferredHeight`, capped at 40 % of the window and
less again when a detail panel is open.  With more panels than that cap it
scrolls, which is the state #965 was reported in.

**Measuring it.**  `ptree <name>` now writes, beside the shown panes and the
listing, one `row <x> <y> <w> <h> <text>` line per row in *screen* pixels
and a `treewin <x> <y> <w> <h> view <sx> <sy> unit <ux> <uy>` line for the
tree window itself (`PropertyIndexPanel::rowRects`).  That is what lets a
test click a row through the X server instead of through a hook.

Two traps when reading those numbers:

- A row's rectangle is not clipped to the tree.  On a scrolled list the
  rows above the top have a negative `y` and the rows below the bottom run
  past `treewin`'s height, so a click at such a rectangle lands on another
  widget entirely and nothing in the list is selected.  Only rows wholly
  inside the tree window can be clicked.
- A group heading carries no panel name, so a click on one opens or closes
  the group and leaves the selection empty.  A selection check has to skip
  the headings, which are the rows whose text is not one of the `entry`
  lines' panel names.

With both honoured, the hit test agrees with the reported geometry under
Xvfb: on a list scrolled by `EnsureVisible` and on one scrolled with the
mouse wheel, the row clicked is the row selected
(`tests/apps/property_list_click_test.py`).
