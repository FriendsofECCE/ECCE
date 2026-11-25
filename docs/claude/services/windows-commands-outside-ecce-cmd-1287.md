---
type: pitfall
title: "Windows: an ecce-* command from a Command Prompt has no socket and no ecce.cmd environment"
area: services
paths: ["src/apps/launch/", "src/util/genutil/Ecce.C", "src/util/jms/MqttLink.C", "include/util/JMSSubscriber.H", "packaging/windows/ecce.cmd", "tests/session/command_env_test.C"]
issues: [1287]
---
Two gaps made every Organizer-request command (`ecce-reparse`, `-rerun`,
`-launch`, `-terminate`, `-annotate`, `-fetch`, `-move`, `-reconnect`,
`-workflow`) fail on Windows:

- They refused with "ECCE is not running" when `JMSSubscriber::getSocketID()`
  was -1. `MqttEndpoint::enablePipe()` never makes a pipe on Windows, so the
  id is always -1 there. The commands poll `processMessage()`, which drains
  the endpoint's queue, so they need no socket; the check is gone. Only a
  `poll()` loop (the job store) needs the socket.
- Started outside `ecce.cmd`, `ECCE_HOME` is unset (fatal assert),
  `ECCE_REALUSERHOME` too, and `LocalData::dir()` is empty, so a folder
  argument is refused. `Ecce::commandEnvironment()`, called first in each
  command's `main`, fills in only what is unset, in ecce.cmd's order:
  `ECCE_HOME` from the program's own path (`<home>/bin/x.exe`),
  `ECCE_REALUSERHOME` from `USERPROFILE`, then a server session
  (`ECCE_REMOTE_SERVER`, `ECCE_REMOTE_DIR`) if a `DataServers` file exists,
  else `ECCE_LOCAL_DATA` = the preference's folder. On Linux and macOS it
  only sets `ECCE_REALUSERHOME` from `HOME`; the installed wrappers set the
  rest. A session started with `ecce --local` while a `DataServers` file
  exists is not inferred: set `ECCE_LOCAL_DATA` in the prompt.

Test: `ctest -R command_env` (Windows; the reply part also runs on Linux).
