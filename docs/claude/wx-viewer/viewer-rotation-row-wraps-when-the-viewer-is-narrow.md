---
type: pitfall
title: "The viewer's rotation row is a wrap sizer: a box sizer squeezed the Y/Z spin controls to 0 px when the docks left the viewer narrow"
area: wx-viewer
paths: [src/apps/builder/Builder.C, src/apps/builder/BuilderPanels.C, tests/apps/clip_test.py, tests/apps/clip_allow.txt]
issues: [839]
---
In the classic layout the property panes dock left (layer 2), the Atom
and Residue Tables left of the viewer (layer 0) and the tools right.
At 1366x768 with a property pane and the Residue Table open, each about
400 px wide, wxAUI gives the centre pane (the viewer) what is left, about
120 px: it does not honour a minimum for the centre pane.

The row under the viewer (`Builder::createRotators`: "Rotation: X/Y/Z"
spin controls and the view buttons) was a `wxBoxSizer`. A box sizer
narrower than its minimum shrinks its last items to 0 px, so the Y and Z
spin controls and all view buttons sat at the pane's edge with width 0
(clip audit: `text-wider-than-field`, "field is 0 px wide").

`p_rotSizer` is now always a `wxWrapSizer` (before, only at HiDPI): a
narrow viewer gets more rows under it, and every control keeps its size.
On wxMSW a wrap sizer's first layout reserves too many rows
(`wxmsw-silent-differences.md`).
