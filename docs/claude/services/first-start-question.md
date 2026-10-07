---
type: map
title: "The first-start question: where a user keeps their work"
area: services
section: "The Gateway window and session end"
paths: ["packaging/gateway/ecce-first-start", "packaging/ecce.in", "packaging/gateway/ecce-session-lib.sh", "packaging/dataserver/ecce-remote-setup", "include/util/RemoteServerDir.H", "src/apps/organizer/CalcMgr.C", "tests/apps/first_start_test.py", "tests/tls/first_connect_test.py"]
issues: [240, 216]
---
`ecce` runs `ecce-first-start` before it makes the session id (#240). It
asks once ("Work on this computer" / "Connect to a server") only when
`skip_reason()` finds nothing set up; every existing setup, and every
deployment mode, skips it. The skip list is the contract: add a new kind
of configuration there, and to `tests/apps/first_start_test.py`.

- The choice is stored as state ECCE already reads: local = the LocalData
  preference (`ecce-localdata pref on`) plus `~/.ECCE-local`; server =
  `~/.ECCE/RemoteServer/` (same layout as `siteconfig/RemoteServer`),
  written by `ecce-remote-setup HOST --user --auto`. Choosing local moves
  that folder to `RemoteServer.off`; choosing a server turns the preference
  off. There is no file of its own.
- `ecce_user_server_mode` (ecce-session-lib.sh; `ecce` and every wrapper)
  turns the folder into a `-remote` session by exporting
  `ECCE_REMOTE_SERVER=1` and `ECCE_REMOTE_DIR`. An installation's own
  `siteconfig/RemoteServer` wins, and `ECCE_LOCAL_DATA`/`--local` win over
  both. Everything that read `siteconfig/RemoteServer/*` now goes through
  `remoteServerDir()` (RemoteServerDir.H) or `${ECCE_REMOTE_DIR:-...}`: a
  new reader of those files must too, or it ignores a user's own server.
- `--auto` tries `--tls --system-ca`, then `--tls --fetch-pin`, then plain
  http, and takes the first that connects. `--user` makes an unreachable
  server an error instead of the "writing anyway" warning.
- Tests set `ECCE_FIRST_START_ANSWER` (`local`, `server:host[:port]`) and
  `ECCE_FIRST_START_SHOT`; never synthetic input. `tests/apps/session_end.py
  first-local first-server` drive it through the real `ecce`.
