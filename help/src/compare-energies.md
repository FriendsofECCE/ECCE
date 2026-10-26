# Compare Energies

**Tools > Compare Energies...** in the Organizer gives the difference between the
final total energies of two calculations. It is how an ionisation energy, an
electron affinity, a reaction energy or the energy of a conformer relative to
another is read off from finished calculations.

## Choosing the two calculations

1. Select two calculations in the Organizer (in a project's table, with the
   Ctrl key held), then choose **Tools > Compare Energies...**. The window opens
   with the first as **Calculation A** and the second as **Calculation B**.
2. With anything else selected, you choose **Calculation A** and
   **Calculation B** from the two lists. They list the calculations of the
   project that holds the selection, and the window starts with the selected
   calculation as A (or the first of the project) and the next one as B.

Changing either list updates the result at once.

## The result

The **Energies** area shows the final total energy of each calculation in
Hartree, and the difference **E(B) - E(A)** in Hartree, kJ/mol, kcal/mol and
eV. The energy is the one the calculation's summary shows as **Final energy**.
The text can be selected and copied.

For an ionisation energy, take A as the neutral atom and B as the cation:
IE = E(cation) - E(atom). For an electron affinity, take A as the anion and B
as the neutral atom: EA = E(atom) - E(anion). A positive value in both cases
means that the energy rises from A to B.

## Notes

The **Notes** area, below the results, says when the two calculations are
not directly comparable. Nothing is blocked; the difference is still shown.
There is a note when

- the code, the method (theory) or the basis set differs between A and B;
- a calculation has no final energy (it is not finished, or the code did not
  report one), so no difference can be made;
- an energy is the last point of a relaxed scan in one calculation and not in
  the other;
- a calculation gives a heat of formation (MOPAC) instead of a total energy.

Energies from different methods or basis sets do not subtract to a physical
quantity. The note is a reminder of that, and the number is still available
if the comparison is what you intend.
