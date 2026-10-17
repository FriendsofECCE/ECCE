---
type: rule
title: "An atom pick chooses the MO diagram (#301)"
area: mo-diagram
paths: [src/apps/builder/MoDiagramPanel.C, src/tdat/chemistry/MoFragments.C, tests/modiagram/capture.py]
issues: [301]
---
The option row's "Centre/first atom" and "Bonded to" choosers (labels:
symbol + atom number from 1, as the Builder numbers atoms) pick the diagram;
both empty is Automatic. A pick overrides "Built as"; "Choose fragments" wins
over a pick. One metal: the fragment-orbital diagram. One other atom X: X
against its bonded atoms (`MoFragments::build(..., pick)`, CENTRAL), refused
unless every symmetry operation leaves X in place. A bond: cut it; a ring
bond is refused; sides swapped by a symmetry operation are HALVES, sides each
mapped onto themselves are GROUPS, anything else is refused (the pieces are
not unions of orbits, so they have no symmetry orbitals). A refusal is a
tier-B line plus the bare spectrum, never a different diagram.
`ECCE_MODIAGRAM_PICK="O1"` / `"C1,C2"` drives it in tests. Not done: falling
back to the molecule's lower symmetry when a pick breaks the group.
