---
type: map
title: ecce-launch --move cancels a pending Slurm job through the terminate path and relaunches (#830)
area: services
paths: [src/apps/organizer/CalcMgr.C, src/apps/launch/ecce-launch.C, tests/launch/ecce_launch_move_test.py]
issues: [830]
---
`CalcMgr::moveLaunchForCommand` checks the queue with `RunMgmt::checkOnMachine`
(Check Job on Machine), requires a Slurm `squeue` line with ST `PD`, then calls
`terminateForCommand`. The job monitor records that cancel as Killed, so the
code waits for the state to leave Submitted and for `stopOldMonitor`, and only
then resets to Ready (job id/path, submit date, submituncertain, reason) and
calls `launchForCommand`. Launch settings are kept for `carryOver`.
The "moved from A to B" line goes into the run log before the new submit; the
new job store moves the old log to `.run_log.last`, so look there too.
Other queue managers are refused: only Slurm's pending state is parsed.
