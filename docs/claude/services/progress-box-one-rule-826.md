---
type: rule
title: "Progress box: every energy in Eh with six decimals and a real minus; a summary line per finished stage; the final energy names what it contains (#826)"
area: services
section: "Pitfalls"
paths: ["src/util/genutil/JobProgress.C", "src/apps/organizer/CalculationContextPanel.C", "src/apps/organizer/ProgressBox.C", "scripts/parsers/orca.desc", "tests/progress/progress_test.C", "tests/progress/codes_test.py", "tests/apps/progress_box_test.py"]
issues: [826]
---
- **Format.** `ehText()` in `JobProgress.C` (blocks, summary, ZPE) and the
  final-energy line in `updateProgress` print `-1533.957145` as
  `U+2212 1533.957145 Eh`. The copied value (Copy energy) keeps an ASCII
  minus so it pastes into a calculator. The workflow boxes keep 3 decimals.
- **Summary lines.** SCF line when no optimisation ran: alone, or before
  `Frequencies:` when the frequencies are analytic or have no point count.
  A numerical frequency run's last SCF is a displaced geometry's, so no SCF
  line there.
- **"What it contains".** The `result` stage is never drawn; the final line
  reads `(SCF + D3)` only when the code's output says so. ORCA only:
  `DFTD3`/`DFTD4` header gives `disp=`, `gCP correction` gives `gcp=`
  (r2SCAN-3c: `SCF + D4 + gCP`). No other code prints such a marker the box
  can read, so they get no parenthesis. ORCA's SCF block needed its own
  `SCF CONVERGED AFTER n CYCLES` entry, or the SCF line said "stopped".
- **Colour.** "N imaginary" is drawn in `PlotPalette::imaginary` wherever it
  appears (blocks and summary) when N is not what the search wants.
