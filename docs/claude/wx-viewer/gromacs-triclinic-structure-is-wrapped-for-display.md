---
type: pitfall
title: "mdrun and solvate leave a triclinic box as a brick; the structure ECCE shows is wrapped into the compact cell first, molecules whole (#476)"
area: wx-viewer
paths: [scripts/gensub, src/dsm/chemistry/GromacsStructure.C, src/apps/organizer/GromacsImport.C, tests/gromacs_md/wraptest.C, src/viz/atomnodes/SGLattice.C]
issues: [476, 930]
---
The viewer draws the dodecahedron or octahedron as its Wigner-Seitz cell
(see gromacs-compact-box-drawn-as-wigner-seitz-cell.md) but draws the atoms
where the file has them, and a `.gro` from mdrun or `gmx solvate` fills the
brick, so a slab of waters sits outside the drawn cell.

- Live runs: gensub makes `view.gro` with `gmx trjconv -pbc mol -ur compact`
  (topology from `topol.tpr`) when the restart file has a 9-value box line,
  and `chemsys.pdb` is made from it. `confout.gro` is not touched. The
  Prepare task did this already; trajectories use the same trjconv call.
- Imported runs without `view.gro`: `GromacsStructure::wrapCompact` does the
  same from the `.gro` text alone (no topology), so molecules are
  residues joined to the next residue when a heavy atom of each is within
  0.175 nm; their geometric centre goes to the image nearest (a+b+c)/2.
  Rectangular boxes come back unchanged. `gromacs_wrap_compact` checks it
  against a brute-force Wigner-Seitz test on a gmx-solvated dodecahedron.
