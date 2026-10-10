---
type: pitfall
title: "A stored basis set can be missing part of an aggregate; it is repaired when read, and copying a calculation copies the damage"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["src/dsm/edsiimpl/DavCalculation.C", "src/dsm/edsiimpl/EDSIGaussianBasisSetLibrary.C", "tests/basisload/run_tests.py", "tests/basisload/fixtures"]
issues: [164, 265]
---
**Calculations saved between 2026-08-28 and 8.16.0 (fa0847fc) can hold
an aggregate basis (6-31G*, 6-31+G*, aug-cc-pVDZ, ...) with one part
missing.** While the library gave both parts the same name and type,
`TGBSGroup::insertGBS()` kept the first, so the file's `<Data>` lacks
the polarization shells. The orbital mapping still lists the key twice,
and nothing else writes a key twice. The save is fixed; the files are
not, and **duplicating a calculation copies its basis file**, so damaged
files with new dates keep turning up.

What the run used depends on the deck. A named deck (NWChem `library`,
ORCA `NewGTO`/`!`, Gaussian Gen naming the set) ran the full basis, so
the MO viewer reported "output has 22, ECCE has 17". An explicit
Gaussian Gen deck from that period really ran without the d shell (17).

`DavCalculation::parseConfigMapElement()` therefore rebuilds a group
whose orbital mapping repeats a key from the default library under its
orbital name, unless its contractions were edited (deleted, uncontracted,
tight or diffuse functions added). After that, the explicit-deck runs
are the ones that disagree with the output, and the MO viewer refuses
them. That is correct: the run did not have the basis the user chose.
`tests/basisload` checks real stored files against the function counts
their own outputs printed.
