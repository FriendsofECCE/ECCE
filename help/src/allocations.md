# Allocations

**Tools > Allocations...** in the Organizer lists each cluster account you
have set up and how much of it is used. The figure is for the account as a
whole, over all jobs charged to it. It is not what one calculation used; that
is the **Cost** row of the calculation's summary.

## What the window shows

One row for each machine and account. The accounts come from the machine's
profiles (see [Allocation profiles](machines.md#allocation-profiles)); a
machine appears if it has an **Allocation command**, set on the Connection
tab of Register Machines, or if its queue manager has a default one (Slurm).
Workstations and machines that ECCE does not connect to are not listed.

| Column | Meaning |
|---|---|
| **Machine** | The machine as registered in ECCE. |
| **Account** | The account of the profile. |
| **Used** | The usage, as the cluster reports it. |
| **Checked** | The time of day the cluster was asked. |

**Refresh** asks the clusters again. Without it, an answer is kept for 15
minutes. **Close** closes the window; it can be resized.

## What Used says

- `4.9 % of 5 000 CPU-h/month used (past 30 days); 3.1 % of 100 GPU-h/month`:
  the output of `projinfo`, as at HPC2N and other NAISS centres. The
  percentages are of the monthly allocations over the past 30 days.
- `1 235 CPU-h used (no allocation size known)`: the Slurm default. ECCE asks
  `sshare` for the account's usage, which is a total with no allocation size,
  so no percentage can be given.
- `not recognised`: the command ran but ECCE could not read its output. Hold
  the pointer over the row to see the line of output that names the account.
  A site's own command can be set in **Allocation command**; ECCE reads
  `projinfo` and Slurm's `sshare` output.
- `not checked: not logged in to <machine>`: ECCE reads allocations only over
  a connection that is already open, and never asks for a password for it.
  Connect to the machine first (for example by launching a job on it), then
  choose **Refresh**.
- `checking...`: the answer is on its way. The window can be used meanwhile.
