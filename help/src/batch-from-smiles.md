# QSAR: batch from SMILES

A QSAR data set is a table of molecules with a measured activity. **Batch Add
from SMILES** turns such a table into one calculation per molecule, all with
the same setup, ready to launch together.

## Making the batch

1. Make a project folder for the data set and select it in the Organizer.
2. Choose **Tools > Batch Add from SMILES...**.
3. Click **Open File...** and choose the table, or paste it into the box.
   A spreadsheet must first be saved as CSV (comma, semicolon or tab
   separated). The first line holds the column headers. Charges written
   before the hydrogens, as MOE writes them (`[N+H]`), are read too.
4. Check the columns. ECCE guesses them from the headers and the values:
   - **Name column**: the calculation names (with *none*, the rows are named
     mol-001, mol-002, ...).
   - **SMILES column**: the structures.
   - **Measured values**: every column that is ticked is stored with each
     calculation under its header, so keep the unit in the header, as in
     `IC50 (uM)`. Columns named like an activity (IC50, Ki, pIC50, activity,
     or a concentration unit in parentheses) are ticked at first; a table of
     a few number columns has all of them ticked. Descriptor columns, such
     as those a MOE export carries, are left unticked.
5. Choose the **Setup**. The first entry makes a calculation called
   *QSAR template* in the folder: xTB, GFN2-xTB, geometry optimisation, on
   this computer with 4 threads. To use another code or method, make or
   open a calculation in the folder with that setup and choose *Same as* that
   calculation.
6. The preview lists each row with its charge, its number of atoms and its
   status. Rows that cannot be made are shown in red with the reason (a
   SMILES that cannot be read, a name used twice); they are left out.
7. Click **Create**. The report says how many calculations were made and
   lists the rows that were not. **Launch Batch** launches all of them with
   the setup's launch settings; **Close** leaves them Ready, and
   **Run Mgmt > Launch Selected** launches them later.

## What each calculation gets

- The structure from the SMILES, in 3-D and cleaned with the force field, as
  **Tools > Add from SMILES** in the Builder makes it.
- The charge from the formal charges in the SMILES: `[NH3+]` gives +1,
  `C(=O)[O-]` gives -1. Write the molecules in the protonation state you
  want to calculate (for example at pH 7); ECCE does not add or remove
  protons. The multiplicity is the lowest one the electron count allows.
- The setup of the template: code, theory, run type, details, basis set,
  solvent and launch settings, as a [New Series](new-series.md) copies them.
- The SMILES and the measured values. The values are shown in the
  calculation's annotation, for example `IC50 (uM) = 0.8; logP = 1.8`.

## Changing the setup afterwards

Open the template in the Calculation Editor, change it and save it. Then
run **Batch Add from SMILES** again with the same table, choose *Same as*
the template and tick **Make again the calculations of the same name that
were not launched**: every calculation of the batch that has not been
launched is made again with the new setup. Launched ones are kept as they
are.

## Command

```
ecce-batch-smiles [--template CALC] [--name COL] [--smiles COL]
                  [--response COL]... [--no-responses] [--header yes|no]
                  [--replace] [--dry-run] [--launch] [--json] <folder> <table.csv|->
```

The command does what the dialog does. Columns are given by header or by
number (1 is the first); without them they are guessed as in the dialog.
`--replace` is the tick box above. `--dry-run` only reads the table and reports each row. `--launch` launches
the new calculations in the running ECCE session (as `ecce-launch` does).
`--json` prints one JSON object: the folder, the template, the numbers made
and not made, and one entry per row with its line, name, SMILES, charge, URL
and the reason a row was not made. The exit status is 0 when every row was
made and 1 when some were not.

The values are stored as the calculation properties `ecce:qsar.smiles` and
`ecce:qsar.measured`, the second a JSON object keyed by column header.
