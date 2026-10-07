---
type: pitfall
title: "A parser script can be handed half a line: post mode delivers a final line with no newline"
area: codereg
paths: ["scripts/eccejobmonitor", "scripts/parsers", "tests/parsers/partial_replay.py"]
issues: [107]
---
A live `eccejobmonitor` never delivers a line without its newline
(`FileReadLine` holds it back), but in post mode the end of the file is the
end of the job and `<JOF>` hands the fragment over. A job killed mid-line and
re-parsed therefore feeds scripts a block ending in half a line. For
`End=task` NWChem blocks the fragment `task_opt...` matches `End` but not the
script's own `last if /end/`, so its tokens become values; for `Lines=1`
entries the value is simply empty. The rule for a script: stop at the End
line as well as at the data, and emit nothing, not a short or empty record,
when the input is incomplete. Same for `.expt` importers (a half orientation
table must not become a shorter molecule) and `File=` files (a fort.7 cut
mid-orbital). `tests/parsers/partial_replay.py` (ctest `parsers_partial`)
cuts the fixtures in all these places and fails on any of it.
