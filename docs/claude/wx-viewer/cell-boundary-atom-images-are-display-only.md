---
type: rule
title: "Atoms on a cell's corners, edges and faces are drawn again as display-only images (#859)"
area: wx-viewer
paths: [src/viz/sgcommands/SGContainer.C, include/viz/SGContainer.H, tests/apps/packing_cells_test.py]
issues: [859]
---
While the cell is shown (`latticeswitch`), `SGContainer::updateCellImages` adds
a plain `SoSphere` per equivalent boundary position (fractional coordinate 0
also at 1, tolerance 1e-3) under the `cellimages` separator, in the radius and
colour of the atom's style. They are not atoms: the atom count, selection
(the separator is `UNPICKABLE`), saved structure, inputs and properties never
see them. Skipped when the cell is replicated (those copies are real atoms).
They are rebuilt from `touchChemDisplay` and `touchLattice`; a style change
reaches them through the first, so a new path that changes style or radii
without touching the display must call `updateCellImages` itself. The
Builder script's `info` reports `cell_images`.
