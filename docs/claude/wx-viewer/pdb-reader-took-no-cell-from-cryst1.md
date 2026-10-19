---
type: pitfall
title: "A PDB's CRYST1 was never read into the Fragment's lattice: a GROMACS box opened in the Viewer had no cell; LatticeDef(a,b,c,angles) is not the PDB frame"
area: wx-viewer
paths: [src/tdat/chemistry/Fragment.C, src/tdat/chemistry/LatticeDef.C, src/apps/builder/Builder.C, tests/launch/gromacs_prepare_test.py]
issues: [930]
---
`Fragment::restorePDB` ignored CRYST1 (only the `.car`/`.gro` readers set a
lattice), so a finished GROMACS Prepare (`chemsys.pdb` with `CRYST1 ... 60.00
60.00 90.00` for a rhombic dodecahedron) opened in the Viewer as bare atoms.
It now reads the record (a > 1 and all angles > 0; `1 1 1 90 90 90` is the
"no cell" placeholder programs write) when the fragment has no lattice yet.

**Do not build the lattice with `LatticeDef(a, b, c, alpha, beta, gamma)`.**
Its vectors come out in another frame (c along z, `a` with a z component,
|a| not equal to `a`), so for a triclinic cell the box is rotated against the
atoms. The PDB convention (a along x, b in the xy plane) is what GROMACS
writes its atoms in; the reader computes those vectors and `assign()`s them,
as the `.gro` reader does. `tests/launch/gromacs_prepare_test.py` compares
the cell the Builder holds with the box line of the stored `.gro` (the
oracle: gmx's own numbers, 0.0000 nm apart).

The atoms of a GROMACS box are not inside the parallelepiped in fractional
coordinates (they fill the compact dodecahedron around the centre), so "all
fractions in 0..1" is not a check to write.

The cell is drawn only while the Periodic Builder pane is open (`Builder.C`
opens it for an imported structure that has a lattice); a calculation opened
from the Organizer shows the cell after Tools > Periodic Builder.
