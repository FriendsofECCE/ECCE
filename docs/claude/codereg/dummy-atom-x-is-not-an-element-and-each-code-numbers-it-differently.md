---
type: pitfall
title: "The dummy atom X has no basis, no electrons, no formula entry - and each code numbers it differently"
area: codereg
section: "Input writers"
paths: ["src/tdat/chemistry/Fragment.C", "scripts/parsers/ai.nwchem", "scripts/parsers/ai.orca", "scripts/parsers/ai.gauss16", "scripts/parsers/ai.mopac", "scripts/parsers/ai.ecceqm", "src/viz/propsgcommands/GTStepCmd.C", "tests/dummy_atoms/run_tests.py"]
issues: [298]
---
In the data model X is an ordinary atom (atomic number 0, symbol `X`) that keeps its
index. `Fragment::uniqueTags`, `tagCounts` and `tagCountsSTL` skip it, so no basis set
is looked up for it (editor, Quick Basis, Basis Set Tool, not-ready check) and the
formula omits it. It adds no nuclear charge or electrons.

Per code (all verified with real runs on NH3 + X with fixed X-N-H angles):
- **NWChem**: the tag `X` in the geometry block is a dummy centre; `zcoord` counts it
  (1-based). Starting angles far from the constrained value make `autoz` fail with
  "imposition of constraints did not converge" - unrelated to the dummy.
- **ORCA**: `DA` in `* xyz`; `%geom Constraints` counts it (0-based). ORCA prints it as
  `XX`. `ai.orca` used to ignore the constraint file although ORCA.edml advertises it.
- **Gaussian 16**: `X` is accepted in Cartesian input but is *not a centre*: ModRedundant
  numbers skip it and naming it gives "Illegal centers". `ai.gauss16` shifts the
  constraint numbers down by the dummies before them and warns when a constraint names
  the dummy. Constraining the dummy needs a Z-matrix (not generated).
- **MOPAC**: `XX`; no constraints in ECCE. Omitted from every output table.
- **ECCE-QM**: rejected with a message.

Gaussian and MOPAC leave the dummy out of their geometry tables, so the trace has fewer
rows than the fragment; `GTStepCmd` and `CalcMgr` (reuse geometry) map the rows onto the
non-dummy atoms in order. Per-atom tables (charges, gradients) are not remapped.
