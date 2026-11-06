---
type: map
title: "Match bands (#707): ecce-bandshift matches modes of two calculations by overlap on the common atoms; the Vibrations panel runs it on modes files"
area: wx-viewer
paths: ["scripts/ecce_bandshift.py", "src/apps/builder/NModePanel.C", "tests/bandshift/run_tests.py", "tests/apps/nmode_bandshift_test.py"]
issues: [707]
---
- `scripts/ecce_bandshift.py` (`bin/ecce-bandshift`) is wx-free. It fits B onto A
  over the common atoms with `ecce_displacement.kabsch_rotation` (proper
  rotation), rotates B's modes, mass-weights both sets (stored vectors are taken
  as Cartesian displacements; no per-code renormalisation), cuts to the common
  atoms, renormalises and matches greedily by largest |overlap| above 0.5. A mode
  with no motion on the common atoms is unmatched ("no motion of the common
  atoms"), which is what makes a free ligand match inside a complex.
- Inputs are calculation folders (last GEOMTRACE frame, `Props/VIB`, `VIBFREQ`,
  `VIBIR`) or a modes file (ecce-displace's format plus an optional `ir M`
  block). The panel writes modes files from the property objects, so it also
  works for calculations that are not on the local disk, and reads `--lines`.
- Installed as `bin/ecce-bandshift` and `bin/ecce-displacement`, so the module
  import falls back to loading the file named `ecce-displacement`.
- Hooks: scene commands `vibbandshift <calc> <atoms|->` and `vibbandclick <row>`
  (BuilderTraceStress.C). A row click selects the mode through
  `wxEVT_GRID_SELECT_CELL` of the band grid.
