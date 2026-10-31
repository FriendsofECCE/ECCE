# Server-side job worker: implementation bookmark

Implementation bookmark based on `origin/main` at `a3813185`. The
Register Machines TOTP setting and client-side SSH prompt path are now
implemented in this worktree. The persistent server-side worker remains to be
designed and implemented.

## Goal

Move job launch, remote monitoring, result storage, and recovery from the
desktop client to a persistent worker on the ECCE data-server host. Run each
worker under that user's mapped local OS account, independently of the GUI and
interactive login. Keep machine configuration, SSH credentials, and durable
job state per user. Preserve the existing client-side launch path during
transition.

Clients may connect transiently to the data server. A worker may hold a
compute-host SSH connection while monitoring. Persist the scheduler job ID,
ECCE state, and monitor bookmark so restarts resume monitoring and never
blindly resubmit. TOTP codes are supplied by an authenticated client only when
the worker needs to authenticate; never persist a code or TOTP seed. If SSH
drops or the data-server host restarts, request a fresh code unless the site
supports a suitable renewable credential or API. Route compute-dependent
client actions through the worker when direct client-to-compute access is
disabled. Enforce per-user data access through ECCE interfaces or explicit
permissions.

## Register Machines: two-factor setting

In **Register Machines > Connection**, add a row immediately before the
**Allocation** section:

- Label: **Two factor authentication**
- Control: drop-down with **None** and **TOTP**
- Default: **None**
- Selecting **TOTP** means the configured remote host requires a TOTP code
  during SSH login.

Persist this as a per-machine `CONFIG.<machine>` setting (suggested key:
`twoFactorAuth`, values `none`/`totp`; omitted or unknown values behave as
`none` for backward compatibility). The setting follows the user's machine
configuration to the data-server account that owns the worker.

When the worker starts an SSH connection configured for TOTP, it requests a
code through the currently authenticated client. The client prompts for the
code and returns it over the authenticated channel. The worker supplies it to
SSH and waits for authentication to finish before proceeding. If the SSH
connection/authentication fails, report the failure to the user with a clear
login-failed message. Do not log or persist the code. Once SSH authentication
completes, the worker owns the connection and monitoring continues if the
client disconnects. A later SSH reconnect asks for a new code.

Relevant UI/config seams:

- `src/apps/machregister/WxMachineRegister.C`: `createConnectionPage()` builds
  the Connection rows; the generic `CfgChoice` row and draft sync/save path can
  persist a menu setting. Insert the new row before the Allocation heading.
- `src/apps/machregister/WxMachineRegister.H`: row kinds and controls.
- `include/tdat/MachineConfigDraft.H` and
  `src/tdat/resources/MachineConfigDraft.C`: managed CONFIG keys and merge
  conflict handling; include the new key so layered configs save correctly.
- `include/tdat/RefMachine.H` and `src/tdat/resources/RefMachine.C`: expose a
  defaulted getter for the setting to SSH/worker callers.
- `src/comm/rcommand/RCommand.C` and `src/comm/rcommand/SshTransport.C`: route
  the TOTP prompt, wait for completed authentication, and surface failure.
  The existing libssh keyboard-interactive path wipes prompt answers, but its
  current prompt handler runs in the client process and recognizes codes by
  prompt text; the worker design needs an authenticated client challenge/reply
  bridge, with the machine setting controlling the expected TOTP flow.

Current implementation touches `WxMachineRegister.C`,
`MachineConfigDraft.C`, `RefMachine.C`, `RCommand.C`, and the existing remote
connection call sites. It stores `twoFactorAuth: totp` in the machine CONFIG;
missing/unknown values resolve to `none`. Configured TOTP logins use libssh's
keyboard-interactive flow, show a **TOTP code** prompt for non-password
challenges, wait for authentication to complete, and report a TOTP login
failure through the existing connection error path. The new mode requires
ECCE to be built with libssh. The worker challenge/reply bridge described
below is not implemented yet.

## Existing execution path

- `src/comm/commxt/Launch.C`: validates SSH, generates submit and monitor
  files, transfers them directly to compute, submits, records the scheduler
  ID, then calls `startJobStore()`.
