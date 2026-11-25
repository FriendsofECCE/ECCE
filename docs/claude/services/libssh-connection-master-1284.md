---
type: map
title: "eccesshmaster: one libssh login per route, shared with job stores as separate programs (#1284)"
area: services
section: "Pitfalls"
paths: ["src/apps/sshmaster/eccesshmaster.C", "src/comm/rcommand/SshMaster.C", "include/comm/SshMaster.H", "src/comm/rcommand/SshTransport.C", "src/comm/rcommand/RCommand.C", "src/comm/commxt/Launch.C", "tests/launch/run_tests.py", "tests/transport/sshd/monitor_login_test.sh", "tests/transport/detachedEnv.C"]
issues: [1284, 1283, 1281, 204, 599]
---
**A single-connection or front-end machine on the built-in ssh client keeps
one login through `eccesshmaster`**, so the job store runs as a fresh
`eccejobmaster` everywhere. Before, `Launch::startJobStore` fork()ed to hand
the authenticated libssh session on; Windows has no fork() and the launch
failed after the submit.

- **Processes.** Launch (and Reconnect, which goes through Launch) opens its
  `RCommand` with `shareLogin` for `frontendFlag` machines. On libssh,
  `RCommand::sshConnect` uses a live master for the route, else starts
  `eccesshmaster` (detached), hands it AuthCache over the usual auth pipe and
  waits on a status file (`started`, then `ok` or
  `cancelled|refused|unreachable|failed` + message). The master logs in with
  an ordinary `RCommand` (`ECCE_SSH_MASTER=serve`, libssh forced), so the
  dialogs are passdialog's, asked once. Every libssh `RCommand` of the user
  then uses a live master for its route (`SshTransport::setMasterEndpoint`:
  `run()` and streams go to the master; files go by exec, as on a nested
  front end).
- **Endpoint.** Hash of user, host, port, front end and `ECCE_REALUSERHOME`:
  POSIX `$XDG_RUNTIME_DIR/ecce-sshm-<uid>/<16 hex>` (macOS `$TMPDIR`, else
  `/tmp`), directory 0700 and checked for owner, mode and symlink, socket
  0600, peer uid checked (`SO_PEERCRED`/`getpeereid`); Windows
  `\\.\pipe\ecce-sshm-<16 hex>` with a DACL for the user's SID only,
  `FILE_FLAG_FIRST_PIPE_INSTANCE`, `PIPE_REJECT_REMOTE_CLIENTS`, and the
  client refuses a server process of another SID (pipe names are first come).
  A `.lock` (POSIX flock, Windows named mutex) makes one login per route.
- **Protocol** (`SshMaster.H`): `HELLO` (a held-open client), `RUN` (framed
  stdout/stderr/exit), `STREAM` (raw both ways; the client's descriptor is
  the connection itself, a CRT descriptor on the pipe on Windows, which
  `winPoll`'s PeekNamedPipe reads), `SIGNAL`. Commands, scripts and file
  contents cross it; no password or code does.
- **Lifetime.** One thread owns the session (non-blocking, polled). It ends
  `ECCE_SSH_MASTER_IDLE` (300) s after its last client, or when the session
  drops. A job store started for a shared login gets
  `ECCE_SSH_MASTER_REQUIRE=<endpoint>`: with no master it does not log in by
  itself; it reports "ECCE's shared login ... has ended ... Use Reconnect"
  as unreachable (Waiting), and Reconnect logs in through the GUI's dialogs.
- **Fallback.** If the master cannot be started for a reason other than the
  login (no private directory, program missing), POSIX logs in directly and
  forks as before (#1283's `MqttLink::reclaimAfterFork` still matters
  there); Windows refuses in `validateRemoteLogin`, before the submit. A
  cancelled or refused master login is final (no second dialog, #599).
  `ECCE_SSH_MASTER=off` turns it all off. OpenSSH machines are untouched
  (#1281: ECCE's ControlMaster).
- **Pitfall (Windows).** The job store is started through
  `RCommand::execbg()`, which runs `nohup <command>` with no shell between,
  so a `VAR=value` prefix is taken for the program name ("nohup: failed to
  run command 'ECCE_SSH_MASTER_REQUIRE=...'"), and the job sat at submitted.
  The prefix goes through `RCommand::withEnv` (`env`, shipped in the
  bundle's `usr/bin`). Linux and macOS start it with `system()`, where the
  shell reads the prefix. Test: `ctest -R transport_detached_env`.
- **Pitfall.** `tests/transport/sshd/build_test.sh` and `frontend_test.sh`
  compile `SshTransport.C` by hand: they need `SshMaster.C` too.
- **Test.** `ECCE_MONLOGIN_SINGLE_ONLY=1 tests/transport/sshd/monitor_login_test.sh`
  (`run_tests.py --single-connect`): the job completes, its store ran under
  eccejobmaster, and sshd saw one login for launch + store. With
  `ECCE_SSH_MASTER=off` (the fork path) the same run fails.
