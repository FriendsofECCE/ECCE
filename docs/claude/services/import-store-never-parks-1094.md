---
type: pitfall
title: "An import or re-parse job store ends; it is never parked, restarted on silence or left past Quit (#1094)"
area: services
paths: ["src/comm/commxt/JobStore.C", "src/comm/commxt/JobParser.C", "src/comm/commxt/JobCatchUp.C", "src/apps/organizer/GromacsImport.C", "tests/progress/local_parse_ends_test.py"]
issues: [1094, 284, 208]
---
Importing a calculation (and the example GROMACS runs at first start) and
Re-Parse Outputs start eccejobmaster/eccejobstore/eccejobmonitor on a
scratch copy, with no job id. The park/restart rules of #208/#284 are for
live jobs: a store that went silent, lost its login or met `ecce_quit`
kept going (exit 4, WaitingJobs, closed messaging), and for an import that
loops with nothing able to end it -- `JobCatchUp::reconnect` refuses an
empty job id, so the calculation stays Running and its processes outlive
Quit (seen on Windows for the four argon slab examples).

- `localParse()` (re-parse, or a non-empty `importDir`) sends `park()`,
  `reconnectAfterSilence()` and `ecceExitMCB()` to `fail()`: Monitor Error
  with the reason, exit 3, no restart, not in WaitingJobs.
- A Quit during the first-start example import therefore leaves those
  examples as Monitor Error; the top-up only re-imports missing ones.
- Live jobs (`importDir` empty) keep the old behaviour, also at Quit.
