---
type: map
title: "Reaction overview (#709): the dialog only shows what `ecce-thermo --reactions` prints"
area: codereg
section: "How a property gets from disk into the Properties menu"
paths: ["src/apps/thermo/ecce-thermo.C", "src/apps/thermo/ThermoGather.C", "src/apps/organizer/ReactionOverviewDialog.C", "src/util/genutil/ThermoRun.C", "src/apps/builder/XYPlotCanvas.H", "tests/apps/thermo_reactions_test.py", "tests/apps/reaction_overview_test.py"]
issues: [709]
---
The dialog runs `ecce-thermo --reactions FILE --chosen T --format csv`
(`runThermoReactions`) and parses the CSV: a reaction table, a blank line,
then one row per soft mode (< 100 cm-1). There is no second implementation
of the numbers, so the hook test compares the dialog's numbers
(`reaction n dE dG lo hi`) with the CLI. Reactions without `[ts]` have no
chart since the energy profiles replaced the dot chart (see the #872 entry).
`ThermoResult::modes` holds per-mode S and G correction with the same
damping as the totals; the soft-mode S adds up to the low-frequency part of
`sVib`. The shares are of the species' S and of G - E_el. A reaction is
refused when `gatherMethodKey` (code, method, basis, solvent, functional,
dispersion) differs between its species. `XYPlotCanvas` got
`setCurveErrorBars` (absolute low/high y, included in the data range). The
organizer target includes `src/apps/builder` only for that header.
