---
type: pitfall
title: "NWChem's eprint holds MO coefficients to 3 decimals; results.h5 takes them from nwchem.movecs"
area: codereg
paths: ["src/tdat/chemistry/ResultsFromCalc.C", "scripts/parsers/nwchem.mo", "data/client/cap/NWChem.edml", "tests/resultsh5/run_calc_tests.py", "tests/resultsh5/fixtures/nwchem-methane-movecs"]
issues: [944, 407]
---
The `molecular orbital vectors` block of the eprint file (what `nwchem.mo`
parses into MO) has 3 decimals; no input keyword changes that (7.2.3).
That alone fails the results.h5 checks: methane B3LYP/6-31G* gives
tr(PS) = 10.0138 and max |C'SC-1| = 4.8e-3; rounding ECCE-QM water's
coefficients to 3 decimals gives 3.4e-3 and 2.0e-3. The basis is not the
cause: NWChem's printed 6-31G* (Cartesian) equals the stored one, and the
same run's Molden file (`property; moldenfile`) passes to 5e-11.

New jobs keep `nwchem.movecs` (#407, `RetainOutputFile`). `gather` reads it
(Fortran unformatted; integers 4 or 8 bytes by build) and replaces the
parsed coefficients only when every coefficient agrees with the parsed one
to 6e-4, i.e. it is the same set of orbitals (degenerate MOs rotate between
runs, and the file is the last SCF's). Imported outputs have no movecs and
still fail, marked; the views then use the Props.
