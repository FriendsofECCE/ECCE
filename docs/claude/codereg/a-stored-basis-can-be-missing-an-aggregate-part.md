---
type: pitfall
title: "A stored basis set can be missing part of an aggregate; it is repaired when read, and copying a calculation copies the damage"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["src/dsm/edsiimpl/DavCalculation.C", "src/dsm/edsiimpl/EDSIGaussianBasisSetLibrary.C", "tests/basisload/run_tests.py", "tests/basisload/fixtures", "src/viz/propsgcommands/ComputeMoCmd.C", "src/apps/builder/MoPanel.C", "tests/apps/orca_mo_test.py"]
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
are the ones that disagree with the output. The MO panel does not refuse
them: `ComputeMoCmd` counts the functions of the completed basis, and when
the output's width fits neither convention it reads the basis again as
stored (`ICalculation::gbsConfigAsStored`, no repair). If that one has the
output's width it is the basis the run used: the orbital is drawn with it
and `MoPanel::reportBasisFallback` says so (a dialog once per calculation,
a warning that stays in the panel; `FallbackFrom`/`FallbackWidth` on the
command). If neither basis fits, the old refusal stands. The other
`gbsConfig()` readers (View Coefficients, MO diagram, Mulliken) still use
the completed basis only.

`tests/basisload` checks real stored files against the function counts
their own outputs printed (`--stored`, and `--stored-exact` for the
shortened counts 17, 11, 23). `tests/apps/orca_mo_test.py` presses Compute
on a 23-function output with the lost-d water basis: drawn, one warning.
