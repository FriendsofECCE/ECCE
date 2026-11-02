# Export for Supporting Information

**File > Export > Supporting Information...** writes the selected
calculations as one multi-structure XYZ file, and optionally a table of the
same values, for the Supporting Information of a paper.

1. In the Organizer, select the calculations. Select several with Ctrl or
   Shift.
2. Choose **File > Export > Supporting Information...** and name the XYZ
   file.
3. Choose whether to also write a table: **CSV** or **LaTeX**. It is saved
   beside the XYZ file with the extension `.csv` or `.tex`.

For each calculation the XYZ file has the number of atoms, a comment line,
and the final geometry in Å. The comment line reads

    name  charge=0  mult=1  E=-76.3698948  G=-76.3664464  code=ECCE-QM method=DFT/DFT/6-31G*

`E` is the total energy and `G` the Gibbs energy, both in hartree and as
stored; `G` is `-` when the calculation has none. The table has the same
fields and the number of imaginary frequencies (`-` without a frequency
calculation).

The values are listed as they are stored. Energies from different codes or
methods are not combined or compared.

The same files can be made from the command line:

    ecce-si-export calc1 calc2 -o si.xyz --table latex
