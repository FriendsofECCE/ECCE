---
type: pitfall
title: "A cancelled job's program outlived scancel: the job shell died on TERM and the program left linuxproc's tree (#855)"
area: services
paths: ["scripts/gensub", "scripts/ecce-rundir-sweep", "src/comm/commtools/RunMgmt.C", "src/apps/organizer/CalcMgr.C", "src/apps/launch/ecce-terminate.C"]
issues: [855]
---
With `ProctrackType=proctrack/linuxproc` (the Debian test Slurm machine) or
`proctrack/pgid`, Slurm finds a job's processes by walking the tree from the
job shell. scancel sends TERM to all of them, then KILL after `KillWait` to
what it still finds. The job script trapped only INT, so the sh job shell
died on TERM at once; a program that survived the TERM (CREST/xtb catch it)
was reparented to init, and the KILL missed it. With `proctrack/cgroup`
everything in the cgroup dies and this does not happen.

The job script now traps TERM as well: the shell waits for the program, so
the program stays in the tree for the KILL, and a program that obeys TERM
ends the run with 302. After the cancel, `RunMgmt::terminate` sends
`scripts/ecce-rundir-sweep` over the job's own login: it TERMs, then KILLs
after 10 s (`ECCE_TERMINATE_GRACE`), the user's processes whose cwd is the
run directory or below, sparing `eccejobmonitor` and its children, and its
one-line answer becomes the tail of the terminate message. It reads /proc,
so only Linux hosts are checked; from a login node it cannot see compute
nodes, and for a Slurm job whose `sacct` NodeList is another host it says
so ("remote") rather than "no process left". Test:
`tests/launch/terminate_leftover_test.py` (emulates linuxproc's TERM/KILL).
