# Export for Supporting Information

`ecce-si-export` writes the selected
calculations as one multi-structure XYZ file, and optionally a table of the
same values, for the Supporting Information of a paper.

Run `ecce-si-export <calculation>... -o supporting-information.xyz`, and
add `--table csv` or `--table latex` to also write a table, saved beside the
XYZ file with the extension `.csv` or `.tex`.
In the Organizer, select a folder or calculations and choose File > Export >
Supporting Information... to be asked for the file name and table kind.

For each calculation the XYZ file has the number of atoms, a comment line,
and the final geometry in Å. The comment line reads

    name  charge=0  mult=1  E=-76.3698948  G=-76.3664464  code=ECCE-QM method=DFT/DFT/6-31G*

`E` is the total energy and `G` the Gibbs energy, both in hartree and as
stored; `G` is `-` when the calculation has none. The table has the same
fields and the number of imaginary frequencies (`-` without a frequency
calculation).

The values are listed as they are stored. Energies from different codes or
methods are not combined or compared.

## Command line

The same files can be made from a terminal with `ecce-si-export`. On Linux
the command is on PATH; on macOS it is in `ECCE.app/Contents/Resources/bin`,
which you add to PATH.

    ecce-si-export calc1 calc2 -o si.xyz --table latex

Calculations marked as excluded (see [Excluded calculations](excluded.md))
are left out of the XYZ file and the table. If the selection contains any,
the export asks whether to include them; the table then has an `excluded`
column with the reason.
