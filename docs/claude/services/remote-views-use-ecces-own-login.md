---
type: pitfall
title: "A remote view must ride ECCE's own login, not start an ssh of its own"
area: services
section: "Pitfalls"
paths: ["src/comm/commtools/TailSource.C", "src/comm/commxt/JobStore.C", "src/comm/commxt/Launch.C", "src/tdat/resources/AuthCache.C", "src/wxgui/comm/WxTailWindow.C", "src/comm/commtools/EcceShell.C", "src/comm/rcommand/SshTransport.C", "src/apps/organizer/CalcMgr.C"]
issues: [204]
---
**Anything that shows a remote file runs over the RCommand connection that
logged in, never through a terminal's own `ssh`.** Run Mgmt > Tail used to
log in through RCommand (AuthCache, then passdialog) only to check the file,
then start `xterm -e ssh -t host 'tail -f ...'`: an OpenSSH client that
knows nothing of the libssh session, so a password-only cluster asked twice
and a two-factor one wanted a second code.

Tail now opens `WxTailWindow`, fed by `TailSource`: one `RCommand`, a file
check, then `startStream(script, true)`, which on libssh is
`SshTransport::openStreamOnLogin` -- an exec channel on the session that
just authenticated (the ordinary `openStream` opens a session of its own and
logs in again; nothing in ECCE uses it any more). Until `closeStream()` that
session belongs to the stream's pump thread, and `run()`/SFTP on it fail
with "busy". On OpenSSH (a shared connection) and locally (DirectTransport)
the plain stream already shares the login. The script ends `tail` when its
stdin closes (`cat >/dev/null; kill`), so a closed window or a dropped link
leaves nothing running on the login node.

The terminal route remains behind the `TailInTerminal` preference
(EcceGlobal) or `ECCE_TAIL_TERMINAL=1`; it passes ECCE's ControlPath
(`OpenSshTransport::controlArgs`) to the terminal's ssh, so a connection
ECCE shares is reused, but over libssh it still logs in again.
Tests: `transport_tail`, `transport_tail_window` (ctest),
`tests/transport/sshd/tail_test.sh` (one login per Tail, counted).

The job monitor follows the same rule. eccejobstore (one process per job,
restarted by eccejobmaster) streams eccejobmonitor with
`startStream(cmd, true)` on the login `initConn()` made, and copies the
output files at the end with `RCommand::copyOnLogin` over that login too;
only if that copy fails does it log in again (`RCommand::get`). So one run
of eccejobstore is one login; a dropped connection ends the run, and the
restarted one logs in once. While the stream runs, `remoteconn` can only be
interrupted (`exec("\003")`); `cleanup()` stops the stream before its
`rm`. Launch copies its files with `copyOnLogin` too (`putOnLogin`), so a
job costs two logins: Launch's and eccejobstore's, each its own process.
eccejobstore sets `AuthCache::LAST_URL`; that fallback takes the newest
credential *of the same kind*, because `initConn()` caches the machine's
password (`ssh://`) before `initDAV()` asks for the data server's. Test: `tests/transport/sshd/monitor_login_test.sh` (sshd's
log counts the password logins per job).
