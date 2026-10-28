---
type: pitfall
title: "A host that accepts TCP but never sends the ssh banner: bounded connect, no libssh retry, never on the GUI thread"
area: services
paths: [src/comm/rcommand/RCommand.C, src/comm/rcommand/SshTransport.C, src/comm/rcommand/OpenSshTransport.C, src/apps/organizer/ReconnectSelected.C, src/apps/organizer/CalcMgr.C, src/comm/commxt/JobCatchUp.C, src/comm/commtools/RunMgmt.C, tests/transport/testSilentHost.C]
issues: []
---
A machine whose sshd hangs (port 22 accepts, no banner) costs the full
connect timeout per login: OpenSSH's `ConnectTimeout` covers the banner,
libssh's `SSH_OPTIONS_TIMEOUT` likewise (it takes a `long*`; an `int*`
happened to work on little-endian amd64 only because libssh masks it).
Both are `RCommand::connectTimeout()` (60 s, `ECCE_SSH_CONNECT_TIMEOUT`).

- **No libssh retry after OpenSSH found the host silent.** The fallback in
  `RCommand::sshConnect` is for an ECCE-owned master that failed for some
  other reason; after a timeout/refused/unknown-name error
  (`RCommand::hostDidNotAnswer`) it only doubled the wait (120 s per login).
  When it does fall back, stderr says why:
  `ecce: ssh to H failed (<ssh's first line>); trying the built-in client`.
- **The Organizer never connects on its GUI thread.** Reconnect, Reconnect
  Selected, Check Job on Machine and the session-start catch-up open the
  login with `ReconnectSelected::openLogin`: a detached worker thread runs
  `RCommandLogin::open()` (plain strings from the registry, gathered on the
  GUI thread), the GUI pumps a progress dialog with Cancel, and a Cancel
  abandons the worker (it deletes its late RCommand). The open login is
  handed to `Launch::adoptConnection`, so this costs no extra login.
  `JobCatchUp::reconnect` logs in *before* deleting the old properties and
  output files, so a host that does not answer leaves them in place.
  Before: one frozen minute or two per job, Cancel only between jobs.
- **The work after the login is off the GUI thread too** (#666). Reconnect,
  Reconnect Selected and the catch-up run `moveJobMonitoringFiles` and the
  job store's `chmod` (`Launch::prepareJobStore`) through
  `ReconnectSelected::runRemote`; the data-server writes, the local
  `eccejobmaster` start and `updateState` stay on the GUI thread. The
  catch-up dialog has Cancel; a Cancel leaves that job and the rest
  Waiting with the reason. Over libssh each command was already bounded
  (`SshTransport::runImpl`: an idle deadline of the RCommand timeout, 30 s;
  sftp by `SSH_OPTIONS_TIMEOUT`), so a slow login node cost minutes on the
  GUI thread, in `ssh_select` (the loop's 50 ms poll), not a hang.
  Check Job on Machine still runs its status commands on the GUI thread.
- Test: `transport_silent_host` (ctest; a listener that never writes, a
  private ssh config through a wrapper `ssh -F`, 5 s timeout).
