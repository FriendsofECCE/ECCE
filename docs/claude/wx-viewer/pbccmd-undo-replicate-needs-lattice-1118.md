---
type: pitfall
title: "PBCCmd::undo asks PBC to replicate the restored lattice; a command that added the lattice restores none"
area: wx-viewer
paths: ["src/viz/sgcommands/PBCCmd.C", "src/viz/sgcommands/SGContainer.C", "src/inv/moiv/ChemDisplayResidues.C", "src/apps/builder/PBC.C", "src/viz/sgcommands/SolvateCmd.C", "src/viz/sgcommands/PBCCreateCmd.C"]
issues: [1118]
---
`PBCCmd::undo` publishes `GeomChange`/`ResidueChange` with value
`replicate` when `shouldAutoReplicateOnUndo()`; `PBC::eventMCB` then calls
`getLattice()->getReplicationFactors()` with no null check. Every PBCCmd
used to start from a fragment with a lattice, so the restored fragment
always had one. Solvate on a molecule is the first PBCCmd that adds the
lattice; its undo segfaulted until `PBCCmd::undo` sent `replicate` only
when the restored fragment has a lattice (otherwise it sends
`LatticeChange`). Create lattice is a `FragCloneCmd` and never had this
path. A new PBCCmd that can add or remove the lattice must be undone with
Edit > Undo in a test.

The same undo then crashed in `ChemDisplay::bBoxResiduesAsRibbons`:
`SGContainer::touchChemDisplay` rebuilt each display's `residueIndex` only
when the fragment had residues, while `computeResidueAttributes` frees
`residueAttributes` when it has none, so an undo from residues (the waters)
to none read a stale index into a NULL array. `residueIndex` is now
emptied when there are no residues.

`tests/apps/builder_edit_bugs_test.runBuilder` loads benzene unless
`structure=None` is passed; a test that builds from SMILES without it
starts from 12 extra atoms.
