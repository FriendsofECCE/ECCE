---
type: map
title: "Difference and experimental curves in the Vibrations spectrum (#707): the model holds them, ecce-spectrum is the numbers' reference"
area: wx-viewer
paths: ["scripts/ecce_spectrum.py", "include/tdat/VibSpectrum.H", "src/apps/builder/SpectrumCanvas.H", "src/apps/builder/NModePanel.C", "tests/spectrum/cli_tests.py", "tests/apps/nmode_spectrum_test.py"]
issues: [707]
---
- `VibSpectrum` has `setDifference` (B minus A, B the compare member), and
  `setExperimental` with `experimentalFactor(kind, lo, hi)` / `experimentalValue`
  (NaN outside the data). Factor = largest computed envelope on a 1 cm-1 grid over
  [lo, hi] divided by the largest experimental value in [lo, hi]; the frequency
  scale never touches it. The CSV curve block gets `difference_<B>_minus_<A>` only
  when the difference is on and `experimental` when one is loaded.
- `NModePanel::fillGraph` builds a fresh model each refresh, so the panel keeps the
  experimental arrays (`p_expX/Y`) and the difference flag and re-applies them,
  like the compare member.
- File reading, transmittance detection (baseline nearer the maximum than the
  minimum; > 1.5 means percent) and T to A are only in `ecce_spectrum.py`; the
  panel calls `ecce-spectrum --parse-exp`. The envelope is implemented twice
  (C++ and `ecce_spectrum.Curve`); `tests/apps/nmode_spectrum_test.py` compares
  every exported column with the CLI to catch drift.
- The difference is drawn in its own pane under the plot (about 25 % of the
  height, `paintDiffPane`) with a signed y axis; it carries the x tick labels and
  title, the main pane keeps its axes. Not in compact (docked) height.
- Zoom fields call `SpectrumCanvas::zoomRange` (the axis's `zoomTo`, as drag-zoom).
