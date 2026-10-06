---
type: pitfall
title: "`eccejobmonitor`'s \"enable all parse types\" check must stay case-insensitive"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["src/comm/commxt/Launch.C", "scripts/eccejobmonitor"]
issues: []
---
**`PDTypesEnable()` in `scripts/eccejobmonitor` compares `all`
case-insensitively, because `Launch.C` sends `"parseTypes ALL"`.** A
case-sensitive check deletes every not-yet-enabled parse descriptor from
the live match table, so `Frequency=all` trace properties (`GEOMTRACE`
and the like) are never extracted while scalar ones (`TE`) still are: no
error, the job completes, and it looks like a per-property parsing bug. The
fix lives in the script so that no C++ rebuild is needed; keep the two
sides agreeing if either changes.
