---
type: map
title: "pw.x outputs import through QuantumESPRESSO.expt (cell and atoms from the output)"
area: codereg
paths: ["scripts/parsers/QuantumESPRESSO.expt", "data/client/cap/QuantumESPRESSO.edml", "tests/importcode/importmeta_test.py", "tests/parsers/expt_cases.py"]
issues: [447]
---
`<Importer>QuantumESPRESSO.expt</Importer>` puts QE in `CodeFactory::getImportCodes()`;
the file is recognised by the existing `verifypattern="Program PWSCF"`. The
script writes a `.frag` with a `lattice_vectors:` block (Angstrom, the format
`Fragment::dumpMVM` writes) and a `.param` (Category/Theory PW, run type,
charge from valence sum minus electrons, cutoff, smearing); there is no `.gbs`
(`no_gbs` in `expt_cases.py`). Cell and positions are the last the file prints
(final `CELL_PARAMETERS`/`ATOMIC_POSITIONS` of relax, vc-relax, md), else the
setup tables times alat. Multiplicity is always written as 1: pw.x does not
echo it. Properties come from `qe.desc` in the job store, as for a live run.
`importmeta` (ctest) imports a pw.x output without the GUI and checks atoms and
cell against the output text.
