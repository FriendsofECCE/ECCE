---
type: pitfall
title: "ChemSelection::rayPick must set ChemSelectionElement; labels shown by the user took clicks from atoms (#975)"
area: wx-viewer
paths: [src/inv/moiv/ChemSelection.C, include/inv/ChemKit/ChemSelection.H, src/inv/moiv/ChemDisplay.C, src/inv/moiv/ChemDisplayText.C, tests/apps/builder_save_pick_test.py]
issues: [975, 423]
---
`ChemDisplay::rayPick` picks only the parts named by `ChemSelectionElement`.
`ChemSelection::pick` sets that element from its `parts` field, but
`SoSeparator::rayPick` (Coin and SGI) traverses the children itself and never
calls `pick()`. The ChemDisplays therefore saw the element's default, `ALL`,
and with atom labels on (View/Options > Atom Labels) also hit the label
quads, which sit at their atom's depth. ECCE's `SGSelection` asks for
`ATOMS | BONDS` only, and `ChemSelection::processPickedPoint` drops every part
not in `parts`, so a label (or bond label, or residue ribbon) being the
nearest hit gave an empty pick: the click selected nothing and cleared the
selection. In a solvated box most clicks land on some atom's label.

`ChemSelection::rayPick` now sets the element too. Anything not in `parts`
is no longer picked at all, so a click passes through a label to the atom
behind it; a click on label text away from any atom is a click in free space.
If labels should ever select their atom, map `atomLabelIndex` to the atom in
`processPickedPoint` and add `ATOMLABELS` to the parts, but a label at a
front atom's depth then covers atoms behind it.

`tests/apps/builder_save_pick_test.py` clicks atoms through X with numbered
and atom-name labels in ball-and-stick and ball-and-wireframe; on the unfixed
tree the ball-and-stick case selected nothing. Plain Wireframe is not a test
case: a click at an atom there picks its bond (#660).
