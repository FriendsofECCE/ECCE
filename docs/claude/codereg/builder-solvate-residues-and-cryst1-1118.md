---
type: pitfall
title: "Atoms added to a Fragment that has residues need residues too, or dumpPDB drops them; Build > Solvate gives each water an HOH residue and dumpPDB writes CRYST1"
area: codereg
paths: ["src/viz/sgcommands/SolvateCmd.C", "src/tdat/chemistry/Solvate.C", "include/tdat/Solvate.H", "src/tdat/chemistry/Fragment.C", "scripts/gensub"]
issues: [1118]
---
`Fragment::dumpPDB` writes atoms residue by residue, and only calls
`FragUtil::createResidues` when the fragment has none. Atoms added after
residues exist, with no residue of their own, are silently left out of the
PDB that MD tasks (GROMACS Prepare) receive. `SolvateCmd` therefore runs
`createResidues` on a residue-less solute first, then makes one `HOH`
residue per water with atom names ` OW `, ` HW1`, ` HW2` (four characters:
dumpPDB uses the name only when it is exactly four long).

dumpPDB writes `CRYST1` only for a lattice in the PDB frame (a along x, b
in the xy plane); pdb2gmx then carries the box into step1.gro. The Prepare
job (gensub `gromacsPrepare`) counts SOL/HOH/WAT oxygens in step1.gro: with
water and a box it copies step1 to step2 (BOX=cell, editconf wording for the
summary) and skips `gmx solvate`; water without a box still goes through
editconf, but solvate is skipped.
