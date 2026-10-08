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
- **The job id is the MSYS pid, not a Windows pid.** `spawnDetached` runs
  `setsid sh -c "$ECCE_DETACH_SCRIPT" & echo $!` inside a short-lived sh, so the
  job is its own process group leader and `$!` names it for as long as it runs.
  The Linux cancel (`kill -TERM -- -pgid`, `ps -p`) then works; MSYS `ps` has no
  `-o`, so `RunMgmt::terminate` rewrites the `ps -o pgid` part to `-##id##` (pgid
  == id by construction). `eccejobmonitor` checks the id with `ps -p`. A Windows
  pid names a wrapper that is gone before the job script runs; `ps -W` WINPIDs and
  `taskkill /T` found nothing. `setsid` must be in the bundled `usr\bin`.
  Native (non-MSYS) children of the job are not in the MSYS group and are not
  reached by `kill`.
- **Closed loopback ports hang `/dev/tcp`.** MSYS bash connecting to a port
  nobody listens on sits in SYN_SENT; probe with `netstat -an | grep LISTENING`.
- **Local broker: loopback TCP with a per-session login.** Windows mosquitto
  rejects `listener 0 <socket>`. `packaging/windows/ecce-broker-win start`
  (called by `ecce-gateway-start` on MSYS for a local session) starts mosquitto on
  127.0.0.1 at a random free port with `password_file` and the central broker's
  ACL under `~/.ECCE/broker_<key>.d/`, and writes `user=`, `password=`, `host=`,
  `port=` to the broker file; `MqttLink` uses that login instead of the data
  server's. `stop` kills it. Nothing yet calls `stop` at session end on Windows.
  A denied subscribe is reported as granted over MQTT 3.1.1 and by mosquitto
  here even over 5; test the ACL with a QoS 1 publish (PUBACK 135).
- `ECCE_HOME`, `ECCE_TMPDIR` and `ECCE_LOCAL_DATA` are rewritten to forward
  slashes at startup (`Ecce.C`), whatever the user exported.
