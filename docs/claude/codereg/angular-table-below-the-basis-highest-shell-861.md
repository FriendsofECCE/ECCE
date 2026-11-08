---
type: pitfall
title: "A code's spherical MOOrdering stops below the basis's highest shell: empty functions, and every orbital view refuses (ORCA g, #861)"
area: codereg
paths: ["data/client/cap/ORCA.edml", "src/tdat/chemistry/BasisFlatten.C", "src/tdat/chemistry/MoAoBasis.C", "tests/basisflatten/run_tests.py"]
issues: [861, 863]
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

ORCA g(+-4) was wrong in that first row (#863): a molecule on the z axis
cannot show m = +-4 or +-5, because those MOs mix with nothing. The fixtures are
HF/cc-pV5Z with H at (0.5294, 0.5294, 0.5294), so every m mixes. ORCA needs g
and h rows with m = +-3 and +-4 negated (146 MOs, c^T S c = 1 to 2e-6, trace(PS)
= 9.999993); NWChem g and h are Condon-Shortley (odd m > 0 negated), good to the
3 decimals of `ecce_print` (3.5e-3, trace(PS) 9.980; test tolerance 5e-3).
The h(+-5) sign is not decided by the data (no change in c^T S c either way at
this precision); it follows the libcint polynomial. To test a table: run the
code, feed its own parser output through `tests/basisflatten/run_tests.py`.
Gaussian-16.edml carries provisional g and h rows (standard polynomials, order
m = 0, +1, -1, ...): not verified, see #863.

When a new basis family reaches a higher shell (i for ORCA), check the table
first: compare `max lval` in the `MOOrdering` against the highest shell in the
basis. MetaDyn still stops at f. Signs per code must be found against that
code's real output, off the z axis, not copied.
