# Reconnect Selected

After ECCE has been closed, restarted or has lost its connection to a
compute machine, a calculation can be left in the state **Waiting for Login**,
or **Submitted** or **Running** without anything following the job. **Run
Mgmt > Reconnect Selected...** in the Organizer reconnects several such
calculations in one step. It does the same as **Run Mgmt > Reconnect** (or the
**Reconnect** button of the Progress box) does for one calculation.

## Reconnecting

1. Select the calculations in a project's table (with the Ctrl key held), or
   in the tree.
2. Choose **Run Mgmt > Reconnect Selected...**. The same item is in the
   right-click menu of a calculation.
3. The window lists each calculation with its **Machine**, its **State** and
   a **Check**. A calculation that can be reconnected shows **OK**. One that
   cannot is listed in grey, with the reason after **Skipped:**.
4. Choose **Reconnect**. A progress window shows the calculation being
   reconnected, and a result list follows.

Calculations on the same machine are reconnected one after the other, so a
login to that machine is asked for once.

Reconnecting reads the job's output again from the start. It may take several
minutes before the previous output has been parsed, and the viewers of the
calculation answer slowly until then.

## Why a calculation is skipped

- **not launched**: the calculation is **Created** or **Ready**.
- **already finished**: the calculation is **Completed**, **Failed**,
  **Killed** or otherwise ended. Use **Run Mgmt > Reconnect** on a single
  calculation to parse its output again.
- **no machine stored**, or **machine "..." is not registered here**: the
  calculation names no compute machine, or one that is not registered under
  **Tools > Register Machines...** on this computer.
- **no job id stored**, or **no run directory stored**: the calculation has no
  record of the job on the machine.

A calculation that fails to reconnect stays **Waiting for Login**, with the
reason shown in the Organizer's status line, and is tried again at the next
start of ECCE.
