---
type: pitfall
title: "Windows local jobs: paths in sh scripts, the job id, Strawberry perl, the broker"
area: services
paths: ["tests/windows", "packaging/windows/bundle-shell.sh", "scripts/eccejobmonitor", "src/comm/rcommand/DirectTransportWin.C", "src/comm/commtools/RunMgmt.C", "src/util/jms/MqttLink.C"]
issues: [133]
---
- **Backslashes in `ECCE_HOME`, `ECCE_TMPDIR`, `ECCE_LOCAL_DATA`** reach `sh`
  scripts (`test -d`, `cp -f`), where `\U` is an escape: "directory does not
  exist" / "unable to copy parse descriptor". Export them with `/`
  (`runapp.ps1` and `launch_local.py` do).
- **The bundled `sh` needs `<install>\etc\fstab` and `<install>\tmp`**
  (`bundle-shell.sh` writes them); without the fstab drives mount at
  `/cygdrive/c`, not `/c`.
- **Strawberry perl is not MSYS perl** (`$^O eq 'MSWin32'`): no `getppid`,
  no `getpwuid`, no `/proc`. `eccejobmonitor` guards each; add the next one
  here when it fails. The monitor that runs is the copy staged into the run
  directory, so re-launch after editing it.
- **Strawberry's `c\bin` must come after `C:\msys64\ucrt64\bin` in PATH**:
  its own libstdc++/libgcc make our exes exit silently at start.
- **Cancel does not end the job's processes.** The job id is the Windows pid
  of the `sh -c "nohup ./submit"` wrapper, which is gone by the time the
  script runs (MSYS exec gives the new process another WINPID); the real
  `submit`, stub and `sleep` have other parents, so `taskkill /T /PID id`
  finds nothing. The MSYS process group does hold them all
  (`ps -W`: PGID column; `kill -9 -- -PGID` ends them).
- **No local broker on Windows yet:** mosquitto for Windows rejects
  `listener 0 <socket>`, and MqttLink's TCP mode needs the data-server login.
