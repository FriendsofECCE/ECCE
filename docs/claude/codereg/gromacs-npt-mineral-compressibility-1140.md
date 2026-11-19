---
type: pitfall
title: "GROMACS NPT on a mineral stack: water's compressibility makes the barostat oscillate and mdrun segfault; the default is 2e-6 /bar when PeriodicMolecules is on"
area: codereg
paths: [scripts/parsers/ai.gromacs, scripts/parsers/md.gmxtask, src/dsm/xml/NWChemMDModelXMLizer.C, tests/gromacs_md/run_settings_test.py]
issues: [1140]
---
- Symptom: NPT "unsuccessful" after a few seconds; `gromacs.log` just stops
  (no fatal message), `gromacs.gmxlog` has "pressure scaling more than 1%"
  with mu swinging (1.07, then 0.76), a LINCS warning, then
  "Segmentation fault" and exit status 139.
- Cause: GROMACS scales every atom, so a mineral sheet stretches with the
  box and the stack is far stiffer than the 4.5e-5 /bar (water) the
  barostat is told. With tau-p 0.5 and grompp's nstpcouple 50 the loop
  overshoots with a growing amplitude. Continuation from the NVT state does
  not help; 2e-6 /bar does.
- The model XML leaves Compressibility out when it is the default
  (4.53e-10 m2/N), so `md.gmxtask` passes it only when present and
  `ai.gromacs` picks 2e-6 (PeriodicMolecules 1) or 4.5e-5. A user who types
  the default value back in gets the default for the system, not water's.
- The value only sets how fast the box relaxes, not where it ends.
