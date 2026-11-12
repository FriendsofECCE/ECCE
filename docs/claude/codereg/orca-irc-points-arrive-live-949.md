---
type: pitfall
title: "ORCA IRC points and structures arrive live; run order once frames come (#949)"
area: codereg
paths: [scripts/parsers/orca.ircpoint, scripts/parsers/orca.irctrj, scripts/parsers/orca.geomtrace, scripts/parsers/orca.ircgeom, scripts/parsers/orca.energy, scripts/parsers/orca.irc, scripts/parsers/orca.desc, tests/parsers/orca_irc_live_test.py, tests/apps/irc_panel_test.py]
issues: [949, 948, 417]
---
ORCA's output prints, per IRC point, only a row of the FORWARD/BACKWARD IRC
iteration table (`k  E  dE  max(|G|)  RMS(G)`), no structure. `orca.ircpoint`
takes each banner and each row (Lines=1 entries) and re-sends IRCCOORD and
IRCENERGY whole, in path order: TS at 0 (the last FINAL SINGLE POINT ENERGY,
kept in `orcaIrcE` by `orca.energy` for an IRC runtype), row k at +(k+1)
forward and -(k+1) backward, the same coordinates `orca.irc` gives the
summary rows. The summary (Frequency=last) replaces both vectors at the end,
so a finished calculation stores exactly what it did before #949.

Path orderStructures: ORCA appends each point to `<input>_IRC_F_trj.xyz` /
`_IRC_B_trj.xyz` (no TS in either; frame k is row k), which the monitor
follows (`monitor-follows-growing-files-949.md`). `orca.irctrj` sends one
IRCGEOM frame per new xyz frame, the TS first (the last CARTESIAN
COORDINATES block, kept in `orcaIrcXyz` by `orca.geomtrace`), and logs each
in `orcaIrcFrames`. Once that file exists `orca.ircpoint`, `orca.irctrj` and
the summary (`orca.irc`) send the vectors in run order (the order
`orcaIrcPoints` received them), so point i is frame i and the readers sort
the three by coordinate, as for G16; `orca.ircgeom` (the `_IRC_Full_trj.xyz`
entry at the end) then sends nothing. Without followed frames (an import)
the vectors stay in path order and IRCGEOM comes from `_IRC_Full_trj.xyz`,
exactly as before: run-order vectors without frames would draw a zigzag.
`tests/parsers/orca_irc_trj_live_test.py` replays both.
