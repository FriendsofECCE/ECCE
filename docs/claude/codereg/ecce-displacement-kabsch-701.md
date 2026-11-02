---
type: map
title: "ecce-displacement: per-element displacement and Kabsch RMSD from stored calculations (#701)"
area: codereg
paths: ["scripts/ecce_displacement.py", "tests/displacement/run_tests.py"]
issues: [701]
---
`scripts/ecce_displacement.py` (installed as `bin/ecce-displacement`) is wx-free
Python, a command-line tool for now (the Viewer panel is a later step).  Input: one or two stored
calculation folders.
- Geometry comes from `Props/GEOMTRACE` (a tsvectable: `rows` atoms x `columns`
  3, one `<step>` per frame, Angstrom).  Elements come from the `rowLabels`
  attribute when they are `1-Co` style, else from `Parameters/chemsys.mvm`'s
  `atom_list:` (the fixtures with bare indices `0 1 2` fall into this case).
- One calculation: per-element max and rms displacement from the first frame
  over the trace.  Frames share a coordinate frame, so no alignment.
- Two calculations: the last frame of each is fitted with Kabsch.  Proper
  rotation only (Horn's quaternion method: the eigenvector of the largest
  *algebraic* eigenvalue of the 4x4 cross-covariance matrix, via power
  iteration shifted by the Frobenius norm so a mirror's most-negative
  eigenvalue is not mistaken for the largest), so a mirror image is never
  reported as RMSD 0.  `--select` restricts both the fit and the table: 1-based
  indices or `within R of atom i` (the pocket around an ion).
- Output text/CSV/JSON via `--json`/`--csv`; two-calculation JSON keys are
  `rmsd`, `n_atoms`, `selection`, `atoms` (`index`, `element`, `dx`, `dy`,
  `dz`, `displacement`).
