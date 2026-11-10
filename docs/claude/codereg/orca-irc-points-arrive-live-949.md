---
type: pitfall
title: "ORCA IRC energies arrive live, one row at a time; the structures only at the end (#949)"
area: codereg
paths: [scripts/parsers/orca.ircpoint, scripts/parsers/orca.energy, scripts/parsers/orca.irc, scripts/parsers/orca.desc, tests/parsers/orca_irc_live_test.py, tests/apps/irc_panel_test.py]
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

Path order rather than run order, because there is no frame to keep in step:
the readers sort by coordinate only when frames, energies and coordinates
have one count, so run-order vectors without frames would draw a zigzag.

The structures: ORCA appends each point to `<input>_IRC_F_trj.xyz` /
`_IRC_B_trj.xyz` while it runs (checked on 6.1.1), but the monitor reads
`File=` entries only after the job (`file-parse-properties-are-stored-last`),
so IRCGEOM still comes from `_IRC_Full_trj.xyz` at the end and a running
ORCA IRC shows points without structures. Live frames need the monitor to
tail those two files.
