---
type: pitfall
title: "ORCA prints an IRC's MOs once, at the TS: PTMO holds one table and only the TS frame gets orbitals"
area: codereg
paths: ["scripts/parsers/orca.mo", "scripts/parsers/orca.desc", "src/dsm/edsiimpl/PropertyTask.C", "src/apps/builder/PointNote.C", "tests/parsers/orca_irc_orbitals_test.py", "tests/apps/orca_irc_orbitals_test.py"]
issues: [751]
---
- With Print[P_MOs], ORCA writes MOLECULAR ORBITALS once, at the TS geometry
  before the IRC starts. `orca.desc` takes it with Frequency=last, which
  used to pair the TS orbitals with the last IRC structure.
- For a runtype containing IRC, `orca.mo` also emits it as PTORB/PTMO (one
  table each; same keys as ECCE-QM's per-point orbitals, #549).
  `PropertyTask::orbitalsAtSomePointsOnly()` is true when PTMO has one table
  and IRCGEOM has more; `orbitalTable(point)` then maps only the frame whose
  IRCCOORD is 0 (the TS) to table 0. Every other point, and "no point
  selected", hides MO/ORBENG/ORBOCC/LOCMO instead of returning the stored
  final set.
- The orbital note (PointNote.C) says ORCA gives orbitals at the TS only.
  `MoPanel::orbitalPointChanged` must refill an empty grid in this case: the
  panel is often opened at a point without orbitals first.
- tests/parsers/fixtures/orca/hcn_irc_mos.out is hcn_irc.out with an H2O MO
  block spliced in, not a real HCN run; replace it with a real P_MOs IRC run.
- Later (#751 step 2): end-point orbitals from `*_IRC_F.gbw` / `_IRC_B.gbw`.
