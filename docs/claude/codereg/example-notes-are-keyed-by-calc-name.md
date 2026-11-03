---
type: pitfall
title: "Example notes are keyed by calculation name; a repeated name takes the wrong note"
area: codereg
paths: [src/apps/organizer/CalcMgr.C, data/client/examples, scripts/parsers/Gaussian-16.expt]
issues: [784]
---
`kNotes` in `CalcMgr::openExampleSet` maps a calculation name to its
Annotation, and `annotate` applies every entry whose name matches. `sn2_irc`
is the ORCA IRC in Organic > Reactions and the G16 IRC of the SN2 workflow,
so the later (ORCA) note landed on the G16 step, in the stored copy too.
A note named `<folder>/<calc>` (the project or workflow it is imported into)
now wins over a bare one; give any new repeated name such a key.

The same example showed an importer gap: `otherOptions` in
`Gaussian-16.expt` had no IRC case, so an imported G16 IRC was an Energy run
and `gaussian-16.irc`/`.ircgeom` (which exit unless the run type is IRC)
stored no path. A stored copy keeps such a parser result until it is
regenerated (`tools/examples/preparse.py <set>`).
