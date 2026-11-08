---
type: map
title: "The Display panel drives VizStyleChooser and the Render actions; surface colours, opacity and cell settings are global (SurfaceStyle) (#881)"
area: wx-viewer
paths: [src/apps/builder/DisplayPanel.C, src/apps/builder/SurfaceStyle.C, src/apps/builder/BuilderDisplayHooks.C, src/wxviz/viztools/VizStyleChooser.C, src/viz/atomnodes/SGLattice.C, tests/apps/display_panel_test.py]
issues: [881]
---
Render > Display Settings... shows a Layer-2 AUI pane (own column; in the
shared Properties column it got no height). Style controls call new public
`VizStyleChooser` methods on a hidden dialog (`ViewerEvtHandler::styleChooser()`),
so Update/preview/automatic flags stay one code path. Labels and axes post
the Render menu's own ids with the check state in `GetInt()`.

`SurfaceStyle` keeps lobe colours, opacity, surface type and cell colour/labels
in wxbuilder.ini (`Display/*`; presets still write `MO/Colors` and
`/CUBE/Colors`). `MoPanel`/`Cube::getSurfaceColors` and `getTransparency`
consult it first; a legacy right-click preset or Semi-Transparent/Opaque clears
the free colours/opacity so both views agree. The panel re-reads on show and
on mouse-enter, it is not pushed. Hook: `display <form>` (BuilderDisplayHooks.C
header). Water's lowest MOs have one sign, so only the positive colour shows.
