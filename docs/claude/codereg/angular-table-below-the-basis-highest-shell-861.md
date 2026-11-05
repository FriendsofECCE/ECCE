---
type: pitfall
title: "A code's spherical MOOrdering stops below the basis's highest shell: empty functions, and every orbital view refuses (ORCA g, #861)"
area: codereg
paths: ["data/client/cap/ORCA.edml", "src/tdat/chemistry/BasisFlatten.C", "src/tdat/chemistry/MoAoBasis.C", "tests/basisflatten/run_tests.py"]
issues: [861]
---
`BasisFlatten::flatten` takes each shell's components from the code's
`<MOOrdering type="spherical">` table in its `.edml`. A shell with an angular
momentum beyond the table's last `lval` gets empty placeholder functions, and
`MoAoBasis::flatBasis` then rejects the whole basis ("The stored basis set does
not match the orbital coefficients"). Density Map, Density Profile,
`ecce-density-field` and the MO surfaces all go through that evaluator, so one
g shell on one heavy atom (ORCA def2-TZVPP on Hg) disabled every orbital view.
The function count was right (455 = ORCA's 455); only the g components were
missing, so the failure message points away from the table.

ORCA's table stopped at f. It now has a g row in ORCA's component order
(m = 0, +1, -1, +2, -2, +3, -3, +4, -4), the ECCE-QM/libcint polynomials with
g(+3) and g(-3) negated, as for f. The signs were found by running ORCA 6.1.1
on HF/cc-pVQZ and checking every MO: the m = 0, +-1, +-2, +-4 rows matched
unchanged; only the m = +-3 MOs (c^T S c off by up to 0.076) needed the
negation. `tests/basisflatten/run_tests.py` (ctest `basisflatten`) keeps that
output as a fixture and asserts c^T S c = 1 for all 85 MOs and trace(PS) = N.

When a new basis family reaches a higher shell (h for ORCA def2-QZVPP heavy
atoms, aug-cc-pV5Z), check the table first: compare `max lval` in the
`MOOrdering` against the highest shell in the basis. Gaussian-16.edml,
NWChem.edml, DirDyVTST, MetaDyn and Polyrate still stop at f in the spherical
table; ORCA stops at g. Signs per code must be found against that code's real
output, not copied.
