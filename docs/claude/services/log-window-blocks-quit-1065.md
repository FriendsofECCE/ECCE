---
type: pitfall
title: "A wx log window at start-up blocks Quit, and a test that quits during the example import times out (#1065)"
area: services
section: "Pitfalls"
paths: ["src/wxgui/jms/OneInstance.C", "tests/containers/guest.py", "tests/containers/central_server_test.py", "tests/apps/session_end.py"]
issues: [1065, 1069, 983, 233]
---
- `wxRemoveFile` of a file that is not there calls `wxLogSysError`. In a
  GUI process the default log target shows that as a modal "ECCE Error"
  window at the next idle. `OneInstance::claim` cleared a stale socket that
  way, so every first one-program start showed "File '/tmp/ecce-one-<uid>/
  <key>.sock' couldn't be removed". While that window was open the host
  window was disabled: the window manager's close did nothing, so Quit
  never came. The central-server container test found it ("alice quits"
  failed, with an 'ECCE Error' window). A file that may be missing gets
  `unlink()` or a `wxLogNull` scope, never a bare `wxRemoveFile`.
- The first-start example import shows an application-modal progress
  window, and it ignores a close until it ends (2-4 minutes on a loaded
  host). `guest.py close` waits for "Example calculations" to go before it
  closes. Without that, Quit fails whenever the import is still running,
  with or without `ECCE_ONE_PROCESS`.
- `session_end.py` two-sessions (#233) starts two `ecce` of one user on
  one display, on purpose. In one-program mode (#1069) the second start
  only brings the first window forward, so those sessions set
  `ECCE_MULTIPLE_INSTANCES=1`. `$DISPLAY` stays out of the single-instance
  key: session state is keyed by `ECCE_SESSION_ID`, and the display only
  draws windows.
