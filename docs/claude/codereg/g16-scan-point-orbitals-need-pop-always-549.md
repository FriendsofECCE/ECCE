---
type: pitfall
title: "Gaussian 16 prints a relaxed scan's MOs only at its first and last step: per-point orbitals need Pop=Always"
area: codereg
paths: ["scripts/parsers/ai.gauss16", "scripts/parsers/gaussian-16.desc", "scripts/parsers/gaussian-16.scangeom", "scripts/parsers/gaussian-16.scanmos", "tests/parsers/g16_scan_orbitals_test.py", "tests/apps/g16_scan_orbitals_test.py", "tests/gaussian_scan/run_tests.py"]
issues: [549]
---
- With `Pop=(full)` an Opt=ModRedundant scan runs the population analysis
  (and prints MOs) at the first and the last optimisation step only, and
  fort.7 (`Punch=(MO)`) holds only the last point's set. `Pop=(full,Always)`
  prints them after every SCF. `IOp(6/7=3)` with `Pop=Always` prints all MO
  coefficients without the density and Mulliken matrices (smaller), but
  `Pop=full` is kept for the final analysis.
- ai.gauss16 adds `Always` when RunType is Geometry/GeoVib and the
  constraint file has a `<Scan steps="N">` with N > 0 — not with NBO, which
  would then run at every step as well.
- A point's set is the last MO block before its `Optimization completed`
  (several steps per point; the block after the last one repeats the last
  point). That is the structure SCANGEOM stores for the point.
- eccejobmonitor allows ONE parse type per `File=` file (a second one with
  `File=g16.g16out` is an error, and the sim drops it as a Begin collision),
  so `gaussian-16.scangeom` pipes the whole log on to
  `gaussian-16.scanmos`, which prints PTORB/PTMO (and the BETA forms for
  UHF) in the ORCA layout. A log in which any point lacks its own block
  (older scans) gives no point tables; the final-point note stays.
- The log prints coefficients with 5 decimals; the last point equals the
  fort.7 set to 5e-6 (same AO order, so the MO/CODEBASIS handling applies
  unchanged).
- Output size: per optimisation step one MO table, density matrix and
  Mulliken matrix (about 3·N²·10 bytes). Water 6-31G*, 6 points/13 steps:
  237 KB -> 449 KB. For N = 300 that is about 2 MB per step.
- IRC (not done): with `Pop=(full,Always)` an IRC prints the TS set before
  `Start new reaction path` and one set before each `CURRENT STRUCTURE`
  (checked on HCN HF/3-21G, MaxPoints=4: TS + 8 points), i.e. in the
  run order #948 stores IRCGEOM in; not yet checked against the panel's
  point index.
