---
type: map
title: "Every plot is an `ewxPlotCtrl` styled by `PlotStyle.H`, the palette the vibrational spectrum canvas uses; `tests/plots/capture.py` pictures them before and after a change"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["include/wxgui/PlotStyle.H", "include/wxgui/ewxPlotCtrl.H", "src/wxgui/ewxClasses/ewxPlotCtrl.C", "src/wxgui/wxplotctrl", "src/apps/builder/BuilderPlotShot.C", "tests/plots"]
issues: [210, 214]
---
`SpectrumCanvas` (#214) is specific to spectra; what is shared is its
colours. `PlotPalette` (was `SpectrumPalette`, still a typedef) lives in
`include/wxgui/PlotStyle.H`, header-only so `tools/spectrum/render` needs
no wx widgets. Every other plot is an `ewxPlotCtrl` (the old wxPlotCtrl
under `src/wxgui/wxplotctrl`); `ewxPlotCtrl::ApplyStyle()` runs from
`Create()` and gives it the system window colours, Okabe-Ito curves
(`SeriesPen(i)`), small tick labels, short outside ticks, a pale grid, and
a readout box for the point under the pointer. Drag zooms a rectangle,
double click on empty plot fits everything, click still selects the point.

- **A new plot panel needs nothing**; if it sets curve pens itself, use
  `p_plotCtrl->SeriesPen(n)` and `GetPalette()`, not `wxBLUE`/`wxCYAN`.
  A curve whose pens still equal the library default is restyled in
  `StyleNewCurve()`; one with its own pens keeps them.
- **`SetCorrectTicks(false)`.** The library's tick "correction" moved the
  view to put ticks on round numbers and cut data off after a resize (the
  Geometry Trace curve left the plot). Ticks now go where they fall.
- **The y-axis window is as wide as its labels** (`CalcYAxisTickPositions`
  re-lays out when the width changes); it used to be a fixed guess at
  `-5.5e+555`. The window now ends at the plot frame so ticks reach it.
- **`DrawAreaOverlay()` and `StyleNewCurve()`** are the two new virtual
  hooks in `wxPlotCtrl`; the readout is drawn in the first.
- **Rate-constant, equilibrium, MO-level and step plots all go through
  this**; `PlotMeta3D`'s plot is commented out and draws nothing.
- **Pictures.** `tests/plots/capture.py --png DIR --tag before|after`
  floats each plot panel in the real Builder at two sizes (scene command
  `plotshot`) and saves the pixels; `tests/plots/sheet.py DIR` makes the
  contact sheets. One Builder per panel and size: resizing a floated
  panel left stale pixels in the plot's native windows.
- **`gtpick <step|mid|last>`** (scene script) clicks the plot where a step
  is drawn and checks the molecule is the trace's; it is in
  `tests/apps/geomtrace_stress.py`.
