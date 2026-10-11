---
type: pitfall
title: "Complex-conjugate irrep pairs are ONE real row flagged `separable:` (norm 2h)"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["data/client/config/CharacterTables", "src/tdat/chemistry/CharacterTable.C", "src/tdat/chemistry/SymmetryAnalysis.C", "tests/symmetry/run_tests.py"]
issues: [132]
---
**C3-C6, Cnh, S4/S6/S8, T and Th have complex irreps.** The codes label
each conjugate pair as one two-dimensional `E` (`E1`, `E'`, ...), so
`CharacterTables` stores the pair as one real row, `2cos(2 pi k j/n)`,
listed on a `separable:` line. Such a row has norm 2h and stands for two
1-D irreps.

Anything that applies the projection formula by hand must divide a
separable row's sum by 2h, not h, or it reports "2E" for one E.
`CharacterTable::reduce()` and `reduceToOneIrrep()` do; use
`CharacterTable::separable()` if you add another. The row's
`dimension()` is 2, which is right for degeneracy but not for the
projector's d/h prefactor (harmless here: results are Gram-Schmidt
normalised afterwards).

The two classes `C_n^k` and `C_n^(n-k)` are separate classes with equal
combined characters, so a table has more classes than rows; the Python
test counts separable rows twice and checks columns through each class's
inverse, derived from the class names.
