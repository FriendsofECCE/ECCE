# Constraints and dummy atoms

A geometry optimisation moves every atom until the energy is lowest. Two
tools let you take control of it:

- a **constraint** holds a bond length, an angle or a torsion (a dihedral
  angle) at a value you choose, or holds atoms where they are, while the
  rest of the molecule relaxes;
- a **dummy atom** is a point in space, shown as `X`, with no electrons and
  no basis functions. It gives an angle or a distance something to refer to
  where there is no atom, such as the lone pair of ammonia.

This page uses both to draw the potential energy of ammonia as the
molecule is pushed from pyramidal towards flat.

## Where the constraints live

Constraints belong to the structure, not to a run type. You define them in
the Builder at any time, and ECCE keeps them with the calculation when you
save it. A calculation uses them only when its run type does (Geometry and
GeoVib); for any other run type they are kept and left out of the input.

The Calculation Editor tells you which case you are in. Beside its
**Geometry Constraints...** button it shows, for example:

> 2 constraints defined: used only by geometry optimisations

**Verify** repeats the note, and the input does not contain the constraints.
Change the run type to Geometry later and they apply, without being set
again.

## Define a constraint

1. In the Builder, choose **Tools > Geometry Constraints...**. You can also
   right-click in the 3D view while atoms are selected and choose
   **Constrain Selected Atoms...**, or press **Geometry Constraints...** in
   the Calculation Editor.
2. Select atoms in the 3D view. Click the first, then add the others with
   **Shift+click** or **Ctrl+click**. Shift+click or Ctrl+click on a selected
   atom takes it out again.
3. The dialog says what the selection would make, for example
   `Angle H2-N1-X5 (centre N1)`. Press **Constrain**.

Two atoms make a bond length, three an angle and four a torsion.
**Constrain** holds the value the geometry has now. **Restrain** pulls
towards it with a spring instead of holding it exactly. **Fix Atoms** holds
the selected atoms where they are; any number of atoms will do.

The constraints made are listed in the dialog. Double-click one, or select it
and press **Edit Value...**, to change the number it holds. **Remove** drops
the selected rows and **Clear All** drops everything.

### Pick the atoms in any order

For an angle the **centre atom** is the one bonded to both of the others;
for a torsion the four atoms follow a chain of bonds. If you pick them in
another order, ECCE puts them right when only one order fits, and shows
what it made: `Angle H2-N1-X5 (centre N1)`.

If the atoms are not bonded the way the constraint needs (an angle between
three atoms where the middle one is not bonded to both, or any atom pair that
is not bonded), ECCE says so and asks before adding the constraint in the
order you picked.

### What each code accepts

The dialog reports what the code of the calculation cannot do. NWChem, for
example, cannot combine **Fix Atoms** with a bond, angle or torsion
constraint: choose one of the two.

## Dummy atoms

The element `X` is the dummy atom. In the Build panel, open **More...** for the
whole periodic table and choose `X`; hold the pointer over it for a reminder
of what it is. It takes part in the structure like any atom:

- it can be bonded to an open site, and then lies at a bond length from that
  atom;
- constraints can use it, and its number counts in the atom list;
- it is not in the formula, adds no electrons and gets no basis set;
- the bond to it is only for drawing and does not reach the input.

Not every code can use one. If the calculation's code cannot (Quantum
ESPRESSO, GROMACS and the NWChem molecular dynamics codes), choosing `X` in
the Builder says so and no atom is added.

## Example: the potential of ammonia

1. Make a new calculation with NWChem and open it in the Builder.
2. Choose **Mode > Atom** (Ctrl+5), element N, with the shape Tetrahedral.
   Click in the 3D view to place the nitrogen. It has four open sites.
3. Choose element H, and click three of the open sites.
4. Choose element `X` and click the fourth open site. The dummy atom lies on
   the axis of the molecule, opposite the three hydrogens.
5. Choose **Tools > Geometry Constraints...**. Select `X`, then `N`, then one
   hydrogen, and press **Constrain**. Do the same for the other two hydrogens.
   Each of the three X-N-H angles is now listed.
6. Double-click an angle and give it a value, for example 100. Give the other
   two the same value.
7. Save the structure. In the Calculation Editor choose the run type
   **Geometry**, choose a basis set and run the calculation.

The molecule relaxes with the three angles held. Change the three values (80,
90, 100, 110, 120) and run again: the energies against the angle draw the
potential, with a barrier where the molecule is flat.
