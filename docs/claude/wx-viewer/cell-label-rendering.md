---
type: pitfall
title: "Cell labels share the edge endpoints and skip collapsed vectors; GL current-state pushes must be balanced"
area: wx-viewer
paths: [src/viz/atomnodes/SGLattice.C, include/viz/SGLattice.H, src/apps/builder/BuilderPbcTest.C, tests/apps/viewer_cell_test.py]
issues: [723]
---
The Viewer and editable Periodic Builder use the same `SGLattice` node.
`PBCCreateCmd`, `PBCEditLatticeCmd` and `PBCEditVectorsCmd` all call
`SGContainer::touchLattice()`. Labels use the first three `toLines()` edge
ends. A zero-length vector must have no label: otherwise the text appears
at the corner of a 2-D cell, away from the atoms.

`GLRender()` had a `glPushAttrib(GL_CURRENT_BIT)` without a pop, exhausting
the GL attribute stack over repeated redraws. Balance it so the nested text
renderer can preserve its own state. `drawViewerText` now uses indexed glyphs
on both GL paths: pass literal a/b/c, with no old display-list index shift,
and do not skip the software path.

`viewer_cell_test.py --png DIR` checks creation, loading, each length/angle
edit, Cartesian vector edits, collapse/restoration of c, and translation.
`pbcset a1x` through `a3z` drive the Cartesian fields; `pbcpress save` saves
the isolated editable fixture so shutdown does not stop at a modal prompt.

Separate limitation: a collapsed vector makes `LatticeDef::getLattice()`
report NaN angles. Restoring c through its length field then uses those NaNs;
restore through the Cartesian vector controls when testing labels. This
label fix does not change singular-cell coordinate arithmetic.
