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
