# Compare Energies

**Tools > Compare Energies...** in the Organizer gives the difference between
the energies of two sides, each a sum of finished calculations. It is how an
ionisation energy, an electron affinity, a reaction energy, a solvation energy
or the energy of a conformer relative to another is read off.

## Choosing the calculations

1. Select two calculations in the Organizer (in a project's table, with the
   Ctrl key held), then choose **Tools > Compare Energies...**. The window opens
   with the first as the only entry of **Side A** and the second as the only
   entry of **Side B**.
2. With anything else selected, choose the calculations from the lists. They
   list the calculations of the project that holds the selection, in
   alphabetical order (capital letters and small letters count the same, and
   numbers count by value, so C2 comes before C10).
3. **Add calculation** adds an entry to a side; the **x** button removes one.
   Each entry has a sign (**+** or **-**) and a whole-number coefficient. For
   butadiene + ethene -> cyclohexene, put butadiene and ethene on side A and
   cyclohexene on side B; for 2 H2 + O2 -> 2 H2O, put H2 with coefficient 2 and
   O2 on side A, and H2O with coefficient 2 on side B. A side is the sum of its
   entries, each energy multiplied by its coefficient (and by -1 for a **-**
   entry).

Changing anything updates the result at once.

## The result

The **Energies** area is a table with the sum of side A and of side B in
Hartree, and **B - A** in kJ/mol, kcal/mol and eV:

- **E** is the final total energy of each calculation, the one the
  calculation's summary shows as **Final energy**.
- **E + ZPE**, **H** and **G** (at the temperature of the frequency
  calculations, shown in the row name) need a frequency calculation for every
  calculation on both sides; otherwise the row says "needs frequencies" and a
  note names the calculations that lack it.
- The solvent model of each side is listed under the table.

Energies in Hartree are shown to 6 decimals, differences to 1 decimal in kJ/mol
and kcal/mol and to 2 decimals in eV, which is about what the methods carry.
**Copy** puts the table on the clipboard with all digits. The Reaction Energies
panel of a transition state search uses the same rounding.

For an ionisation energy, take the neutral atom as A and the cation as B:
IE = E(cation) - E(atom). For an electron affinity, take the anion as A and the
neutral atom as B: EA = E(atom) - E(anion). A positive value in both cases means
that the energy rises from A to B.

A gas-phase and a solvent calculation of one molecule (same atoms, charge,
multiplicity, method and basis set) give **B - A** as the solvation energy, and
the window says so.

## Warnings and notes

The **Notes** area, below the table, says when the sides are not directly
comparable. Nothing is blocked; the numbers are still shown. There is a warning
when

- the code, the method (theory), the basis set or the dispersion correction
  differs between any two of the calculations;
- the solvent model differs (except for the gas phase / solvent case above);
- the atoms on the two sides differ, so the sides are not a balanced reaction;
- the temperatures of the frequency calculations differ, so H and G are not
  comparable;
- a calculation gives a heat of formation (MOPAC) instead of a total energy, or
  an energy is the last point of a relaxed scan on one side only.

A calculation that has no final energy (it is not finished, or the code did not
report one) is named in a note and no table is made.

Energies from different methods or basis sets do not subtract to a physical
quantity. The warning is a reminder of that, and the number is still there if
the comparison is what you intend.
