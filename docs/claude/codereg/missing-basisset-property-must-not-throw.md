---
type: pitfall
title: "A calculation without basisSetDocument must open, as one with no basis set chosen"
area: codereg
section: "Pitfalls"
paths: ["src/dsm/xml/VDoc.C", "src/apps/calced/CalcEd.C", "tests/apps/calced_no_basis_test.py"]
issues: []
---
`VDoc::loadBasisSetName()` threw `InvalidException` when the
`basisSetDocument` property was absent. Every app that opens the
calculation then died with "Unhandled unknown exception; terminating the
application." and no window. It now falls back to the default name, so
`gbsConfig()` finds nothing and CalcEd shows "Not ready: choose a basis
set". The sibling loaders for optional documents (meta-potentials, QMMM
terms) never threw; the other required names (molecule, run log) still do.

Test: `tests/apps/calced_no_basis_test.py` (property removed from the
`.ecce-meta` of a Gaussian-16 calculation; a control keeps it).
