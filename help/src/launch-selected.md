# Launch Selected

**Run Mgmt > Launch Selected...** in the Organizer submits several
calculations in one step, without opening the Launcher for each. It uses the
launch settings each calculation already stores (machine, queue, cores,
memory, wall time, run directory), unless another machine is chosen.

## Launching

1. Select the calculations in a project's table (with the Ctrl key held), or
   in the tree.
2. Choose **Run Mgmt > Launch Selected...**. The same item is in the
   right-click menu of a calculation.
3. The window lists each calculation with its **Machine**, **Queue**,
   **Cores** and **Wall time**, and a **Check**. A calculation that can be
   submitted shows **OK**. One that cannot is listed in grey, with the reason
   after **Skipped:**. Select a skipped calculation and choose **Open in
   Launcher** to set it up there.
4. Choose **Launch**. A progress window shows the calculation being
   submitted, and a result list follows.
   The Organizer stays usable while the batch launches. **Stop** in the
   progress window finishes the calculation being submitted and submits no
   more. Delete, Cut, Paste, Run Mgmt actions and Quit wait until it is done.

## Launch on

**Launch on:** at the top of the window is **As set up** by default: each
calculation is submitted to the machine it stores. Choosing a registered
machine submits all of them there instead. Where the machine has more than
one allocation profile, **Profile:** appears beside it (profiles are made in
**Tools > Register Machines...**).

For another machine, each calculation's stored settings are carried over
where they fit:

- Cores, nodes, memory and wall time are kept, and cut to the limits of the
  machine and its queue.
- A queue the machine does not have is replaced by the machine's default
  queue.
- The account, the run directory and the scratch directory are those of the
  chosen profile (**Default** when no profile is chosen).

The **Changed** column shows what differs from the stored settings, for
example `queue batch → normal; 48 → 12 cores`, before anything is
submitted. When a calculation is launched, the machine and settings used are
stored with it, as the Launcher stores them, so **Reconnect** and the
Launcher afterwards show where it ran.

## Why a calculation is skipped

- **not ready**: the calculation is already submitted or finished.
- **no machine stored**, or **machine "..." is not registered here**.
- **code ... is not registered on ...**: the chosen machine does not offer the
  code of the calculation. Register the code for the machine under **Tools >
  Register Machines...**, or choose another machine.
- **no run directory stored**: with **As set up** the calculation has none;
  with a machine, its profile has none.
- **no core count stored**, or **no queue stored**.

A machine set to staged launches cannot be used here; launch from the
Launcher. A failed submit leaves the calculation **Ready**, with the reason
in the Progress box and the run log.

## From the command line

With ECCE running, calculations prepared outside the window can be launched
from a terminal:

    ecce-launch [on=MACHINE] [profile=NAME] <calculation folder or URL>...

The Organizer submits them in one batch, with the same rules as Launch
Selected, and its tree shows the change at once. `on=` and `profile=` act as
**Launch on** and **Profile** do. A folder inside the data folder is turned
into the calculation's address; with a data server, give the address.

One line is printed for each calculation, either `name: job id N` or
`name: not launched: reason` (for example, a calculation that is already
submitted). The exit status is 0 only when every calculation was launched.
Without a running ECCE session the command says `ECCE is not running: start
ECCE first` and launches nothing. While a Launch Selected batch is submitting,
it asks to be run again later.

### Terminating from the command line

    ecce-terminate [--reason "text"] <calculation folder or URL>...

cancels submitted, running or waiting calculations the way **Run Mgmt >
Terminate Selected** does, without asking for confirmation. Calculations in
other states are refused by name, with their state. The reason, if given, is
shown as the calculation's run status reason. One line is printed for each
calculation, either `name: terminated, job id N` or `name: not terminated:
reason`; the exit status is 0 only when every calculation was terminated.
Paths, URLs and the message without a running ECCE session are as for
`ecce-launch`.

### Reconnecting from the command line

    ecce-reconnect <calculation folder or URL>...

reconnects calculations the way **Run Mgmt > Reconnect Selected** does,
without asking for confirmation. A machine that needs a password asks for it
in the Organizer window, as the menu item does. One line is printed for each
calculation, either `name: reconnected, job id N` or `name: not reconnected:
reason`, for example when the calculation's state cannot be reconnected; the
exit status is 0 only when every calculation was reconnected. Paths, URLs and
the message without a running ECCE session are as for `ecce-launch`.

### Fetching files from the run directory

    ecce-fetch [--list] <calculation folder or URL> [pattern ...]

copies files from the calculation's run directory on its compute machine into
the calculation's stored outputs, for a job whose automatic copy-back missed
them, for example cube or PDOS files. It works for a finished calculation as
well as a running one. The patterns are file-name patterns such as `*.cube`,
relative to the run directory; without any, `*.cube *.cub *.pdos` are
fetched. A file already stored under the same name is replaced.

`--list` only lists the run directory and stores nothing. The login is the one
Reconnect uses, so a machine that needs a password asks for it in the
Organizer window. One line is printed for each file, `name: fetched (N
bytes)`, for a pattern that matched nothing `pattern: no file matched`, and
`name: not fetched: reason` when nothing could be fetched; the exit status is
0 only when every pattern fetched something. Paths, URLs and the message
without a running ECCE session are as for `ecce-launch`.
