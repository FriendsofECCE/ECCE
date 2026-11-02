---
type: pitfall
title: "ORCA prints an IRC's MOs once, at the TS; the two end points come from re-reading their gbw (PTMO holds three tables)"
area: codereg
paths: ["scripts/parsers/orca.mo", "scripts/parsers/orca.desc", "src/dsm/edsiimpl/PropertyTask.C", "src/apps/builder/PointNote.C", "scripts/gensub", "scripts/parsers/orca.ptmo", "tests/parsers/orca_irc_orbitals_test.py", "tests/apps/orca_irc_orbitals_test.py"]
issues: [751]
---
- With Print[P_MOs], ORCA writes MOLECULAR ORBITALS once, at the TS geometry
  before the IRC starts. `orca.desc` takes it with Frequency=last, which
  used to pair the TS orbitals with the last IRC structure.
- ORCA also writes `<job>_IRC_B.gbw` and `_IRC_F.gbw` (backward/forward end),
  which it never prints. gensub's `orcaIrcEnds` re-runs ORCA on the compute
  machine from each one (the input with the IRC block removed, `MOREAD
  NoIter`, `%moinp`, `* xyzfile` of `_IRC_B.xyz`/`_IRC_F.xyz`), so the
  printed block goes through orca.mo's own AO ordering; a molden export
  would need its own (p is x,y,z there, z,x,y in ORCA's text).
- gensub appends the TS block again and the two end blocks to the output under
  the heading `ECCE-IRC-END-ORBITALS` (TS, B, F). An orca.desc rule cannot
  share a Begin with another (the first wins), hence the new heading;
  `[PTMO]...` (script `orca.ptmo` = orca.mo with `$main::PTONLY`) takes them
  with Frequency=all, one table each. The `[MO]` rule keeps ORCA's own block.
  An IRC output without these appended blocks (an import) gets no PTMO.
- `PropertyTask::orbitalsAtSomePointsOnly()` is true when PTMO has fewer
  tables than IRCGEOM has frames; `orbitalTable(point)` maps the IRCCOORD-0
  frame to table 0, the most negative IRCCOORD to the backward-end table and
  the most positive to the forward-end one (each only if that branch exists).
  Every other point, and "no point selected", hides MO/ORBENG/ORBOCC/LOCMO.
- The orbital note (PointNote.C) says ORCA gives orbitals at the TS and the
  two end points. `MoPanel::orbitalPointChanged` must refill an empty grid.
- tests/parsers/fixtures/orca/hcn_irc_mos.out is a real ORCA 6.1.1 run plus
  the three appended blocks as gensub writes them.
