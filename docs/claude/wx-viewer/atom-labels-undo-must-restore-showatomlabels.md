---
type: pitfall
title: "AtomLabelsCmd::undo must restore showAtomLabels; turning it on after Save killed atom picking (#423)"
area: wx-viewer
paths: [src/viz/sgcommands/AtomLabelsCmd.C, include/viz/AtomLabelsCmd.H, src/apps/builder/Builder.C, tests/apps/builder_save_pick_test.py]
issues: [423]
---
`Builder::doSaveThumb` runs `AtomLabelsCmd` (type NONE) and then `undo()`
around the thumbnail render. `undo()` used to call
`sg->setAtomLabels(targetSize)`, which switches `showAtomLabels` on in
every `ChemDisplayParam` whenever any atom was a target, whatever the state
was before. After a Save the labels were therefore on, with empty text, in
that calculation's scene graph. A click on an atom then selected nothing
(`Builder::processSelectionChange` read no atoms, `SelectionChanged` cleared
the selection), the rubber band selected nothing, and the state stayed for
that calculation until ECCE restarted, because the Builder keeps one scene
graph per calculation URL. Ctrl+A and the atom table still worked, since
they do not go through the pick.

`execute()` now records whether labels were shown and `undo()` restores
exactly that. `tests/apps/builder_save_pick_test.py` (ctest
`apps_builder_save_pick`) clicks atoms through X after Save, a second Save
and a switch back to the calculation.

Not checked: whether labels the user turns on (View > Atom Labels) also take
clicks away from atoms.
