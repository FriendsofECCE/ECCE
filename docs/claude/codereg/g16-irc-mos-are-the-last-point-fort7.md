---
type: pitfall
title: "A Gaussian 16 IRC punches only the last path point's MOs into fort.7; there are no per-point orbitals"
area: codereg
paths: ["scripts/parsers/ai.gauss16", "scripts/parsers/gaussian-16.mo", "scripts/parsers/gaussian-16.desc", "tests/parsers/cases.py"]
issues: [751]
---
- The IRC route already carries `Punch=(MO) Pop=(full)`; fort.7 then holds one
  MO set, that of the last point (its energies equal the last "Alpha occ.
  eigenvalues" in the log, not the TS's). `gaussian-16.mo` parses it as for
  any run, so MO/ORBENG/ORBOCC exist and pair with the last IRCGEOM frame.
- No PTMO is written, so `orbitalsAtSomePointsOnly()` is false and no point
  selection changes the orbitals. Per-point orbitals would need a G16 run per
  point (the log prints orbitals only at the end).
- Fixture `hcn_irc_mo.fort7` is a real HCN IRC fort.7 (case
  `g16-hcn-irc-mofile`).
