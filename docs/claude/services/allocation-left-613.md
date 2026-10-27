---
type: map
title: "Design (#613, not built): the calculation summary shows the allocation and how much is left"
area: services
section: "Design"
paths: ["src/apps/builder/CalcInfoPanel.C", "src/tdat/resources/RefMachine.C", "src/tdat/resources/RefQueueMgr.C", "siteconfig/QueueManagers", "src/apps/machbrowser/MachineBrowser.C", "src/comm/rcommand/OpenSshTransport.C", "src/comm/rcommand/RCommand.C", "include/dsm/MachinePreferences.H", "include/dsm/AllocationProfiles.H"]
issues: [613, 541, 487, 599]
---
**Status: design for review. It runs a command on a remote machine, so the
points under "Remote shell" need a yes before anyone writes code.**

The "Launch Info" block of the calculation summary (`CalcInfoPanel`, Machine,
Queue, # Processors, Run Directory, Start Date ...) gets two lines for a
machine that has an allocation command: **Allocation:** (the account the job
was submitted with) and **Left:** (core-hours used of allocated, "checked
10:20").

## What exists already
- The command is already a machine setting: `RefMachine::allocationCommand()`
  (Register Machines, Connection tab, "Allocation command"), with the queue
  manager's default behind it (`Slurm|queryAllocationCommand: sshare -l -U -u
  $USER` in `siteconfig/QueueManagers`), resolved by
  `RefQueueManager::allocationCommand(qmgr, machineCommand)`. The Machine
  Browser's Allocation button runs it and shows the raw text.
  **No new setting is needed**: #613's "set per machine in Register Machines"
  is this field. Empty (and no queue-manager default) means the lines are not
  shown. A workstation uses no queue-manager default, as in the Browser.
- The account is stored per calculation at launch since #541
  (`launch_account`, `launch_profile`; `Launchdata::account`, `profile`).
  A calculation launched before that has no account: show neither line.
- Per profile? The command is per machine; only the account differs per
  profile. The command gets the account through a `##account_no##`
  substitution (same token as the submit script), so `projinfo -p
  ##account_no##` works. A command without the token is run as written and its
  whole output is the raw fallback (below). Slurm default for a stored account:
  `sshare -n -P -A ##account_no## -o Account,User,RawUsage,GrpTRESMins` is
  not the default today (the default is the per-user fair-share view); adding
  a second, account-keyed default `queryAllocationLeftCommand` is a separate
  decision, see "Open".

## Remote shell
- **Which connection.** `RCommand` on the machine's registered shell, user
  and front end, exactly as `MachineBrowser::queryStatus` builds it, with
  `execout(cmd, out, "", timeout=20)`. Under `ECCE_SSH_CONTROL_DIR`
  (`owned-ssh-master.md`) that is a session on the shared master.
- **No new login, ever.** The summary must not be what opens a login. Before
  running, ask the transport whether a master is live
  (`OpenSshTransport::masterAlive()`, `ssh -O check`, a local socket test);
  if not, show the cached value or nothing, and run nothing. This needs one
  small public method (`RCommand::loginIsLive()`, false for any transport that
  cannot tell, including libssh and Windows) -- the only change in
  `src/comm/rcommand`. The command is never run with a password, never
  through `openMaster()` and never starts askpass, so it cannot cause a
  dialog or count against the `.outcome` memory of `owned-ssh-master.md`.
- **Off the GUI thread**, like the launch steps (`launch-steps-run-off-the-gui-
  thread.md`): the panel is drawn at once from the cache and refreshed with
  `CallAfter` when the command returns. A MaxSessions refusal is not retried.
- **Never on a machine without the command**, and not for `localhost` /
  `noRemoteAccess` / `dummy` machines (no connection exists), nor for a
  calculation with no stored account.
- **Central server.** Preferences, cache and the command are per user on the
  client; the data server is not involved and nothing is stored in the
  calculation. A shared system broker (SharedBroker) changes nothing: the
  command runs from the user's own session.

## Caching
- Key: machine + account. Value: parsed result, raw text, time read. In memory
  of the program showing the summary, plus a small file
  `~/.ECCE/AllocationCache` (one line per key) so a restart inside 15 minutes
  does not ask again. Read at most every 15 minutes per key; shared by every
  summary that names the same machine and account. A failed run is cached too
  (as "nothing", 15 minutes) so a down cluster is not tried on every click.
- Shown with its time ("checked 10:20"); an old cache entry is shown, marked
  with its time, only while the master is not live.

## Parsing
Parsing rules go in wx-free code (`include/dsm/AllocationLeft.H`, header-only
like `AllocationProfiles.H`) with a unit test from recorded samples; the panel
only shows the result.
- **projinfo (HPC2N/NAISS).** Known only from memory and not checked on a
  machine from here: a header line naming used and allocated (with units such
  as `[core*h]` or `[h]`) and one row per project starting with the project
  id, followed by numbers. Rule: find the row whose first token equals the
  account (case-insensitive); take the first two numeric tokens after it as
  used and allocated; take the unit from the header if a bracketed unit is
  there, else "core-hours" is not assumed and no unit is shown. **A real
  `projinfo` sample from Kebnekaise must be added to the test before
  release.**
- **Slurm `sshare`/`sreport`.** With `-n -P` the output is `|`-separated: the
  account's row, `RawUsage` in cpu-seconds (decayed), and a limit only when
  the site set `GrpTRESMins` (`cpu=<minutes>`). Used = RawUsage / 3600;
  allocated = cpu minutes / 60. No limit set: show used only ("48 000 core-h
  used"), never a made-up total.
- **Anything else** (a site's own tool, a column layout not recognised, no
  matching row, a number that does not parse): show the one output line that
  contains the account, trimmed to 120 characters, with the time; if no line
  contains it, show nothing. Wrong numbers are worse than the raw line, so a
  rule that finds a pair that is not used <= allocated plus a margin falls back
  to the raw line too.

## Where it shows and when it shows nothing
- `CalcInfoPanel` Launch Info, under Queue: "Allocation: naiss-2" always when
  an account is stored and the machine has the command; "Left: 12 000 of
  50 000 core-h (checked 10:20)" when a value or a raw line exists.
- Nothing, with no error text, when: no command; no stored account; no live
  connection and no cache; the command failed, timed out (20 s) or printed
  nothing; the output matched no rule and has no line with the account. The
  reason goes to the log only.

## Open (needs a decision before code)
1. The `loginIsLive()` check on `RCommand`, above: it is the only remote-shell
   change, and it is what keeps the feature from ever prompting.
2. Whether Slurm gets an account-keyed default command, or only machines that
   set one (such as HPC2N with `projinfo`) show the lines.
3. A projinfo sample (Kebnekaise: `projinfo`, `projinfo -p <account>`).
4. Tests: a stub `ssh`/command (as `testOpenSshTransport`) for the live and
   not-live paths and the 15-minute cache; sample files for the parser.
