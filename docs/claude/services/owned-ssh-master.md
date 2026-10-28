---
type: map
title: "ECCE's own ssh master (ECCE_SSH_CONTROL_DIR): one login per host per state directory"
area: services
section: "Pitfalls"
paths: ["src/comm/rcommand/OpenSshTransport.C", "src/comm/rcommand/RCommand.C", "packaging/ecce.in", "packaging/gateway/ecce-session-lib.sh", "packaging/gateway/ecce-gateway-reap", "src/apps/gateway/GatewayApp.C", "scripts/ecce-askpass", "tests/transport/sshd/owned_master_test.sh", "tests/transport/testOwnedMaster.C"]
issues: [487, 599, 631, 666]
---
**With `ECCE_SSH_CONTROL_DIR` set (Linux/macOS), every compute-machine
connection is OpenSSH under a master ECCE owns**, so the Launcher, the job
store and its monitor, tail and the Machine Browser share one login per
host instead of one each, across all sessions of the state directory. Unset, invalid or on Windows: nothing changes.

- **Where.** `ecce` calls `ecce_ssh_control_setup`
  (`ecce-session-lib.sh`): `$XDG_RUNTIME_DIR/ecce-ssh/<8 hex of a hash of
  uid, `ECCE_REALUSERHOME` and `ECCE_HOME`>` -- the **state directory's**,
  not a session's (#631: a job store outlives its session and keeps the
  directory it started with, so per-session directories asked for the login
  once per session after a dropped connection; the uid alone would let two
  people on one Unix account share masters) (tmpfs: works with NFS homes), macOS `$TMPDIR/ecce-ssh/...`, else
  `$ECCE_REALUSERHOME/.ECCE/cm/...`; mode 0700, only a root that keeps the
  path at 66 bytes or less (a socket is bound as `<path>.<16 chars>`, and
  104 bytes is the macOS limit). `OpenSshTransport::ownedControlDir()`
  re-checks owner, mode and "not a symlink" and logs one line when it
  refuses. The socket is `<16-hex hash>` of local host, user, host, port
  and jump host, so a socket never serves another route. `.home` (`ECCE_HOME`) and `.state`
  (`ECCE_REALUSERHOME`) name the owner for the reaper, and `.born` is written once; the reaper leaves directories younger than two
  minutes alone (the directory's own mtime moves with every socket).
  **The name is reused by every session** of the state directory, so `.born`
  alone let an old session's reaper remove it while a new one started
  (#666: the new session then fell back to libssh). Every setup writes
  `.claim`, which counts as `.born` does; the start sweep skips its own
  session's directory; `ecce-gateway-start` sets it up again after the
  sweep; and `ownedControlDir()` makes a missing one again (0700, inside a
  parent that is ours and 0700, with `.home`/`.state`) instead of refusing.
  A `.gateway.lock` not free after 5 s is explained (`ecce_lock_wait`: the
  pids that have it open, and `kill -CONT` for a stopped one); tests
  `session-control-dir-claim`, `session-lock-holder`.
- **Login.** Through `ecce-askpass` (`SSH_ASKPASS_REQUIRE=force`), under an
  `flock` on `<hash>.lock` (the base socket's, whatever overflow master is
  in use). A password the Launcher or AuthCache already has is passed once
  through a FIFO, to the password prompt only (0 dialogs).
  **N processes that lose the master together cost one dialog, whether or
  not it is answered.** The holder writes `<hash>.outcome` (epoch time and
  `ok|cancelled|failed|timeout`) before releasing the lock; a waiter that
  finds the master still dead and a non-ok outcome from after it began
  waiting, or from the last 120 s, does not ask: it reports a cancel
  (`cancelled`, `timeout`: `loginCancelled()`) or a refusal (`failed`:
  `loginRefused()`) and returns false. An unanswered dialog is
  `ECCE_SSH_ASK_TIMEOUT` seconds (default 120). Only the process that
  asked (`loginPrompted()`) writes the user-visible line in the job store;
  the others park the job as Waiting quietly. A cancel, no answer or a
  refused password is never a reason for the libssh fallback
  (`RCommand::sshConnectOpenssh`): that second dialog is what multiplied
  them (#599). Before this, the claim held only for an answer within the
  timeout, and the failure memory was per process.
- **MaxSessions.** One master carries about 10 sessions and a monitor holds
  one for its whole job. A refused session (`Session open refused by peer`,
  `administratively prohibited`) moves the host to the overflow master
  `<hash>-1`, `-2`, ... (up to 8): about one login per 9 jobs. The index
  in use is the file `<hash>.idx`, read and written under the host lock, so
  that every process of the session is on the same master. For monitor
  streams ssh first prints the refusal and then tries a direct connection
  of its own, which is why `openStream` reads the message after 250 ms
  instead of waiting for EOF.
- **Session end.** The directory outlives any one session. The gateway
  sends `ssh -O stop` to each socket when its session ends only if no other
  ECCE program of the state directory lives (`SessionLease::otherPrograms()`;
  running monitor streams go on, no new sessions). The reaper
  (`ecce-gateway-reap` section 1) runs `ssh -O exit` and removes the
  directory once no ECCE program of that state directory lives, in any
  session -- an `eccejobstore` or `eccejobmaster` counts. Directories in the
  old per-session naming (no `.state`) are still cleaned by the old rule: when
  no program of that session lives. An abandoned master is open until then
  (ControlPersist=8h bounds an idle one; `ECCE_SSH_CONTROL_PERSIST` changes
  it). Side effect: an ECCE restarted while another session or a job store
  lives reuses the live master and asks for no login; it is bounded by
  ControlPersist. A `-O stop`ped master that a restarting job store needs is
  logged in again by that job store, and with no display it parks as Waiting.
- **libssh fallbacks.** A host whose front end needs nested ssh
  (`ECCE_SSH_FRONTEND=nested`), and a host whose `ssh -J` was refused for
  forwarding (`stdio forwarding failed`, or `administratively prohibited`
  with no master up), stay on libssh for the rest of the process, with one
  log line (`OpenSshTransport::forwardingRefused`, used by
  `RCommand::sshConnect`). `ECCE_SSH_BACKEND=libssh` is the way out; the
  user's own `ControlMaster`/`ControlPath` wins over ECCE's.
- **Tests.** `tests/transport/sshd/owned_master_test.sh` (private sshd,
  password only, MaxSessions 3): 5 streams cost 2 logins, a killed master
  with 4 openers costs 1, an unanswered or cancelled login with 4 openers
  costs 1 and falls back to nothing, two dead masters with 4 processes cost 1, a known password costs 0 dialogs, two ids of one state directory share one directory, another
  `ECCE_REALUSERHOME` gets its own, three sessions' stores that lose their
  masters cost one new login (#631), and the reaper
  leaves no socket or master. `testOpenSshTransport` covers the rest with a
  stub ssh, including forked processes sharing one login, one outcome and
  one overflow index.
