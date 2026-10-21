---
type: map
title: "A second calculation's spectrum is a compare member of VibSpectrum; it follows the first's scale and width, and one y range covers both"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["include/tdat/VibSpectrum.H", "src/tdat/chemistry/VibSpectrum.C", "src/apps/builder/SpectrumCanvas.H", "src/apps/builder/NModePanel.C", "tests/spectrum"]
issues: [173, 410]
---
`NModePanel` "Compare..." lists the other calculations in the same folder
that have `VIBFREQ` and `VIBIR` or `VIBRAM`, and `VibSpectrum::setCompare`
holds the chosen one. The model, not the canvas, decides what two curves mean:

- The second spectrum takes the first's frequency scale, FWHM and line shape
  (setters forward), so the curves differ only in what was calculated.
- Intensities are drawn as printed when both have a known unit; if only one is
  relative, the second is stretched so its tallest stick equals the first's
  (`compareFactor`). Both curves share one y axis, `maxStick`/`maxEnvelope`
  and `defaultRange` cover both. `compareFactor` must not call `maxStick`
  (which includes the compare): that recursed forever once.
- `NModePanel::fillGraph` rebuilds the spectrum on every refresh, so it
  re-applies the stored compare (`p_compare`); `SpectrumCanvas::setSpectrum`
  replaces the whole model and would otherwise drop it.
- The second curve is not clickable and has no table rows; the table, mode
  animation and hover belong to the calculation the panel was opened on.
- CSV (#392 export): a "# compared with" header line, the second's sticks in a
  block of their own, the curve gets a third column `intensity_<name>`; names
  are stripped of commas and quotes.

Hooks (with `ECCE_SPECTRUM_DUMP`): `ECCE_SPECTRUM_COMPARE=<calculation name>`
and `ECCE_SPECTRUM_CSV=<path>`. `tests/spectrum/capture.py --compare CASE
g16-h2o-optfreq` installs both calculations in the fixture account and checks
the second's sticks and the CSV against the output file's own numbers;
`SPECTRUM_REPO_CLIENT=1` is needed when the builder is newer than /opt/ecce.
Unit test: `vibspectrum_test` (`testCompare`).
