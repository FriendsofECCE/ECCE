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
electronic energy E, zero-point energy and Gibbs energy (Hartree); the reason
if the calculation is excluded; and a Problems column.

A value that is not stored is shown as `-`, never as 0. Convergence is read
from the last progress snapshot kept with the calculation, so a calculation
without one shows `-`.

## What is highlighted

A row is shown in red when the calculation did not terminate normally, did
not converge, has an imaginary-mode count that does not fit its run type (none
for a minimum, one for a transition state), or has S<sup>2</sup> more than 0.1
from S(S+1). Excluded calculations are grey.

From a terminal: `ecce-si-export --checks csv <calculation>...` (or `latex`)
prints the same table.
