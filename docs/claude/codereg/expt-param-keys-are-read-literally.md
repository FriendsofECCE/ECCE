---
type: pitfall
title: "An `.expt`'s `.param` keys and `.frag` attributes are read literally; a wrong key is dropped silently"
area: codereg
section: "New-code checklist (gotchas found integrating ORCA, issue #38)"
paths: ["*.expt", "ORCA.expt", "Gaussian-16.expt", "NWChem.expt", "src/dsm/edsiimpl/DavCalculation.C", "src/dsm/edsiimpl/ICalcUtils.C", "tests/importcode"]
issues: [235]
---
**An importer's `.param` keys and `.frag` attributes are read
literally, and a wrong or missing one is dropped with no message (#235).**
`DavCalculation::processImportParameters` reads `ES.ChemSys.Multiplicity`
(ORCA.expt wrote `ChemSys.Multiplicity`, so every ORCA import had
multiplicity Unknown) and `DavCalculation::import` reads `SphericalBasis`
(absent means Cartesian). The point group is not a `.param` key at all.
It goes in the `.frag` as `fragment_attributes:` / `point_group: X` /
`end_fragment_attributes:`, which `Fragment::restoreMVM` reads. Without
it the import records C1 unless the job store's PNTGRP parse catches a
line afterwards, and orca.desc's PNTGRP only matches the thermochemistry
line, so it misses a UseSym single point. Use the molecule's group
(ORCA "Auto-detected point group"), not ORCA's "Reduced point group":
that is the abelian subgroup the orbital labels use.

`ICalcUtils::importNameBasis` looks a library name up per element, and an
aggregate with an ECP part (def2-* = basis + `DEF2_ECP.POT`, Rb onwards)
returns that part for every element, empty for light ones. Inserting it
gave benzene the ECP "Def2-ECP...". The import now drops an ECP that has
no potential for the element.

`tests/importcode/importmeta_test.py` (ctest `importmeta`) runs the real
importer plus `TaskJob::import()` in local mode, with no data server, and
checks the stored point group, multiplicity, basis, ECP and spherical
flag against patterns read from the output text. Add each new importer's
fixtures there.
