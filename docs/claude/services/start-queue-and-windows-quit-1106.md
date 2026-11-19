---
type: pitfall
title: "Two tool starts in 100 ms: the second stayed queued forever; Windows Quit left organizer.exe lingering behind a separate tool (#1106)"
area: services
section: "Pitfalls"
paths: ["src/wxgui/jms/WxJMSMessageDispatch.C", "src/apps/organizer/CalcMgr.C", "src/apps/organizer/CalcMgrApp.C", "src/util/jms/MqttBroker.C", "tests/apps/separate_viewer_test.py"]
issues: [1106, 990]
---
- **Start queue.** `getAppMCB` starts the one-shot `p_toAppExec` timer only
  for a newly queued packet and `appExec` starts one packet per shot, so a
  request that arrived within 100 ms stayed `execStarted=false`; the
  same-name check then answered "ok" to every later request for that tool.
  `restartExecTimerIfWaiting()` (end of `appExec`, `completeStartupProcess`,
  the same-name branch) starts a new shot while a packet waits.
  `tests/apps/separate_viewer_test.py` sends both starts in one write
  (`ctest -R apps_separate_viewer`).
- **Windows Quit.** No gateway runs there, only the gateway's `ecce_quit`
  closes a separate tool, and `lingerForSession(10)` waited for them with no
  limit. When `MqttBroker::hostsSession()` (the Organizer made the broker:
  Windows local only; Linux/macOS adopt `eccebroker`, `-remote` and
  SharedBroker never set `ECCE_LOCAL_BROKER`) Quit publishes it through
  `CalcMgr::publishGatewayQuit()`, the code of the `gatewayquit` hook, and
  the linger ends after 15 s at most. On macOS the Organizer's Quit
  publishes it unconditionally (#1120): the gateway there outlived Quit as
  long as a separate Builder was open, as on Linux, where that is wanted
  (`session_end.py` builder case). A job store hearing `ecce_quit` only
  closes its messaging (`ecceExitMCB`), the job and its stored state go on.
