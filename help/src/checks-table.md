# Checks table

The Checks table lists every calculation under a folder in one sortable grid,
so that a failed or doubtful run is seen before its numbers are used.

1. In the Organizer, select a folder (or some calculations).
2. Right-click and choose **Checks Table...**, or use
   **File > Export > Checks Table...**.
3. Click a column heading to sort by it; click again to reverse. Values that
   are not stored sort last.
4. **Export CSV...** and **Export LaTeX...** write the table to a file.

Columns: state; normal termination; optimisation converged; SCF converged;
number of imaginary modes; the expectation value of S<sup>2</sup> and its
deviation from S(S+1); charge; multiplicity; method; basis; solvent;
electronic energy E, zero-point energy and Gibbs energy (Hartree); the core-hours
booked and used (see below); the reason
if the calculation is excluded; and a Problems column.

A value that is not stored is shown as `-`, never as 0. Convergence is read
from the last progress snapshot kept with the calculation, so a calculation
without one shows `-`.

## Core-hours booked and used

**Booked (core-h)** is the cores times the wall time limit the job was
launched with: what the queue was asked for. **Used (core-h)** is the cores
times the time the job ran, from the start and end time of the job itself. A
calculation that was never launched, or has no start or end time, shows `-`.
`ecce-si-export --checks csv --total <folder>` adds a last row, `total`, that
sums both columns over the calculations in the folder (a folder stands for
the calculations below it); a calculation without a value adds nothing to the
sum.

## What is highlighted

Imaginary modes have wavenumbers below -1 cm⁻¹. Frequencies with magnitude
below 1 cm⁻¹ are numerical translation/rotation zeros and are excluded from
mode counts, including small negative values displayed as `-0.0`.
The same rule is used in Rank flags, thermochemistry, the vibrations panel
and the finished job's progress panel. Progress shows the total number of
vibrational modes (real plus imaginary), followed by the imaginary count;
stored frequencies supply the counts when the output does not report them.

A row is shown in red when the calculation did not terminate normally, did
not converge, has an imaginary-mode count that does not fit its run type (none
for a minimum, one for a transition state), or has S<sup>2</sup> more than 0.1
from S(S+1). Excluded calculations are grey.

From a terminal: `ecce-si-export --checks csv <calculation>...` (or `latex`)
prints the same table.
