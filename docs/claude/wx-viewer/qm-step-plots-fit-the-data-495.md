---
type: map
title: "QM step plots draw an energy relative to step 1 and convergence data as log10; XYPlotCanvas never leaves the data range (#495)"
area: wx-viewer
paths: ["src/apps/builder/StepPlotAxes.H", "src/apps/builder/XYPlotCanvas.H", "src/apps/builder/GeomTracePropertyPanel.C", "src/apps/builder/PlotAnyPropertyPanel.C"]
issues: [495, 496, 493]
---
`StepPlotAxes::shape` is the one place that turns a stored per-step series
into what is drawn: TEVEC/ESCFVEC/TECPVEC relative to step 1 in kJ/mol (the
final absolute value goes in the title, each point's in the hover), DELTAE/
RMSDP/DEWVEC as log10 of their size (the raw value in the hover). Geometry
Trace, Geometry/Wave Step Plots and VectorPlot all call it, and
`xAxisName` names the axes ("Optimisation step", "SCF cycle").

`XYPlotCanvas`: the zoomed-out view is the data range, and zoom, drag-zoom
and pan are clamped into it (`clampView`); `setIntegerX` gives whole-number
ticks for step axes. Two series of different length in Step Plots are cut
to the shorter, not padded with zeros that were never data.

**A G16 trace is longer than its energy series.** `gaussian-*.desc` routes
Input, Z-Matrix and Standard orientation blocks all into GEOMTRACE (the
Standard frame is what the modes use, #312), so a 38-step optimisation has
78 frames against 38 energies; a click on plot point i shows frame i, not
step i. Not fixed here.

The Geometry Trace's Play continues from the clicked point
(`OnPointClick` sets the playback's current step, #496).
