---
type: pitfall
title: "A tool pane opened into a full right-hand column is laid out with no height and its window floats over the viewer"
area: wx-viewer
paths: [src/apps/builder/BuilderPanels.C, src/apps/builder/PBCGUI.C, src/apps/builder/BuilderPbcTest.C, tests/apps/pbc_ux_test.py]
issues: []
---
AUI gives a docked pane a pixel offset (`dock_pos`) from the top of its dock
and leaves any gap before it empty; panes that do not fit get height 0, and
their window is then not moved or hidden, so it stays where it was created
(the Periodic Builder showed as a caption-less box over the viewer).
`Builder::makeRoomFor` (Tools menu click, and the Periodic Builder after its
contents change size) renumbers the column's positions, gives the pane the
spare height, and closes the least wanted tool panes until it fits.
The Periodic Builder's controls sit in a `wxScrolledWindow` (it is built by
`PBC::Create`, which bypasses `PBCGUI::Create`) and the panel must never
`Fit()` or `Centre()` itself: it is AUI-managed.

Which tab opens: a calculation with no properties has an empty Properties
tab, so `refreshColumn` forces the Structure tab for it even when `/ColumnTab`
saved the Properties tab. Test hooks: scene commands `panelmode`, `toolmenu`,
`paneclose`, `panestate`, `pbcset`, `setcontext`, `columntab`, `xshot`
(`tests/apps/pbc_ux_test.py`).

Fold, Generate and a cell edit keep molecules whole: `Fragment::makeMoleculesWhole`
rebuilds each bonded group at the minimum image and (for Fold/Generate) moves it
by whole lattice vectors as a unit; the per-atom fold split methane across the
cell faces and a larger cell then left three H 6 A from C.
