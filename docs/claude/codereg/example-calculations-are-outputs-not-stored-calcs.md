---
type: map
title: "Example calculations ship as program outputs; File > Open Example Calculations imports a fresh copy"
area: codereg
paths: ["data/client/examples", "src/apps/organizer/CalcMgr.C", "scripts/parsers/MOPAC.expt", "data/client/cap/MOPAC.edml", "tests/apps/examples_test.py"]
issues: [302]
---
`data/client/examples/<set>/<name>/` holds an output and its input (ECCE-QM water
and ethanol (Raman), ORCA benzene, MOPAC ammonia, NWChem methane), not a stored calculation: they
go through the ordinary importer, so they stay valid when the stored format
changes. Folders are grouped in sets (`general`: at most five calcs, imported at first
start into `Examples`; `computational`: the GROMACS Trp-cage, into
`Examples_Computational`; `genchem` -> `Examples_GeneralChemistry`, five GROMACS
runs (one `1-dynamics` task each, see md-speed-and-msd-come-from-the-mdanalysis-program.md);
`organic` -> `Examples_Organic`, 17 calcs, and
`spectroscopy` -> `Examples_Spectroscopy`, 10 calcs, #447/#485), each with a
`set.txt` (display name, one line).
`CalcMgr::openExampleSet` (File > Open Example Calculations asks which set;
`examples SET` of `ECCE_TEST_ORGANIZER` skips the dialog) imports a set into
its project, or only selects that project if it exists, and sets
`ecce:exampleSet` (the set version) on it. At a user's first start
`CalcMgr::importExamplesOnce` (CallAfter from the constructor) imports
`general` once, recorded as the `Organizer.ExampleSet` preference; an
existing `Examples` is only recorded. The import shows "N of M - <calc>" in
the progress dialog (and as `example progress` log lines); never Update() it
to its maximum, which waits for a click on Close. `isolate.py` sets
`ECCE_NO_EXAMPLE_IMPORT=1` so the other app tests start with an empty tree;
`examples_test.py` clears it. Every example's note (`kNotes`, keyed by the calculation name = output file
stem, so the same name has the same note in every set) must say "This
calculation shows ..." and state its method; numbers in a note come from the
shipped output, and nothing is said about agreement with literature. The ORCA
IRC outputs import like any ORCA output. `tools/examples/preparse.py [SET...]`
stores the pre-parsed copy of every file-output set. To change the set,
add the output to the set in `exampleSets()` and the expected `Props` to
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
