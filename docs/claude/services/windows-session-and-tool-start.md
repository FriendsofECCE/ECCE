---
type: map
title: "Windows: the session broker, and the Organizer starts the tools"
area: services
paths: ["packaging/windows/ecce.cmd", "packaging/windows/ecce-broker-win", "src/wxgui/jms/WxJMSMessageDispatch.C", "src/tdat/resources/AuthCache.C", "src/util/jms/MqttLink.C", "src/apps/organizer/CalcMgrApp.C", "tests/windows/organizer-probe.ps1"]
issues: [133, 194, 213]
---
Windows runs no gateway process and has no `fork()`.

- **Broker.** `ecce.cmd` makes an `ECCE_SESSION_ID` and runs
  `ecce-gateway-start` in both modes. A local session gets `ecce-broker-win
  start`: mosquitto on 127.0.0.1 at a free port, `allow_anonymous false`, a
  user and password made up for the session, the central broker's ACL. The
  login is in the session's broker file under `~/.ECCE` (the #213 D3 design);
  a `-remote` session gets the central broker's file instead.
  `ecce-broker-win watch`, started by `ecce.cmd`, stops the broker once no
  ECCE window (organizer, builder, calced, ... by image name) is left; job
  monitors do not keep it up.
- **Tool start.** Tools send `ecce_get_app` addressed to `Gateway`. On Windows
  the Organizer calls `registerMyselfAsAppExecer()` and its endpoint also
  takes that one topic addressed to `Gateway` (`MqttEndpoint::answerAs`).
  `startToolWin` runs `<ECCE_HOME>\bin\<InvokeArg>.exe` with CreateProcess, no
  shell; `ecce-viewer` is `builder.exe` with `ECCE_INVOKE_VIEWER` and
  `ECCE_INVOKE_FROMECCE` set. Reuse of an open tool is the Linux poll over the
  broker, unchanged. Child exit is checked with `OpenProcess`/`WaitForSingleObject`
  (`waitpid` is a stub there).
- **Login cache to the tool.** An anonymous pipe; only its read end is in the
  child's `PROC_THREAD_ATTRIBUTE_HANDLE_LIST`, and the child gets
  `-pipe handle:<n>` (a handle number, no secret). `AuthCache::pipeOutHandle`
  writes the cache and closes the write end; `pipeIn` reads the handle. No
  password on a command line, in the environment or in a file.
- **Still a file:** `pipeOut(name)` on Windows (Launcher -> eccejobmaster ->
  eccejobstore, which go through `sh`) writes the cache to a temp file that
  `pipeIn` deletes after reading. Moving those to handles needs the handle to
  survive the MSYS `sh` in between.
- The Organizer is the only starter: tools open while it runs. Closing it
  leaves open tools working, but they cannot start others.
- `tests/windows/organizer-probe.ps1` starts the broker as `ecce.cmd` does and
  opens CalcEd, Builder (from CalcEd, then reused), Basistool, Launcher,
  MachineRegister and the Viewer from the Organizer's `start` test command.
