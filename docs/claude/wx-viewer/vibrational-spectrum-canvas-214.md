---
type: map
title: "The Vibrational Frequencies graph is `SpectrumCanvas` over the wx-free `VibSpectrum` model (#214); look at `tools/spectrum/render` output, and mind the traps below"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["include/tdat/VibSpectrum.H", "src/tdat/chemistry/VibSpectrum.C", "src/apps/builder/SpectrumCanvas.H", "src/apps/builder/NModePanel.C", "tools/spectrum", "tests/spectrum"]
issues: [214, 173]
---
`NModePanel` no longer uses `ewxPlotCtrl`. `VibSpectrum` (sticks, scale,
broadening, `SpectrumAxis`, ticks) is wx-free and tested by
`vibspectrum_test`; `SpectrumCanvas.H` only paints it and is the same
code `tools/spectrum/render` paints onto a bitmap (`xvfb-run`).
`tests/spectrum/run_tests.py` compares the canvas with the numbers read
from the raw fixtures by `oracle.py` (no ECCE parser involved);
`tests/spectrum/capture.py` does the same through the real Builder.

- **Intensities in the units the code printed.** The pane axis is
  km/mol or Å⁴/amu only for the unit strings `knownIntensityUnits()`
  recognises (`KM/Mole`, `A^4/AMU`, `A**4/AMU`); anything else is drawn
  relative (largest stick 1). A new code with another spelling needs it
  added there.
- **All-zero intensities mean "not computed".** The Gaussian fixture
  stores `VIBRAM` as zeros without `Raman=`; such a property gets no pane.
- **Near-zero modes are not drawn.** ORCA lists the 6 translations and
  rotations as 0.00 and NWChem as about 1e-5 among the modes, so mode
  numbers do not start at 1 for the first stick. `zeroTolerance()` is 10
  cm-1; a real imaginary mode below that magnitude is lost.
- **Imaginary modes are drawn at their negative wavenumber** in the
  warning colour, listed in the hover, and left out of the envelope.
- **The envelope is area-normalised**, so its y values are
  intensity per cm-1 and sit on a second, right-hand axis next to the
  stick axis; they are not comparable with the stick heights.
- **The palette is shared.** `SpectrumPalette` is a typedef of
  `PlotPalette` in `include/wxgui/PlotStyle.H`, which every other plot
  uses too (see the plots entry); the spectrum paints byte-identically
  with it.
- **Hooks:** `ECCE_SPECTRUM_DUMP=<path>` (with `ECCE_SPECTRUM_CLICK=<mode>`
  and `ECCE_EXIT_AFTER_DUMP=1`) writes what the panel's canvas holds and
  paints it to `<path>.png`.
- `tools/spectrum/render` is not in CMake; the test compiles it with
  `wx-config`, like the MO diagram's canvas check.