- `src/apps/jobstore/eccejobmaster.C`: currently supervises
  `eccejobstore` on the client host and passes the client's `AuthCache` to it.
- `src/comm/commxt/JobStore.C` and
  `src/apps/jobstore/eccejobstore.C`: connect to compute, run
  `eccejobmonitor` over a stream, parse progress/results, update ECCE, and
  restart from a bookmark stored beside the job-store config. That storage is
  currently client-side.
- `src/comm/commxt/JobCatchUp.C`: client-side reconnect/catch-up; this needs to
  become a worker recovery/reconcile path for worker-owned jobs.
- `help/src/machines.md`: document the new menu setting and prompt behavior.
- `docs/claude/services/remote-views-use-ecces-own-login.md`: records that one
  job-store run uses one SSH login/stream and a dropped connection ends that
  run today.

## Implementation sequence

1. **Resolve identity and storage contracts.** Define ECCE-user-to-UID mapping,
   worker request authentication, and how a worker accesses the user's
   calculation and writes DAV/JMS updates after the client disconnects. Do not
   reuse the current client `AuthCache` pipe as durable worker identity.
2. **Add and persist the machine option.** Implement the Connection-panel menu,
   CONFIG draft/save/merge support, default parsing in `RefMachine`, and help
   text. Test default `None`, saving/reloading `TOTP`, and old configs with no
   key.
3. **Create a per-user worker supervisor and durable record.** Start workers
   under the mapped user UID. Store owner, calculation URL, machine/config
   snapshot, remote directory, submission phase, scheduler ID, ECCE state,
   bookmark, and recovery status in user-private storage. Use atomic writes.
   Distinguish not-submitted/submitting/submitted so a restart cannot
   accidentally submit twice; reconcile an ambiguous crash during submission.
4. **Bridge TOTP to the authenticated client.** Use an expiring, one-use,
   user/job-bound challenge. Keep the code in memory only. Wait for SSH auth
   completion and report failures to the client. If no client can answer, park
   the job in a waiting-for-auth state. After successful login, client
   disconnect must not own or cancel the SSH stream.
5. **Route an opt-in launch through the worker.** Keep existing
   `Launch` validation and artifact generation where practical, but send a
   launch package through an authenticated data-server API. The worker owns
   compute-side transfer, submission, monitoring, result storage, and
   reconnect. Start with one user, one machine, one scheduler, and one code.
6. **Make monitor checkpoints crash-safe.** Reuse `eccejobmonitor` and the
   `JobStore` parser/storage logic where practical. Persist the bookmark and
   job state on the data-server host. Ensure a checkpoint cannot advance past
   uncommitted properties/results: write it after durable storage or make
   replay idempotent. Recover from the saved scheduler ID; never relaunch the
   submit command on monitor restart.
7. **Proxy compute-dependent actions and retain fallback.** Route cancel,
   reconnect, queue check, and tail through the worker where required. Keep
   client launch/monitor behind the existing path or an explicit feature
   switch until worker mode is proven and can be rolled back safely.

## Vertical-slice acceptance

Use a test SSH host that requires password plus TOTP and a simple scheduler
job. Verify:

1. The machine defaults to **None**; choosing **TOTP** survives save/reload.
2. Launch reaches the worker under the correct UID, prompts in the client,
   authenticates, and persists the scheduler job ID before monitoring.
3. A wrong/expired code or SSH failure reports login failure to the user and
   does not claim the job was launched successfully.
4. Disconnect the client after SSH authentication; worker monitoring and
   result storage continue.
5. Restart the worker/data-server host; it reconnects with the stored job ID
   and bookmark, requests a fresh code if SSH must be reopened, and does not
   resubmit.
6. Reconnect a client and verify persisted state, progress, output files, and
   parsed results through ECCE interfaces.
7. Verify another user's client cannot inspect or operate this user's worker
   state.

Use the existing launch and SSH integration coverage under `tests/launch/`
and `tests/transport/sshd/` as starting points. The initial implementation
should add focused config serialization, worker recovery, TOTP challenge, and
end-to-end disconnect/restart cases. Keep the client path covered as a
regression path.
