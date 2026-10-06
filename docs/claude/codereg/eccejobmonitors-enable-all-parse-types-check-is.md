---
type: pitfall
title: "`eccejobmonitor`'s \"enable all parse types\" check is case-sensitive"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["Launch.C", "scripts/eccejobmonitor"]
issues: []
---
**`eccejobmonitor`'s "enable all parse types" check is case-sensitive**
(`scripts/eccejobmonitor`, `PDTypesEnable()`) — it only recognizes
lowercase `all`, but `Launch.C` hardcodes `"parseTypes ALL"`
(uppercase) when invoking it. The mismatch makes it silently delete
every not-yet-enabled parse descriptor from its live-monitoring match
table, so any `Frequency=all`-style trace property (`GEOMTRACE`, and
presumably any other code's equivalent) never gets extracted during
a run — no error, the job completes normally, only the trace data is
missing. Scalar single-value properties (`TE`, ...) are unaffected,
so this looks exactly like a per-property parsing bug and not the
systemic one it is. Fixed script-side (case-insensitive) rather than
touching `Launch.C`, so it doesn't need a C++ rebuild — if a fresh
build still drops a trace property with an otherwise-correct
`Begin`/`End` match, this fix predates it and something new is wrong.
