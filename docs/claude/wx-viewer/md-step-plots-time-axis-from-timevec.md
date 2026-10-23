---
type: map
title: "An MD run's Geometry Trace and Geometry Step Plots use TIMEVEC (ps) as the x axis; gromacs.time fills it and StepPlotAxes.H holds the shared pieces"
area: wx-viewer
paths: ["src/apps/builder/StepPlotAxes.H", "src/apps/builder/GeomTracePropertyPanel.C", "src/apps/builder/PlotAnyPropertyPanel.C", "scripts/parsers/gromacs.time", "tests/apps/md_plots_test.py"]
issues: [480]
---
Both panels were already on `XYPlotCanvas`; what #480 changed is the axes.
`gromacs.time` reads the "Step Time" value row (before the energies marker,
so `gromacs.energy` never sees it) into `TIMEVEC` in ps, one point per log
step beside TEVEC/PRESSVEC. `StepPlotAxes::mdTime` returns it (ns from 1000 ps)
only when it matches the plotted series in length and advances; a
minimisation (time 0 throughout) and QM keep the step index. Time-axis
plots get a thin "running average" curve (cumulative mean); the s.d. stays in
MD Summary. Plot names drop the " Vector" suffix; `unitLabel` gives bar,
kg/m³, nm³. A point click still maps plot point i to trace frame i (clamped,
`GTStepCmd`): the trace holds at most 100 thinned frames with no time.
`tests/apps/md_plots_test.py` checks it against a real gmx 2025.2 NPT log
(`tests/parsers/fixtures/gromacs/water_npt.log`); pictures in
`candidates/480/`.
