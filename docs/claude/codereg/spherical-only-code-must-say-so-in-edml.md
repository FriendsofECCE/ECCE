---
type: pitfall
title: "A spherical-only code must say `<SphericalOnly>` in its EDML, or its basis is recorded Cartesian"
area: codereg
section: "New-code checklist (gotchas found integrating ORCA, issue #38)"
paths: ["data/client/cap/ORCA.edml", "src/dsm/chemistry/GBSRules.C", "src/apps/basistool/WxBasisTool.C", "src/apps/calced/CalcEd.C", "tests/basis/testCoordSys.C"]
issues: [239]
---
**`GBSRules::autoOptimize()` records spherical only when the library basis
says it was designed for spherical functions**, and 79 of the shipped
`.BAS.meta` files (cc-pVDZ and def2-SVP among them) leave that flag blank,
so the rule records Cartesian. NWChem and Gaussian are then told Cartesian
and use it, so the record is true for them. ORCA has no Cartesian
functions and ignores the recorded convention, so the record was false:
the MO code expected six d functions where ORCA wrote five and only drew
because `ComputeMoCmd` falls back by coefficient width ("matches the
spherical basis, not the recorded one").

A code with spherical functions only declares `<SphericalOnly>True</SphericalOnly>`
in `<GaussianBasisSetRules>`; `GBSRules::enforceCodeCoordSys()` then sets
spherical in autoOptimize, when CalcEd saves the basis (a basis picked
by an older client), and in the Basis Set Tool, whose Cartesian button is
disabled. Imports were already right: `ORCA.expt` writes
`SphericalBasis: TRUE`. Checked by the `gbscoordsys` test.
