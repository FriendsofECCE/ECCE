---
type: rule
title: "Test suites get a per-run state directory, ports and X display"
area: services
section: "Pitfalls"
paths: ["tests/apps/isolate.py", "tests/apps/xdisplay.py", "tests/launch/harness.py", "tests/teaching", "tests/queues", "tests/apps"]
issues: [220]
---
**Nothing a suite starts may have a fixed path, port or display number.**
Two runs at once (two worktrees, or `ctest` beside a manual run) that
shared `~/.cache/ecce-teach-state` or ports 8596/8688 killed each other's
services (`killLeftovers` matches by state path) or failed with "Address
already in use".

- **State**: `isolate.runState(tag)` makes `~/.cache/ecce-<tag>-<random>`
  (mkdtemp; never `/tmp`, which is RAM), writes a `.ecce-test-run` marker
  with the owner's pid and start time, and removes the directory (and
  anything still running from it) at exit. `ECCE_TEST_KEEP_STATE=1` or a
  suite's `--keep` leaves it. `ECCE_TEST_STATE` still wins and is never
  deleted. A killed run leaves its directory; the next run of the same tag
  reaps those whose marker's owner is dead, and nothing else.
- **Ports**: `isolate.freePort()` binds port 0 and reads the number;
  `ECCE_DATASERVER_PORT`/`ECCE_BROKER_PORT` pin one explicitly. Do not
  derive one port from another (`+10`); ask for each.
- **Display**: `xdisplay.Display()` uses `Xvfb -displayfd`; only
  `ECCE_TEST_XDISPLAYS=a-b` falls back to scanning that range.
- **Liveness (#220)**: `Display.responsive()` scales its timeout with the
  load average, retries, and `probeNote` reports the wait and load, so a
  slow machine is not reported as a dead X server.
- **Not covered**: `tests/launch/remote_test.sh` and `tests/transport/sshd`
  (containers; ports and the client home are env-selectable), `tests/hosts`
  (containers, fixed inside them).
