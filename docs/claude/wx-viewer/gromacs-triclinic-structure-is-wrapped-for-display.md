---
type: pitfall
title: "mdrun and solvate leave a triclinic box as a brick; the structure ECCE shows is wrapped into the compact cell first, molecules whole (#476)"
area: wx-viewer
paths: [src/tdat/chemistry/CompactCell.C, src/tdat/chemistry/Fragment.C, src/dsm/edsiimpl/MdTask.C, src/viz/propsgcommands/GTStepCmd.C, scripts/gensub, src/dsm/chemistry/GromacsStructure.C, src/apps/organizer/GromacsImport.C, tests/gromacs_md/wraptest.C, src/viz/atomnodes/SGLattice.C]
issues: [476, 512, 930]
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

- Existing calculations (#512): runs made before the wrap was done at
  creation hold the brick in their stored `chemsys.pdb` and trajectory.
  The geometry is `CompactCell::wrap` (coordinates only, any unit);
  `Fragment::wrapIntoCompactCell` applies it to a fragment with a
  triclinic lattice, and `MdTask` calls it whenever it restores an input or
  output fragment (bonds are regenerated, since they were found on the
  brick). `GTStepCmd` wraps each Geometry Trace frame of an MD task the
  same way. Rectangular boxes are untouched, and wrapping twice changes
  nothing, so new runs pass through harmlessly.
