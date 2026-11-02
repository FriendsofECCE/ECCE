# New Series

A series is one calculation's setup applied to a list of structures, giving
one calculation each. Use it for the same method and basis on a set of
molecules, or for the same job on every conformer.

## In the Organizer

1. Make the **template**: a calculation with the code, theory, run type,
   basis set, solvent and launch settings (machine, cores, wall time) you
   want. Its own structure does not matter, and it need not have been run.
   If some of the structures have unpaired electrons, use an unrestricted
   theory in the template: the series keeps the template's theory as it is.
2. Select the template. To use other stored calculations as structures,
   select them in the project table together with the template.
3. Right-click the template and choose **New Series from This...**.
4. In the file dialog, choose the `.xyz` files to add (several at once), or
   cancel it if the structures are all calculations.
5. A report lists each new calculation as made, or not made with the reason.
   The new calculations appear next to the template, named after the xyz file
   or the calculation, in state Ready with their input files generated.
6. Select them and choose **Run Mgmt > Launch Selected**.

## Charge and multiplicity

An `.xyz` file takes them from words in its second (comment) line:
`charge=-1 mult=2`. Without them the charge is 0 and the multiplicity is the
lowest the number of electrons allows. A stored calculation brings its own
final geometry, charge and multiplicity. A structure whose multiplicity does
not fit its electrons is not made, and the report says so.

## What is copied from the template

The code, theory, run type, the values of the theory and run type details,
the basis set, the solvent and the launch settings. Results, run state and
geometry constraints of the template are not carried: the constraints refer
to the template's atoms. If a structure contains elements the template's
basis set was not assigned to, the basis set of the same name is assigned to
the new elements.

## From the command line

```
ecce-series [--charge N] [--mult M] <template> <folder> <structure>...
```

Each structure is an `.xyz` file or a stored calculation, all in the local
data folder. The command prints one line per structure and works on the
stored calculations directly: an open Organizer shows the new calculations
after the folder is refreshed.
