---
type: pitfall
title: "ORCA IRC (#417): the structures are in <input>_IRC_Full_trj.xyz, one frame per summary row, TS included"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["scripts/parsers/ai.orca", "scripts/parsers/orca.desc", "scripts/parsers/orca.ircgeom", "scripts/codereg/orcaruntype.py", "src/apps/builder/IRCPanel.C", "tests/parsers/orca_irc_run_test.py"]
issues: [417]
---
The IRC PATH SUMMARY in the ORCA output has the energies only. The
structures are written to `<input name>_IRC_Full_trj.xyz` in the run
directory, in path order with the TS in the middle. ECCE always names the
ORCA input `orca.orcain`, so the file is `orca.orcain_IRC_Full_trj.xyz`,
which `orca.desc` reads as a `File=` entry (`orca.ircgeom` makes one IRCGEOM
frame per xyz frame). Rename the input and the structures silently stop
arriving. `tests/parsers/orca_irc_run_test.py` runs a generated deck in
real ORCA and fails if the name changes.

Gaussian gives one structure per point except the TS (forward points, then
reverse points); ORCA gives one per point including the TS. `ircFrameMap`
in `IRCPanel.C` tells them apart by the frame count (equal to the number of
points: ORCA, identity map). The input needs the geometry to be the TS;
`ai.orca` always asks ORCA to compute the initial Hessian (`calc_anfreq`, or
`calc_numfreq` when chosen), since no `.hess` file is kept with a calculation.
