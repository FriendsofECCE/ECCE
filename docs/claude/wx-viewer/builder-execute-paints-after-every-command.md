---
type: pitfall
title: "`Builder::execute()` paints after every command, so a command sequence shows each intermediate state (#226)"
area: wx-viewer
paths: [src/apps/builder/Builder.C, src/wxviz/viewer/SGViewer.C, src/apps/builder/MoPanel.C, src/viz/propsgcommands/IsoSurfaceCmd.C, src/viz/propsgcommands/IsoValueCmd.C, src/inv/moiv/ChemIso.C]
issues: [226]
---
`Builder::execute(cmd)` (non-batch) ends with `SGViewer::refreshRenderArea()`,
which is `Refresh(false)` + `Update()`: a synchronous paint. A panel that
reaches its final scene through several commands therefore puts every
intermediate scene on screen, one frame each.

MoPanel's Compute ran `IsoSurfaceCmd` (built at a hard-wired 0.05, 0.002
for ESP), `SurfDisplayTypeCmd`, then `IsoValueCmd` (the slider's cutoff):
two frames of a 0.05 surface before the requested one. `IsoSurfaceCmd` now
takes an `isovalue` parameter (0 = the field-type default) and MoPanel
passes the cutoff it is about to put on the slider. Cube.C still builds at
the default and never applies its slider on grid selection.

To see it headlessly: `tools/coin/scenes/calc-water-mopanel.scene`
(`mopanel <name>`, a Builder-only scene command) presses Compute and writes
every frame of the Builder canvas with the thresholds of the `ChemIso`
nodes in the scene at that frame; `ECCE_DEBUG_ISO=1` also logs each surface
`ChemIso::regenerate` actually generates.
