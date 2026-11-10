---
type: pitfall
title: "ORCA relaxed scans print optimizer MOs too: store only the converged final evaluation of each point"
area: codereg
paths: ["scripts/parsers/ai.orca", "scripts/parsers/orca.desc", "scripts/parsers/orca.mo", "scripts/parsers/orca.ptmo", "scripts/parsers/orca.scanmos", "tests/parsers/orca_scan_orbitals_test.py", "tests/apps/orca_scan_orbitals_test.py"]
issues: [549]
---
- Real ORCA 6.1.1 H2 HF/STO-3G scans with `Print[P_MOs] 1` print MOs
  during optimization and again after `THE OPTIMIZATION HAS CONVERGED`,
  before `OPTIMIZATION RUN DONE`. Even a diatomic gives two sets per
  point. Taking every MO block gives twice as many tables as SCANGEOM.
  ORCA also saves `.001.gbw`, `.002.gbw`, etc.; they are unnecessary here.
- ai.orca already requests `Print[P_MOs] 1` and `Print[P_OrbEn] 2`.
  orca.scanmos reads the completed primary output (`File=orca.orcaout`),
  collects only each point's final MO block after convergence, validates
  the contiguous sequence, and calls orca.ptmo/orca.mo for its PTORB/PTMO
  tables (including beta). This shares the ordinary parser's AO ordering.
  The existing PropertyTask point-selection path needs no change.
- The File= rule's first label is SCANPTMO, not PTMO. JobParser looks up
  the script by the first label via JCode, not by Begin; sharing PTMO with
  the IRC rule would dispatch both to the same script. Stored keys remain
  PTORB/PTMO; SCANPTMO is only the dispatch label.
- If any point lacks its final MO block, no point tables are emitted.
  Final-only output retains the existing final-point note and behavior.
  Earlier outputs already containing every point's MOs can be reimported;
  they need no rerun. IRC keeps its existing TS/end-point handling.
- No output growth over ECCE's previous default. Two real 26-point H2
  HF/STO-3G runs with MO printing disabled/enabled produced 1,279,110 and
  1,295,594 bytes respectively (about 16 KiB more, including 52 MO blocks).
  The new stored tables contain 26 sets, not 52.
- Tests: parser fixture preserves the orbital/geometry/scan sections of a
  real three-point run. Independent regex reads ORBITAL ENERGIES rather
  than the coefficient parser's energy row. The headless test checks all
  three points and revisits the first; --legacy checks final-only data.
