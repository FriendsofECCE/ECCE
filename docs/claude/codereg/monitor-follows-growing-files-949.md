---
type: map
title: "Follow=yes: the monitor reads a File= entry as it grows, during the run (#949)"
area: codereg
paths: [scripts/eccejobmonitor, scripts/parsers/orca.desc, scripts/parsers/orca.irctrj, tests/parsers/monitor_follow_test.py, tests/parsers/eccejobmonitor_sim.py]
issues: [949]
---
A `File=` entry with `Follow=yes` is not sent at the end (`JobFilesGet`).
In live mode `FollowRead` reads the complete lines the file gained since the
last read (byte offset kept per file) and sends them as one block of the
entry's parse type, at most once per `timePauseReadLine` and once more when
the job ends, before the buffered blocks. The block's first line is
`#ECCE-FOLLOW <file> <offset of the first byte>`: a script keeps its own
consumed offset in `$key`, joins a record split between reads and skips
bytes sent again (a restarted monitor starts every followed file at 0). In
post mode (import, reparse) a Follow entry sends nothing, so the end-of-job
file must be another, ordinary `File=` entry, whose script skips itself when
the followed one delivered (`orca.ircgeom` checks `$key/orcaIrcFrames`).
Each entry needs its own bracket name (`[IRCTRJF][IRCGEOM]`): the client
looks the script up by the bracket text, and a duplicate overrides.
The file is read beside the job output (the run directory), as `File=` is;
on a scheduler host that is the shared run directory the monitor sees.
