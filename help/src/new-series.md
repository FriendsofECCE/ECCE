# New Series

A series is one calculation's setup applied to a list of structures, giving
one calculation each. Use it for the same method and basis on a set of
molecules, or for the same job on every conformer.

## From finished calculations, in the Organizer

To redo several finished calculations in another code, for example the
results of an xTB run in ORCA:

1. Select the calculations in the tree or the project table.
2. Right-click and choose **New from This > In Another Code...**, or use
   **Edit > New from This in Another Code...**.
3. Choose the code, the end of the new names (`_orca` gives `water_orca`)
   and the folder, which is the one holding the first selected calculation.
   Click **OK**.
4. The first new calculation opens in the Calculation Editor. Set the theory,
   run type, basis set and launch settings, and save.
5. In the small window that stays in the Organizer, click **Make the
   Others**.

Each new calculation takes its source's final geometry, charge and
multiplicity and the setup from step 4, and is Ready. The Organizer's
message area says how many were made and lists any that were not, with the
reason, for example a multiplicity that does not fit the electrons. Select
the new calculations and choose **Run Mgmt > Launch Selected**.

Cancel in the small window leaves the first calculation and makes no others.

## From the command line

Make the **template** first: a calculation with the code, theory, run type,
basis set, solvent and launch settings (machine, cores, wall time) you want.
Its own structure does not matter, and it need not have been run. If some of
the structures have unpaired electrons, use an unrestricted theory in the
template: the series keeps the template's theory as it is.

The new calculations appear next to the template in state Ready with their
input files generated. Select them in the Organizer and choose
**Run Mgmt > Launch Selected**.

## Charge and multiplicity

An `.xyz` file takes them from words in its second (comment) line:
`charge=-1 mult=2`. Without them the charge is 0 and the multiplicity is the
lowest the number of electrons allows. A stored calculation brings its own
final geometry, charge and multiplicity. A structure whose multiplicity does
not fit its electrons is not made, and the report says so.

## What is copied from the setup

The code, theory, run type, the values of the theory and run type details,
the basis set, the solvent and the launch settings. Results, run state and
geometry constraints of the template are not carried: the constraints refer
to the template's atoms, so the new calculations have none, and the report
says when constraints were left out. If a structure contains elements the template's
basis set was not assigned to, the basis set of the same name is assigned to
the new elements.

## Command

```
ecce-series [--charge N] [--mult M] <template> <folder> <structure>...
```

Each structure is an `.xyz` file or a stored calculation, all in the local
data folder. The command prints one line per structure and works on the
stored calculations directly: an open Organizer shows the new calculations
after the folder is refreshed.
