---
type: map
title: "Every plot is an `XYPlotCanvas` (src/apps/builder/XYPlotCanvas.H), drawn like the vibrational spectrum with `PlotStyle.H`'s palette; `tests/plots/capture.py` pictures them before and after a change"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["include/wxgui/PlotStyle.H", "src/apps/builder/XYPlotCanvas.H", "src/apps/builder/BuilderPlotShot.C", "tests/plots"]
issues: [210, 214]
---
`XYPlotCanvas` (#930) is the one x-y plot of the Builder: the Geometry
Trace and Step plots, Relaxed Scan, MO energy levels (`LEVELS` style, key
above the plot), reaction-rate, equilibrium and metadynamics plots. It
paints with the same axes, ticks (`niceTicks`), fonts, `PlotPalette` and
hover box as `SpectrumCanvas`. `ewxPlotCtrl`/`wxplotctrl` are no longer
used by any panel. A click on a point selects it and calls
`setPointHandler(curve, index)`; drag zooms a rectangle, the wheel zooms
about the pointer, right/middle drag pans, double click on empty plot or
Home fits.

- **Curve colours:** `addCurve(..., series)` takes `palette().series(i)`
  (Okabe-Ito); `setCurveColour` overrides it.
- **Hooks:** `gtpick` calls `pointPosition()`, which lays the plot out
  itself, so it works before the first paint.
- **No export:** the old control had none either; Scan has its own CSV.
- **Pictures.** `tests/plots/capture.py --png DIR --tag before|after`
  floats each plot panel in the real Builder at two sizes (scene command
  `plotshot`) and saves the pixels; `tests/plots/sheet.py DIR` makes the
  contact sheets. One Builder per panel and size.
- **`gtpick <step|mid|last>`** (scene script) clicks the plot where a step
  is drawn and checks the molecule is the trace's; it is in
  `tests/apps/geomtrace_stress.py`.
