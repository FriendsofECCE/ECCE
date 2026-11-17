---
type: rule
title: "ECCE-QM ghost centres: Z = 0 with a basis element, in the grid, out of everything else (#1091)"
area: codereg
paths: ["src/qm/molecule.hpp", "src/qm/input.cpp", "src/qm/grid.cpp", "src/qm/basis.cpp", "tests/qm/ghost_test.py"]
issues: [1091]
---
Input: a fifth geometry column `ghost` (`O x y z ghost`). `Atom::Z` becomes 0 and the
element is kept in `Atom::ghost_Z`; `basis_Z()` returns Z for a real atom and ghost_Z for
a ghost. A dummy atom (X, no basis) is the same Z = 0 without ghost_Z, so
`without_dummies()`/`real_atoms()` keep an atom exactly when `basis_Z() != 0`.

Why Z = 0 is enough: nuclear repulsion, nuclear attraction (libcint CHARGE_OF), the
electron count, the dipole's nuclear term and D3 (skips Z = 0) all read `Z`, so a ghost
drops out of them with no further change. Places that need the element read `basis_Z()`:
the basis shells (`basis.cpp`) and the DFT grid (`grid.cpp`: Bragg radius, Treutler-Ahlrichs
xi, Becke a_ij). The grid therefore INCLUDES ghosts, so E_A(AB) and E_AB use the same
quadrature. The SAD initial guess gives a ghost no density.

Refused at parse time (`input.cpp`): no real atom at all (zero electrons, nothing to
compute), any runtype but energy, solvent, `charges`, polarizability/raman. `localise` is
switched off. A gradient on a ghost would need the Pulay terms of its basis functions;
nothing computes them, so no ghost-atom optimisation.

Output: `begin atoms` rows end with `ghost`; the symbol is the element, Z is 0.
results.h5 writes atomicNumber 0 for a ghost.
