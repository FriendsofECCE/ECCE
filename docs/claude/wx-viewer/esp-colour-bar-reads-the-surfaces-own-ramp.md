---
type: rule
title: "The ESP colour bar draws the surface's own ramp and the range the grid carries; it follows the surface on screen, not the type drop-down"
area: wx-viewer
paths: [src/apps/builder/MoPanel.C, src/viz/propsgcommands/IsoSurfaceCmd.C, tests/apps/orca_mo_test.py]
issues: [210]
---
The legend under the MO panel's ESP range row is built from
`IsoSurfaceCmd::potentialRampColor` and `potentialRampRange`, the same
functions that fill the surface's colour table, so the bar cannot drift
from the surface. Its end values are `SingleGrid::colorFieldMin/Max` read
after the IsoSurfaceCmd ran: that is the measured range (95th percentile of
|V| on the 0.5x..2x isovalue shell), a typed range, or `ECCE_ESP_RANGE`,
whichever won. The potential is in atomic units, Hartree/e (1 a.u. =
627.5095 kcal/mol per e); the density isovalue is the slider's 10^value.

The bar is shown from the end of a successful ESP Compute until another
Compute replaces the surface. Choosing a type in the drop-down draws
nothing, so it neither shows nor hides the bar. Moving the isovalue slider
does not rescale the ramp (IsoValueCmd leaves it), so the bar keeps its
range and only the isovalue text changes.

Check: `tests/apps/orca_mo_test.py --only esp` (scene commands `mofield`,
`mopanel`, `mobar`); the fixed range comes from `ECCE_ESP_RANGE`, the auto
one from the `ECCE_DEBUG_ISO` log, neither from the bar's own code.
