---
type: map
title: "The first-start question: where a user keeps their work"
area: services
section: "The Gateway window and session end"
paths: ["packaging/gateway/ecce-first-start", "packaging/ecce.in", "packaging/gateway/ecce-session-lib.sh", "packaging/dataserver/ecce-remote-setup", "include/util/RemoteServerDir.H", "packaging/windows/ecce.cmd", "packaging/macos/ecce.sh", "tests/apps/first_start_window_test.py", "tests/windows/ecce_cmd_test.py", "src/apps/organizer/CalcMgr.C", "tests/apps/first_start_test.py", "tests/tls/first_connect_test.py"]
issues: [240, 216]
---
`ecce` runs `ecce-first-start` before it makes the session id. Since #767
it asks nothing: a client with nothing configured is made local (the
LocalData preference on, `~/.ECCE-local`) and goes on. `skip_reason()` lists
what is left as it is: siteconfig/RemoteServer, SharedBroker, `-remote`,
`--local`/ECCE_LOCAL_DATA, a central-server account, no display,
ECCE_NO_FIRST_START, and a recorded server (`~/.ECCE/RemoteServer`, an
answer "server ..." in `~/.ECCE/first-start-answer`, `~/.ECCE/dataserver`,
the preference off). `--check` prints `local` or `skip: <why>`. The old
window survives as `--change` (tests, `ecce-first-start --apply` for
scripts); Preferences no longer starts it. Preferences > Data lists the
user's servers (`~/.ECCE/ServerList`, ServerList.H, `ecce-localdata servers`)
and shows an installation's or an earlier start's server as read-only;
Remove on the earlier one runs `--apply local`. Added servers are only
stored in a1. The skip list is the contract: add a new kind of
configuration there and to `tests/apps/first_start_test.py`.
Tests that need a server session use `seedChoice` (session_end.py), not
the window's answer hook.

- The choice is stored as state ECCE already reads: local = the LocalData
  preference (`ecce-localdata pref on`) plus `~/.ECCE-local`; server =
  `~/.ECCE/RemoteServer/` (same layout as `siteconfig/RemoteServer`),
  written by `remote_setup()` in `ecce-first-start` (Python, so Windows and
  macOS need no curl/openssl; same result as `ecce-remote-setup HOST --user
  --auto`, whose own TLS pin/CA tests are `tests/tls/first_connect_test.py`,
  now run against the Python code). Choosing local moves
  that folder to `RemoteServer.off`; choosing a server turns the preference
  off. `~/.ECCE/first-start-answer` only records that the question was
  answered.
- `ecce_user_server_mode` (ecce-session-lib.sh; `ecce` and every wrapper)
  turns the folder into a `-remote` session by exporting
  `ECCE_REMOTE_SERVER=1` and `ECCE_REMOTE_DIR`. An installation's own
  `siteconfig/RemoteServer` wins, and `ECCE_LOCAL_DATA`/`--local` win over
  both. Everything that read `siteconfig/RemoteServer/*` now goes through
  `remoteServerDir()` (RemoteServerDir.H) or `${ECCE_REMOTE_DIR:-...}`: a
  new reader of those files must too, or it ignores a user's own server.
- It tries https with a certificate the system trusts, then https pinned to
  the certificate presented now (whole-certificate pin, `server.pem`), then
  plain http, and takes the first that answers; nothing answering writes
  nothing. `--apply local|server:HOST[:PORT]` does the same with no window
  (tests; macOS CI, which has no wxPython to open it).
- All platforms ask (no darwin skip). On Windows the local folder is
  `~/ecce-local` (which `ecce.cmd` made unasked before the question);
  `ecce.cmd` runs the window with
  `python\python3w.exe` and exit 3 quits. A server (installation's
  `siteconfig/RemoteServer`, else `~/.ECCE/RemoteServer`) makes `ecce.cmd`
  export `ECCE_REMOTE_SERVER`, a session id and run `ecce-gateway-start`
  under the bundled bash, which skips its flock/reap on MSYS (neither
  exists there); a local session gets its own loopback broker
  (windows-session-and-tool-start.md).
  Edit > Preferences > Data folder > Change Server... (formerly Edit > Change Server...) starts the script with `python3w.exe` on Windows.
- macOS: ECCE.app bundles no Python or wxPython, so the window is skipped
  silently (ImportError) until it does; local data stays the default there.
- The data server login window's "Use this computer instead..." is no
  longer shown (Andy, 2026-10-10): the start window asks at every start,
  so it was the same question twice. The machinery below is kept (and the
  test hook still presses it), so it can be removed or brought back in one
  place (`WxDavAuth.C`, `showUseLocal`). It ran
  `ecce-first-start --apply local` and ends this session (the gateway
  skips its "Authentication Failure" message, other apps leave their
  main loop). The gateway then exits with status 10
  (`WxDavAuth::EXIT_SWITCHED_TO_LOCAL`, its own `main()`, since a refused
  wx OnInit is always 255) and `ecce` starts once more as `--local` with
  a new session id, in both normal and --bug mode. Restart rather than
  switch in place: the session's broker and -remote state are already
  set up. Only the gateway restarts; another app's login window (and
  Windows' Organizer, no gateway) still says "From the next start". Test hook `ECCE_TEST_AUTH_ANSWER=uselocal[:s]`;
  `tests/apps/session_end.py auth-uselocal`.
- Tests set `ECCE_FIRST_START_ANSWER` (`local`, `server:host[:port]`,
  `continue` = Continue on what is preselected; the window logs
  "preselected local|server") and `ECCE_FIRST_START_SHOT`; never synthetic
  input. The suite's isolated user (`tests/apps/isolate.py`) has a recorded
  answer, so the other cases are not asked. `tests/apps/session_end.py
  first-local first-server` drive it through the real `ecce`.
- "Connect to a server" (#259): the address field is prefilled with the active
  server, else the last one (`~/.ECCE/RemoteServer.off/DataServers`), else
  `localhost` when `ecce-dataserver-start` is in `$ECCE_HOME/bin`. An address
  that is this computer, with the server package and nothing answering on the
  broker port (8088, `ECCE_BROKER_PORT`), is the per-user data server, the
  same state as "here" (no RemoteServer, LocalData preference off), not a
  client connection; a broker answering there is a central server, and it
  connects as a client. Any other address must also answer on the broker port
  (8883 when the setup chose TLS) or it is refused with nothing written.
  Tests that connect to a stub data server need a listener on
  `ECCE_BROKER_PORT`. `ecce-localdata` defaults `ECCE_REALUSERHOME` to `$HOME`
  (USERPROFILE) so the script can call it outside a session.
