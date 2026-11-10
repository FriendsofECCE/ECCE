---
type: pitfall
title: "Job folders under $ECCE_TMPDIR/jobs: who removes them, and when"
area: services
paths: ["src/util/genutil/TempStorage.C", "src/apps/jobstore/eccejobmaster.C", "src/comm/commxt/Launch.C", "src/comm/commxt/JobParser.C", "src/comm/commxt/JobCatchUp.C", "src/apps/organizer/CalcMgrApp.C"]
issues: [924]
---
- Launch's staging folder (`<calc>__XXXXXX`) is NOT done when the files
  reach the run directory: eccejobmaster/eccejobstore read
  `eccejobstore.conf` and the `.desc` files from it and log into it for
  the job's whole life. Local jobs run in their run directory, not here.
- eccejobmaster removes the staging folder, and an import's or re-parse's
  monitor folder (`-importDir`), only when the store exits 0 with no
  restart. A failed or killed monitor leaves them; the Organizer's
  start-up sweep (`TempStorage::sweepStale`, `launchjob sweeptmp`) removes
  them after 3 days unless the calculation's watch lock is held or (Linux)
  a live process names the folder.
- Before the store starts, `~Launch` and `TempStorage::JobDirGuard` remove
  the folders on every return path.
- On Debian 13 /tmp is RAM: a leak here is memory, not disk.
