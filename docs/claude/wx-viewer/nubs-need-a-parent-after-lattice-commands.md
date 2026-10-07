---
type: pitfall
title: "`addCovalentBonds` never bonds nubs; lattice Generate/Replicate/Fold relied on it and left every nub parentless (#243)"
area: wx-viewer
paths: [src/tdat/chemistry/Fragment.C, src/tdat/chemistry/SymmetryOps.C, src/viz/sgcommands/SGContainer.C, src/viz/sgcommands/PBCReplicateCmd.C, src/viz/sgcommands/PBCFoldCmd.C, src/apps/symmetry/genmollat.f]
issues: [243]
---
A nub's parent is its first bonded atom (`Fragment::nubParent`), and Add
Hydrogens, delete, add-on-nub and bond-two-nubs all dereference it.
`Fragment::addCovalentBonds` skips nubs on both sides, so any command that
rebuilds every bond (Periodic Builder Generate via
`SymmetryOps::generateLatticeFragment`, Replicate, Fold, Equivalent
Rectangle, Transform Cell) used to leave all nubs with no bonds, and the
next Add Hydrogens dereferenced null. `addCovalentBonds` now ends with
`bondLooseNubs`, which bonds each unbonded nub to the nearest atom over
minimum images and moves it back beside that atom: `genmollat` puts every
centre into [0,1) fractional, so a carbon at the origin gets three of its
nubs on the opposite faces. Real atoms are not moved back, so hydrogens
added before Generate can sit on the far face, unbonded in the display
(bonding is not periodic). Test: `tests/apps/pbc_edit_test.py`.
