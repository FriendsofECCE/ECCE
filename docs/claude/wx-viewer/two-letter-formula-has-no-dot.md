---
type: rule
title: "The Hill-order formula is written He2, not He.2; hillOrder() still reads the old form"
area: wx-viewer
paths: [src/tdat/chemistry/TFormula.C, tests/formula/formula_test.C]
issues: []
---
`TFormula::formatElement` put a "." after a two-letter symbol with a count
("Fe.2Cl3"). No parser needed it: `parse_formula` and `hillOrder` read symbols
as a capital plus lower-case letters, so "Fe2Cl3" is unambiguous (only the
display reads `empiricalFormula`, via `hillOrder`). Formulas stored by older
versions keep the dot, and `hillOrder` accepts and drops it. Hill order is
unchanged.
