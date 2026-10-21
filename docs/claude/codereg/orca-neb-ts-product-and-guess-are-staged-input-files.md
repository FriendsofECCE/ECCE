---
type: pitfall
title: "ORCA NEB-TS (#419): the product and guess are .xyz input files of the calculation, matched by name"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["scripts/parsers/ai.orca", "scripts/codereg/orcaruntype.py", "scripts/codereg/calcpick.py", "data/client/cap/ORCA.edml", "src/apps/calced/QstStructures.C", "src/apps/calced/ESInputController.C", "tests/parsers/orca_neb_run_test.py", "tests/apps/calced_neb_test.py"]
issues: [419]
---
`! NEB-TS` needs `%neb NEB_End_XYZFile "neb_product.xyz"` (and, if picked,
`NEB_TS_XYZFile "neb_guess.xyz"`) next to the deck. CalcEd writes both files
into the generator directory (`nebWriteStructureFiles`, renumbered and aligned
onto this calculation, which is the reactant, with the same AtomMap code as
Gaussian's QST), `ai.orca` refuses the run type without `neb_product.xyz`, and
CalcEd then stores them as inputs of the calculation. The launcher copies every
file `getDataFiles(AUXILIARY_INPUT)` returns to the run directory, and that list
is built from the `<Input type="auxiliary">` entries of `ORCA.edml`: the two
names are listed there, so renaming a file in one place and not the other
silently leaves it out of the job (ORCA then dies on a missing file).

`removeInputFiles` clears all inputs but `guess.gbwin` on every Save, so a
product that is no longer picked cannot linger. `orcaruntype.py` and
`ged16runtype.py` share `calcpick.py` (candidate list and the URL-valued choice).

ORCA 6.1.1 `NEB-TS Freq` ends with the TS optimisation and its frequencies, so the
existing parse patterns give the TS energy, geometry and modes (one imaginary).
The NEB image path is not parsed: GEOMTRACE also picks up every image's
coordinate block during the NEB part, and the path energies (PATH SUMMARY) do not
reach the IRC Path panel.
