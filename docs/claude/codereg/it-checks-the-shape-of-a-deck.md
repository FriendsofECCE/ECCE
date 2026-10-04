---
type: rule
title: "It checks the SHAPE of a deck and must never check a keyword"
area: codereg
section: "The input checker (`scripts/parsers/verifyinput`, #148)"
---
**It checks the SHAPE of a deck and must never check a keyword.**
Per code: section order and the blank lines between them, Link 0
directives preceding the route, the title/charge/geometry positions,
block open/close (`* xyz`…`*`, `… end`, `&namelist`…`/`), and the
basis block — declared primitive counts matching what is listed, and
every element in the geometry actually covered. This tree has been
wrong about keyword *validity* from reading manuals repeatedly
(NWChem rejects its own documented `disp grimme3`; ORCA takes
`6-31++G**` and refuses `6-31++G`; Gaussian wants `GD3BJ`, ECCE
wrote `GD3-BJ`), and a checker that calls a correct deck wrong is
*worse than no checker* — the user stops reading it, including when
it is right. Anything not decidable from the file is `UNSURE`, which
is a real verdict, not a weak `BAD`.
