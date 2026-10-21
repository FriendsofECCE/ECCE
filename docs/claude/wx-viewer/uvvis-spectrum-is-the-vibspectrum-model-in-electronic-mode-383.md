---
type: map
title: "The UV-vis Spectrum panel is `SpectrumCanvas` over `VibSpectrum` in electronic mode; the width is always in eV, even on a nm axis"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["include/tdat/VibSpectrum.H", "include/tdat/ExcitedStates.H", "src/tdat/chemistry/ExcitedStates.C", "src/apps/builder/UvVisPanel.C", "src/apps/builder/SpectrumCanvas.H", "tests/spectrum/uvvis_test.C"]
issues: [383, 214]
---
`VibSpectrum` takes an x unit (`setXUnit`: `X_WAVENUMBER`, `X_EV`, `X_NM`)
instead of a second model class, so `SpectrumCanvas` (zoom, pan, hover, click,
shared pop-out state, the palette) is reused unchanged. In electronic mode
the "modes" are excited states stored in eV, the one intensity pane is the
oscillator strength `f`, and the wording (axis title, "State", hover
line) comes from `VibSpectrum` (`xTitle`, `itemWord`, `positionText`, ...),
not from strings in the canvas. `ExcitedStates` (wx-free) builds the model
from the four properties and writes the CSV.

- **The curve is evaluated in energy.** `envelope(x)` converts an nm
  position to eV (`hc/x`) before adding the area-normalised line, so
  switching the axis shows the same curve, not one with a Jacobian. Its unit is
  f per eV, whatever the axis; the integral over energy is the sum of f
  (`uvvis_test`). The width is in eV in both axes.
- **Axis limits are per unit.** `SpectrumAxis::setMinSpan` (0.5 eV, 10 nm;
  20 cm-1 stays the default) and `defaultRange`'s own branch (steps of 0.5 eV
  or 10 nm, three widths of margin); the nm axis ascends left to right.
- **No vibrational conventions.** Near-zero and imaginary modes only exist
  for wavenumbers; an electronic state is drawn if its energy is positive.
  A calculation whose f are all zero has `has()` false and shows
  "No excited state has a nonzero oscillator strength."
- **Properties.** `EXCIENERGY` (eV), `EXCIOSC`, `EXCISYM` (spin and symmetry
  label, spaces replaced by `_`), `EXCITRANS` ("8>9:0.70503,7>10:-0.19",
  orbitals 1-based, ORCA's 0-based numbers shifted by one) in
  `scripts/parsers/excited.pl`, one `[EXCIENERGY]...` entry in
  `gaussian-16.desc` and `orca.desc`. Wavelength is not stored: it is hc/E
  (reproduces ORCA's one-decimal nm and Gaussian's two-decimal nm).
- **Which block.** Gaussian prints one block per geometry (an Opt+TD job has
  five): `Frequency=last`, so the converged geometry. ORCA's entry runs from
  the first "EXCITED STATES" header to the velocity-dipole table, so the
  listing and the electric-dipole table (energy to six digits, fosc to nine)
  reach one script; a state without a row in that table (a triplet) has f 0.
- **Hooks:** `ECCE_UVVIS_DUMP=<path>` with `ECCE_UVVIS_AXIS=nm|eV`,
  `ECCE_UVVIS_WIDTH=<eV>`, `ECCE_UVVIS_CLICK=<state>`, `ECCE_UVVIS_CSV=<path>`.
  `tests/spectrum/makecalc.py orca-h2co-tddft <dir>` builds a calculation
  folder for `tests/panels/capture.py --open-panel "UV-vis Spectrum"`.
- Checks: `tests/parsers/excited_oracle_test.py` (each code against its own
  printed table), `uvvis_test`, the `g16-h2co-tddft` and `orca-h2co-tddft`
  parser cases. Do not put results of two codes side by side in any text.
