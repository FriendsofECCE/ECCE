---
type: pitfall
title: "A failed job's output ends inside a .desc block; the monitor must drop that block, not die"
area: services
paths: ["scripts/eccejobmonitor", "src/comm/commxt/JobStore.C", "tests/launch/failed_output_test.py"]
issues: [470, 467, 468]
---
A job that fails usually stops in the middle of a section some `.desc`
entry reads: ORCA NEB-TS `HCN_TS` printed MOLECULAR ORBITALS and died in the
property step before MULLIKEN POPULATION ANALYSIS, the `[MO]` entry's `End`.
`JobOutputGet` used to `Die` on that open block at the end of the file, so
the monitor exited before `JobFilesGet`: no output file, no buffered
(`Frequency=last`) properties, no `jmSTATUS`. `eccejobstore` treats a
monitor `fatal` as `restart()`; each restarted monitor read on from the last
bookmark, sent a progress snapshot of one point (which replaced the stored
`ecce:progress`, #467) and died the same way. The calculation ended Failed,
reason "FATAL error message from eccejobmonitor", with only the run log in
`Outputs/` (#470) and the job's own status (231) never recorded.

Now the open block is dropped (`MsgSendBlockDrop` puts back the complete
block of that type it replaced), a `warn` is sent and the monitor goes on.
This is the one completion path for every code and every deployment mode:
the monitor runs on the compute host and the job store on the client in
local, central-server and shared-broker setups alike.

Still true: a monitor restart for any *other* fatal error resends progress
from the bookmark only, so the stored snapshot shrinks to what follows it.
`tests/launch/failed_output_test.py` (ctest `launch_failed_output`) runs the
real monitor on a job that ends mid-block with status 231.
