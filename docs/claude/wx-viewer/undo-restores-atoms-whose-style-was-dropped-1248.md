---
type: pitfall
title: "Undoing the delete of a whole style group restored the atoms but drew nothing: their display style had been removed (#1248)"
area: wx-viewer
paths: [src/viz/sgcommands/FragCloneCmd.C, include/viz/FragCloneCmd.H]
issues: [1248]
---
A PDB's solvent gets its own display style on opening. Deleting all of it
left the style without atoms, and `SGContainer::removeUnusedStyles()`
dropped its ChemDisplay. `FragCloneCmd::undo` then restored atoms still
tagged with that style's name, so no display drew them: the atom count was
right, the screen showed nothing, and the next Undo was the previous
command ("Undo Select Solvent"), which looked like an unrecorded delete.
`FragCloneCmd` now saves the style descriptors with its undo fragment and
re-creates missing ones in `restoreFrag`. A test of undo must check what is
drawn (`drawn` script hook), not only the atom count.
