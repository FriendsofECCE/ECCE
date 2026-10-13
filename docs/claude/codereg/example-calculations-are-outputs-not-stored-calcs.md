---
type: map
title: "Example calculations ship as program outputs; File > Open Example Calculations imports a fresh copy"
area: codereg
paths: ["data/client/examples", "src/apps/organizer/CalcMgr.C", "scripts/parsers/MOPAC.expt", "data/client/cap/MOPAC.edml", "tests/apps/examples_test.py"]
issues: [302]
---
`data/client/examples/<name>/` holds an output and its input (ECCE-QM water
and ethanol (Raman), ORCA benzene, MOPAC ammonia, NWChem methane), not a stored calculation: they
go through the ordinary importer, so they stay valid when the stored format
changes. `CalcMgr::openExamples` (menu item and the `examples` command of
`ECCE_TEST_ORGANIZER`) makes project `Examples`, or `Examples_2`, ... (project
names allow no spaces), sets `ecce:exampleSet` (the set version) on it, and
imports each output; it never touches an existing project. At a user's
first start `CalcMgr::importExamplesOnce` (CallAfter from the constructor)
does the same once into `Examples`, recorded as the `Organizer.ExampleSet`
preference; an existing `Examples` is only recorded. `isolate.py` sets
`ECCE_NO_EXAMPLE_IMPORT=1` so the other app tests start with an empty tree;
`examples_test.py` clears it. To change the set,
add the output to `kExampleOutputs` and the expected `Props` to
`tests/apps/examples_test.py`, and raise `kExampleSetVersion`.

- NWChem is imported from the `ecce_print` stream (`.eprint`); a plain stdout
  output is refused. Run the input with an `ecce_print <name>.eprint` line.
- MOPAC had no importer: `MOPAC.expt` (+ `<Importer>` in `MOPAC.edml`) reads
  Hamiltonian, run type, charge, spin and the last `CARTESIAN COORDINATES`
  block of a multi-data-set output; the properties come from `mopac.desc`.
- The Props names that mean a charge scheme: `ESPFIT` is CHELPG, `MKFIT` is MK.
- `JobParser::importCalculation` looked the machine up by host name; the
  Windows client registers itself as `localhost` only
  (`RunMgmt::registerLocalMachine`), so every import there ended "<host> must
  be a registered compute server", the empty calculation was deleted again
  and the Examples project stayed empty. It falls back to `localhost` now.
- `openExamples` selects the new project once, imports them all with
  `p_batchImport` set (no per-calculation tree rebuild, which wxMSW paints
  step by step) and rebuilds the project's subtree once at the end; a failed
  import is reported after that, not overwritten (#263).
