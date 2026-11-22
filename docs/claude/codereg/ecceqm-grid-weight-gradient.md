---
type: pitfall
title: "ECCE-QM XC gradients differentiate both the Becke partition and moving grid points"
area: codereg
paths: ["src/qm/grid.cpp", "src/qm/grid.hpp", "src/qm/xc.cpp", "src/qm/xc.hpp", "tests/qm/grid_weights_test.py"]
issues: [1250, 1124]
---
The quadrature point belongs to an atom and moves rigidly with it. Its
radial/angular weight stays fixed; its normalised Becke partition changes
with every centre. `Grid::atom` and `atomic_w` must follow the coordinates
through the spatial permutation in `XcIntegrator`.

`BeckePartition::weight` differentiates the three polynomial smoothing
passes, the size-adjusted elliptic coordinate, interatomic distances, product
weights and normalisation. Products are differentiated without dividing by
pair weights, which can be zero. The point's motion is added using
`d/dr = -sum_A d/dR_A`. `XcIntegrator::gradient` adds both `rho*exc*dw`
and the density's spatial derivative on the point's owner. Both are needed
for agreement with energy differences; subtracting a mean force alone
cannot supply the internal-coordinate response. The same chain rule applies
to LDA/GGA and closed/open spin channels. An independent implementation is
documented in [PySCF's grid-response gradient source](https://pyscf.org/_modules/pyscf/grad/rks.html).

Before the fix, B3LYP/6-31G* net-gradient norms (Eh/bohr) were:

| Geometry (bohr) | 99/590 | 150/974 |
| --- | ---: | ---: |
| Li at (0,0,0), F at (0,0,3) | 5.559207e-4 | 4.218520e-5 |
| Water, `gradient_test.pushed("h2o")` | 2.605474e-7 | 1.122586e-8 |

With grid response, the norms are at most 1e-10 Eh/bohr on both grids
(limited by printed gradient precision). Default-grid energy differences
agree to 2.24e-9 Eh/bohr for LiF and 3.90e-9 for water.

`qm_grid_weights` repeats both measurements, requires net force < 1e-6,
checks every gradient component against central energy differences (1e-4
bohr, < 1e-5 Eh/bohr) on the default grid, and translates the whole molecule
to check energy and gradient invariance. `qm_gradient` also covers LDA,
other GGAs, unrestricted/restricted-open shells and solvent.

The analytic DFT **Hessian still holds the grid fixed**. This fix makes
finite differences of gradients include grid response; it does not add
second partition derivatives or their CPKS terms to `hessian_terms`.
