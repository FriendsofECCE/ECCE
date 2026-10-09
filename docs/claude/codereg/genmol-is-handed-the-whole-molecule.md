---
type: pitfall
title: "genmol is handed the whole molecule after the Builder's Find"
area: codereg
paths: ["src/apps/symmetry/dosymops.f", "src/apps/symmetry/genmol.f", "src/viz/sgcommands/FindSymmetryCmd.C", "src/tdat/chemistry/Fragment.C", "src/apps/calced/CalcEd.C", "src/apps/calced/ESInputController.C"]
issues: []
---
**Symmetry > Find sets `Fragment::useSymmetry(true)` on the whole molecule,
not on its symmetry-unique atoms.** The Calculation Editor (and the input
writer's `NumElectrons`) then run `Fragment::generateFullMolecule()`, i.e.
genmol, on every atom. genmol must therefore return a molecule that is
already complete unchanged.

It did not: in `dosymops.f` the "is this atom already present" test had an
empty body, so each atom's images were added again. Water came back as
H4O (5 atoms, 12 electrons), methane as 17 atoms, benzene as 72, in every
release from the PNNL base to 9.0.0-alpha.8. The test now skips an atom
already generated from an earlier one; from an irreducible set the result
is unchanged.

Tests: `tests/symmetry/run_tests.py` (`checkGenmol`: full and irreducible
input for water, methane, benzene) and `tests/apps/symmetry_find_test.py`
(the real Builder Find and Calculation Editor).
