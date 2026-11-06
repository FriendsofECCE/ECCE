---
type: map
title: "H2 curves (#858): ORCA has no rigid scan, GuessMix is reapplied per point, closed-shell CCSD has gradients; Fit Morse is ecce-morse --json"
area: codereg
paths: ["scripts/ecce_morse.py", "src/apps/builder/ScanPanel.C", "src/util/genutil/RateRun.C", "scripts/parsers/ai.orca", "scripts/parsers/ORCA.expt", "data/client/cap/ORCA.edml", "scripts/codereg/orcatheory.py", "scripts/codereg/orcatheory.dlg", "tools/examples/make_h2_scans.py", "tests/morse/unit_tests.py", "tests/apps/morse_panel_test.py"]
issues: [858]
---
ORCA 6.1.1 facts, each checked with a run:
- `%geom Scan` without `! Opt` is ignored (one single point). ECCE's scans
  are relaxed scans (Geometry run type); for a diatomic that is the rigid curve.
- `%scf GuessMix 75` (the "Broken-symmetry guess" box, UHF/UDFT) is applied
  again at every scan point: an outward UHF scan with it breaks symmetry near
  1.2-1.3 A just like the inward one. The outward example therefore runs
  without it and stays on the closed-shell (RHF) curve.
- Closed-shell CCSD has analytic gradients (Lambda equations); open-shell
  CCSD aborts in mdci_check unless `NumGrad` (ai.orca adds it).
- `ai.orca` rounds the scan end (5.0 + 92 * -0.05 was 0.399999999999999).
- ORCA.expt did not know CCSD: the imported theory was empty.

Fit Morse in the Relaxed Scan panel runs `ecce-morse <folder> --json`
(`runMorse`, beside `runRate` so it shares the JSON reader); the panel draws
`curve_r_A`/`curve_E_hartree` and shows `report` unchanged, so no Morse
formula is in C++. The default window is every point below the energy at the
longest distance. Masses: most abundant isotopes from Isotopes.txt; the bond
from the stored constraint, the ORCA `Scan B i j` echo, or a diatomic.
`ecce_morse.py` resolves its own path with realpath, so a symlinked
`bin/ecce-morse` (test harness homes) still finds Isotopes.txt and
ecce_si_export.

Compare... draws other scans of the same folder, each relative to its own
lowest point (no cross-method energy scale). Only curve 0 (the panel's own
calculation) answers clicks: the others have no structures in this panel.

Hooks: `scanmorse on|off [window]` (logs `ok re=... De_eV=...` and the text
box), `scancompare [none|NAME...]`, `scanpng PATH [W H]` (light palette).
