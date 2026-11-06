# Renaming and moving calculations

**Edit > Rename** (or the toolbar's pencil) gives a calculation, folder or
project a new name. A name may contain letters, digits, `.`, `_` and `-`.
**Edit > Cut** followed by **Edit > Paste** on another folder, or dragging a
calculation onto a folder in the tree, moves it there. The calculation keeps
its inputs, outputs, results and run log; only its name or place changes.

## While a job is running

A calculation whose job is **Submitted**, **Running** or **Waiting for
Login** cannot be renamed or moved, and neither can a folder or project that
contains one. While the job runs, its results are written back to the place
the calculation had when it was launched, so a renamed or moved calculation
would lose them. ECCE says which calculation is in the way; rename or move
it once the job has ended, or after terminating it (**Run Mgmt > Terminate
Calculation**).

Calculations in a reaction study (Reactant, Product, Transition-State) keep
their names, because the study finds them by name.

A copy (**Edit > Copy** and **Paste**) is always possible: the copy is a new
calculation and the running job keeps writing to the original.
