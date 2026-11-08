---
type: pitfall
title: Offload hosts have 6 physical cores and no memory enforcement; overlapping builds swap them to a standstill
area: services
paths: [tools/offload/run-offload.sh, tools/offload/remote-run.sh]
issues: []
---
NerdEgg and radium each have 6 physical cores (nproc shows 12 threads); NerdEgg
31 GB RAM, radium 15 GB. Slurm on NerdEgg counts memory (CR_Core_Memory,
RealMemory=30000) but does not enforce it (TaskPlugin=task/none, no cgroups), and
a job without --mem is booked the whole node. On 2026-11-07 a deb build
(ninja -j6, then cpack of the 160 MB client deb) plus a ctesti run (second,
install build at -j6 outside Slurm) overlapped: 24 GB went to disk swap, the host
stopped answering ssh for hours, and a forced restart was needed. No OOM kill
happened: swap absorbed everything.

Rules since then (run-offload.sh 6568d9da1, c5305d274): one job per host,
JOBS=4, the install build goes through BUILDWRAP (Slurm), NerdEgg builds ask
`srun -c 4 --mem=12G`. Never build or package on these hosts outside
run-offload.sh; never more than 4 compile jobs; check for own running jobs
before queueing. Proposed host-side protection: earlyoom, vm.swappiness=10,
Slurm task/cgroup + proctrack/cgroup + DefMemPerCPU with ConstrainRAMSpace.
