---
type: map
title: "NWChem MP2 and coupled cluster: which module each theory uses, and why MP/CC decks once aborted (#297)"
area: codereg
section: "How a property gets from disk into the Properties menu"
paths: ["scripts/parsers/ai.nwchem", "scripts/parsers/nwch.tpl", "scripts/parsers/nwchem.desc", "scripts/parsers/nwchem.thermo", "scripts/parsers/nwchem.emp2", "scripts/parsers/verifyinput", "data/client/cap/NWChem.edml", "tests/nwchem_cc/run_tests.py"]
issues: [297]
---
Run against NWChem 7.2.3 (water and OH, 6-31G*), not read from the manual.

- **`task ccsd` and `task mp2` still work** for a closed-shell RHF
  reference, with `freeze core N`, `thresh`, and numerical gradients,
  optimisation and frequencies. The "IO error ... End of File" line is only
  the missing restart vector (`t.t2`); the job then starts from MP2 and
  converges. The ticket's failure was ECCE's, below.
- **What really failed:** `ai.nwchem`'s `&SCF` died with "Unrecognized
  Category" for every category but SCF/DFT, because the template's `dft`
  block carries `##SCF.ConvergenceIterations##`, which the engine retries
  as `##SCF##`. Since #92 made resolver errors real, no MP, CC or NWPW deck
  could be generated at all. `&SCF` now returns empty (the block is dropped
  afterwards).
- **Open shell is where the modules differ.** ccsd: "nopen is not zero";
  mp2 with ROHF: "unknown SCFTYPE" (UMP2 works). The tensor contraction
  engine (`tce` block, `task tce ...`) does UCCSD, ROCCSD and their (T), and
  energy, gradient, optimisation and frequencies all run (the last three
  numerically, so they are slow). Those four theories are `UCCSD`,
  `UCCSD(T)`, `ROCCSD`, `ROCCSD(T)` in `NWChem.edml`; `&UsesTCE` selects
  them and `&SCFTheory` must test `^RO` before `^R`.
- **TCE needs no symmetry:** it aborts with "non-Abelian symmetry not
  permitted" for a linear molecule, so `&chemsys` writes `noautosym` for TCE
  decks whatever the Theory Details box says.
- **`removeUnusedSections` matches the first word of a line**, so the
  method line `ccsd` inside the `tce` block started a removal of the
  `ccsd` section there; it now only looks at column-0 lines.
- **Where the energies come from.** The ccsd module writes its energies to
  the `ecce_print` trace (`nwchem.emp2`, now also `ECCSD`). TCE writes only
  the final total energy there (TE = the highest method), so
  `nwchem.thermo` reads TCE's stdout through the `File=` entry and emits
  EMP2/ECCSD/ECCSDTPERT/ECCSDTPERT2 (TCE's CCSD[T]) and CORRELTN.
  `eccejobmonitor` allows one parse type per `File=` value, so that entry
  has to be shared with the thermochemistry script.
- **Verify** (`verifyinput`) flags a ccsd or an RHF/ROHF mp2 task with
  multiplicity above 1 as BAD ("openshell").
- Not done: `NWChem.expt` (import of an existing deck) does not read a `tce`
  block.
