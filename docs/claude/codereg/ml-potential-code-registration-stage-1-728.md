---
type: map
title: "The ML potential code (MLP) is registered like xtb, but its runner is inlined into the job script and its version is a metadata string (#728)"
area: codereg
section: "Getting a calculation set up (the \"code registration\" system)"
paths: ["data/client/cap/MLP.edml", "scripts/ecce-mlp", "scripts/parsers/ai.mlp", "scripts/parsers/mlp.desc", "scripts/gensub", "scripts/codereg/mlptheory.py", "scripts/codereg/mlpruntype.py", "tests/mlp", "tests/launch/mlp_test.py"]
issues: [728]
---
Application type `MLP` (resource `mlp_es`, CONFIG key `mlp`, gensub sub `mlp`),
Periodic group in the code bar. One Theory, `MACE`, category `SE` with
`needsBasis="false"`; run types Energy and Geometry.

- **The registered "program" is a Python interpreter**, not the code. gensub
  reads `scripts/ecce-mlp` and writes it into the run directory with a
  quoted heredoc (`ecce-mlp.py`), so a cluster needs the environment (ASE,
  mace-torch) but no ECCE. A new runner option is a `key=value` word on
  line 1 of `mlp.mlpin` and a resolver in `ai.mlp`; the deck keys are read by
  the runner, not by gensub.
- **Deck** = `mlp key=value ...`, then extended xyz; the cell goes in the
  `Lattice=` comment from the fragment's `lattice_vectors:`. A cell relaxation
  without a lattice is refused in `ai.mlp`.
- **Output is the parser's input.** `ecce-mlp` prints `ECCE-MLP-FRAME` blocks
  as it goes (GEOMTRACE, TEVEC, LATTICEVEC from one frame, so they stay the
  same length) and one `ECCE-MLP-FINAL` block (TE, EGRADVEC, STRESS,
  PRESSURE). The two use different markers because two entries on one Begin
  starve one. Units: eV and eV/A in the output, Hartree and Hartree/Bohr
  after `mlp.final`; stress in kbar with QE's sign (`-1 *` ASE's).
- **VERSION is not a file in Props/.** A string property is stored as
  `ecce:version` in the calculation's `.ecce-meta`; the launch test reads it
  from there.
- **A step or time limit ends "unsuccessful".** `ecce-mlp` prints
  `ecce-mlp: NOT CONVERGED`, gensub lists it under `%notConverged`, and the
  last structure is still stored. `ecce-mlp terminated normally` is the
  normal-termination marker.
- **The Calculation Editor's fixed note** ("for screening, not for reporting")
  lives in the Theory and Runtype dialogs (`mlptheory.py`, `mlpruntype.py`);
  the ASL notice follows the model menu in `CheckDependency`.
- **Mock for tests**: `ECCE_MLP_MOCK=lj|emt` swaps the network for an ASE
  calculator (`tests/mlp/run_tests.py`, `tests/launch/mlp_test.py`). The real
  MACE run is `tests/mlp/mace_test.py` with `ECCE_MLP_PYTHON`.
