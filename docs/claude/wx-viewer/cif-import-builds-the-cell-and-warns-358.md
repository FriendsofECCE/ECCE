---
type: pitfall
title: "CIF import: operations are applied in-process, a space-group name alone goes through the genmollat helper; a CIF needs a lattice to be saved (#358)"
area: wx-viewer
paths: [src/tdat/chemistry/CifIO.C, include/tdat/CifIO.H, src/apps/builder/CifCalculation.C, tests/cifio/, tests/fixtures/cif/]
issues: [358]
---
`CifIO::read` expands the asymmetric unit with the operations the file lists
(`_symmetry_equiv_pos_as_xyz` or `_space_group_symop_operation_xyz`), wraps
into [0,1) and drops same-element images closer than 0.1 A. With only a
space-group name it normalises it (`F m -3 m` -> `Fm-3m`, `P 31 2 1` ->
`P3_121`) and reuses the `.car` route (`pointGroup()` +
`SymmetryOps::generateLatticeFragment`), which runs the external `genmollat`
helper: that case needs `$ECCE_HOME/bin` and loses the atom labels. There are
no operation tables in C++; `data/client/config/SpaceGroups` holds names only.

The lattice vectors are assigned in the PDB frame (a along x), as for CRYST1
(see pdb-reader-took-no-cell-from-cryst1.md).

Warnings are not returned through the load call: `CifIO::takeWarnings()` is
drained by `Builder::readFragmentFromFile` and `Builder::interpretCommand`
into `showMessage`. A new path that loads a CIF has to drain them too.

Export refuses a structure with no lattice (a CIF without a cell is useless);
the Save As dialog reports `CifIO::lastError()`. Partial occupancy is resolved
only between entries on the same position; disorder given as separate
positions (disorder groups) is read as separate atoms.
