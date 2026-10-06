---
type: map
title: "`eccejobmonitor` polls every 2 s for a local job without a queue manager, 10 s otherwise"
area: codereg
section: "Machine configuration (CONFIG files)"
paths: ["src/comm/commxt/Launch.C", "scripts/eccejobmonitor", "tests/launch/run_tests.py", "tests/queues/run_tests.py"]
---
The monitor's pauses (`timePauseFileExist`, `timePauseJobExist`,
`timePauseReadLine`, `timePauseReadLinePartial`) default to 10 s in the
script. `Launch` writes them as 2 s into `eccejobmonitor.conf` only when the
queue manager is Shell and `RCommand::isRemote()` says the machine is this
computer for this login; every other job keeps the script's defaults, since
there each check is a scheduler query or runs on a shared login node. The
pause bounds how late a state change (running, completed) reaches the data
server. `tests/launch` checks the local conf and prints the time of each
state change; the queue suite checks that a scheduler job's conf has no
pause lines.
