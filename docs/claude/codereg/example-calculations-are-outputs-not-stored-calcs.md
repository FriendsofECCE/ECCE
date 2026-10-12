---
type: map
title: "Example calculations ship as program outputs; File > Open Example Calculations imports a fresh copy"
area: codereg
paths: ["data/client/examples", "src/apps/organizer/CalcMgr.C", "scripts/parsers/MOPAC.expt", "data/client/cap/MOPAC.edml", "tests/apps/examples_test.py"]
issues: []
---
`data/client/examples/<name>/` holds an output and its input (ECCE-QM water,
ORCA benzene, MOPAC ammonia, NWChem methane), not a stored calculation: they
go through the ordinary importer, so they stay valid when the stored format
changes. `CalcMgr::openExamples` (menu item and the `examples` command of
`ECCE_TEST_ORGANIZER`) makes project `Examples`, or `Examples_2`, ... (project
names allow no spaces), sets `ecce:exampleSet` (the set version) on it, and
imports each output; it never touches an existing project. To change the set,
add the output to `kExampleOutputs` and the expected `Props` to
`tests/apps/examples_test.py`, and raise `kExampleSetVersion`.

- NWChem is imported from the `ecce_print` stream (`.eprint`); a plain stdout
  output is refused. Run the input with an `ecce_print <name>.eprint` line.
- MOPAC had no importer: `MOPAC.expt` (+ `<Importer>` in `MOPAC.edml`) reads
  Hamiltonian, run type, charge, spin and the last `CARTESIAN COORDINATES`
  block of a multi-data-set output; the properties come from `mopac.desc`.
- The Props names that mean a charge scheme: `ESPFIT` is CHELPG, `MKFIT` is MK.
