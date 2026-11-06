---
type: map
title: "The Displacement panel hands ecce-displacement a scratch calculation folder (#701)"
area: wx-viewer
paths: ["src/apps/builder/GeomTracePropertyPanel.C", "tests/apps/displacement_panel_test.py", "scripts/ecce_displacement.py"]
issues: [701]
---
`ecce-displacement` reads a calculation folder, but the Builder has only the
data-server object. `GeomTracePropertyPanel::runDisplacement` therefore writes
`Props/GEOMTRACE` (rowLabels `1-Na 2-O ...` carry the elements) of the open
calculation, and of the one chosen by **Compare with...**, into a temp folder,
runs the tool with `--csv` and parses the table; no number is computed in C++.
Colouring writes `ChemColor::atomColor` per atom and saves/restores the prior
table and binding (restored on untick and in `loseFocus`). Compare offers only
siblings in the same folder whose GEOMTRACE has the same row count.
Test: `tests/apps/displacement_panel_test.py` (scene commands `gtdisp*` in
`BuilderTraceStress.C`).
