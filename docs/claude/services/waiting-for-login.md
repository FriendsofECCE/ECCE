---
type: map
title: "Waiting for login: a lost monitor parks the job, session start catches it up"
area: services
section: "Pitfalls"
paths: ["src/comm/commxt/JobStore.C", "src/apps/jobstore/eccejobmaster.C", "include/util/WaitingJobs.H", "src/util/genutil/WaitingJobs.C", "include/comm/JobCatchUp.H", "src/comm/commxt/JobCatchUp.C", "src/apps/organizer/CalcMgr.C", "src/apps/organizer/CalcMgrApp.C", "tests/launch/waiting_test.py", "src/dsm/dav/DavEDSI.C"]
issues: [208, 206]
---
Losing the monitor says nothing about the job (#208, option A). When
eccejobstore loses it -- heartbeat timeout, monitor died, login refused,
`cd`/stream start failed (`restartSystem`), or SIGTERM at shutdown or
logout -- it calls `park()`: state **Waiting** ("Waiting for Login",
`STATE_WAITING`, between running and completed), reason in
`runStatusReason`, the URL recorded in `~/.ECCE/waiting/<fnv>.job` of the
*client* (in `-remote` mode too; the server is not involved). Exit 4 lets
eccejobmaster retry, exit 5 (SIGTERM) does not. A used-up restart budget
therefore leaves Waiting, never `system`. `restart()` (exit 2: protocol or
internal errors) still means Failed ("Monitor Error"), `fail()` too.

- A job that is really gone is decided by the monitor, as live: status
  302 is killed only with `killrequested`, otherwise `system` with the
  reason "neither an output file nor a status file" (c52c4e76).
- **Catch-up** is `JobCatchUp::run()`: the Organizer calls it once per
  session (`WaitingJobs::firstInSession`, the session key) after it opens,
  and `launchjob catchup` in tests. For each record whose calculation is
  waiting/submitted/running and not *watched* (a live eccejobmaster holds
  `flock` on `<fnv>.lock`), it does Run Management > Reconnect
  (`JobCatchUp::reconnect`, also what the Organizer's Reconnect calls): a
  fresh monitor from bookmark 0, the same parsers as live. If the login
  fails it is left Waiting again with the reason.
- Reconnect deletes the calculation's properties first. `DavEDSI`'s
  `listCollection(vector<ResourceResult>&)` skipped the collection's own
  entry only when the URL contained a "0", so on a port such as 8699 it
  DELETEd `Props/` itself and every later PUT got 409: the job ended
  "completed" with no properties. Fixed; `waiting_test.py` uses
  zero-free names so it covers that path.
- Waiting is treated like running for delete and state changes
  (`TaskJob::canChangeState`), and appears in `ResourceDescriptor*.xml`
  wherever Running does.
