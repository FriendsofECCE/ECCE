---
type: pitfall
title: "An askpass that fails makes ssh send an empty password: never use /bin/false or a silent exit as 'do not ask'"
area: services
paths: [scripts/ecce-askpass, src/dsm/edsiimpl/SftpEDSI.C, src/comm/rcommand/OpenSshTransport.C, src/comm/rcommand/RCommand.C, src/apps/passdialog/PasswordDialogApp.C, tests/apps/fakessh/ssh, tests/apps/ssh_password_refused_test.py]
issues: [1274, 1272]
---
OpenSSH's `read_passphrase` returns `""` when the askpass program exits
non-zero, and `userauth_passwd` sends it. With `NumberOfPasswordPrompts` 3,
an askpass that cannot or will not answer costs the server three failed
passwords; after the agent's keys those use up `MaxAuthTries` ("Permission
denied, please try again." twice, then "Too many authentication failures"),
and on a cluster with fail2ban they count against the user.

- **"Do not ask" is `OpenSshTransport::NoPrompt`, per thread.** SftpEDSI's
  background logins (listing, desk, stamp, copies; everything but
  `probe()`) used `ECCE_ASKPASS=/bin/false` in the process environment:
  `/bin/false` answered every prompt with `""`, the variable leaked into
  logins on other threads (#1272 runs SSH-folder connects on a worker while
  the GUI thread and the job catch-up log in), and a cached login's
  transport kept whichever askpass it was created with. Under `NoPrompt`
  no master is opened through askpass (`wantsMaster`, `nextOverflowMaster`),
  no libssh fallback and no passdialog/hostkeydialog: the BatchMode=yes
  attempt's failure is the answer, and no `.outcome` is written, so the
  next real connect is not blocked for 120 s.
- **`ecce-askpass` ends ssh rather than answer nothing** (`stop_ssh`: kills
  `$PPID` when ECCE started the login and the parent is `ssh`): after a
  Cancel, a remembered cancel, or no passdialog. The cancel marker alone
  stopped the dialogs but not the empty passwords.
- **A second password prompt for the same login means the last answer was
  refused**: askpass passes `ECCE_PASSDIALOG_NOTE` and the dialog says so.
  Same login = the `fed` marker (FIFO password) or `typed` holding the same
  `user@host` (a jump host's prompt comes first on some routes).
- **Test:** `tests/apps/ssh_password_refused_test.py` with the
  password-only mode of `tests/apps/fakessh/ssh`, which sends `""` on an
  askpass failure as OpenSSH does. A stub that skips that step hides the bug.
