---
type: pitfall
title: "`ComputeMoCmd` caches a grid before computing it; a failed Compute must remove it"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["src/viz/propsgcommands/ComputeMoCmd.C", "src/apps/builder/MoPanel.C", "tests/apps/orca_mo_test.py"]
issues: [239]
---
**The MO grid is added to the cache (`addMOGrid`, keyed "MO alpha N")
before it is filled.** A Compute that gives up afterwards (coefficients
that do not fit the basis, an orbital index past the stored rows) left an
empty grid under that key, and the next Compute of the same orbital found
it, skipped the computation and reported "The grid value for this MO is
0". So the first press said one thing and every later press another.
Those paths now `removeMOGrid(key)`. The width mismatch is reported to
`MoPanel` through the command's `CoefWidth`/`BasisWidth` parameters, which
gives a dialog naming both counts once per calculation.
`tests/apps/orca_mo_test.py` presses Compute twice on a 23-function ORCA
output and requires exactly one dialog.
