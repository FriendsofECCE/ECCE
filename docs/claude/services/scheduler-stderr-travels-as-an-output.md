---
type: map
title: "The queue system's stderr file reaches Outputs through the monitor's end-of-job file list"
area: services
paths: ["scripts/eccejobmonitor", "src/comm/commxt/JobStore.C", "src/comm/commxt/Launch.C", "include/comm/JobFailureReason.H", "siteconfig/submit.site", "tests/launch/scheduler_stderr_test.py"]
issues: [594]
---
Only the Slurm and LSF templates in `siteconfig/submit.site` give stderr a
file of its own (`--error=slurm.err`, `-e $submitFile.err`); PBS, Moab and
SGE join it to stdout (`-j oe`/`-j y`). A site or user `CONFIG.<host>` can
change any of that, so the name is read from the generated script, not
hard-coded.

Path: `Launch.C` writes `submitScript <script>` into the monitor config for
every queue manager except Shell. `JobFilesGet` in `eccejobmonitor` calls
`SchedulerErrSend`, which greps the script in the run directory for
`--error`/`-e`, skips names with `%`, `$` or `/` (`%j` is unknown there),
skips empty files, and sends the rest as a `jmFILE sched` message. The job
store treats `sched` like `output` (stored by name, so it is listed in
Outputs, fetched in the same `interactGetFiles` pass: no extra remote
command) and remembers the name. On an Unsuccessful end `cleanup()` scans it
for a known error and appends " (see slurm.err in Outputs: <last line>)" to
`runStatusReason` (`JobFailureReason::schedulerErrNote`).

Pitfalls: the monitor sends files before it knows the outcome, so the
"success keeps it only if <= 64 kB" rule uses the job's own exit status in
`.ecce.status` (0 = success); a job that exits 0 but is judged Unsuccessful
from its output loses a stderr file over 64 kB. An old monitor on the compute
host never sends `sched`; an old job store ignores it (not stored). Monitor and
job store are one code path for local, central-server and shared-broker modes.
