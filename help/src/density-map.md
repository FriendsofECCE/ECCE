# The Density Map

The Density Map draws the electron density, or one molecular orbital, as
contour lines on a flat slice through the molecule. It is calculated from the
orbital coefficients and the basis set stored with the calculation, so it
works for every code whose orbitals ECCE reads. Open it from the
**Properties** list, beside the **MOs** panel. It needs a calculation that has
orbitals.

While the panel is open, the slice is drawn in the 3-D view as a translucent
square, so you can see where it cuts the molecule. The square follows every
change you make. It disappears when you close the panel.

## Show

- **Total density**: the sum over all occupied orbitals, in electrons per
  bohr³.
- **One orbital**: the orbital chosen in the **MOs** panel. Select another
  orbital there and the map follows. The **MO** box beside it holds the same
  number; use it or the arrows to step through the orbitals. For an
  unrestricted calculation a second box chooses alpha or beta.
- **Sum of the degenerate set**: some orbitals come in sets with the same
  energy, such as the two π orbitals of N₂ or CO. Any mixture of the members
  is an equally good answer, so **One orbital** shows one arbitrary member of
  the set, and the map says which one. The sum |ψ₁|² + |ψ₂|² of all members
  does not depend on that choice, has no signs, and for a linear molecule is
  the same all the way around the axis. Orbitals count as degenerate when
  their energies agree to 0.000015 hartree, the same limit the MO diagram uses.

## Plane

The slice is flat. Choose how it is placed.

- **Molecular plane**: the best-fit plane through all the atoms, for a flat
  molecule. Benzene gives the ring plane. A linear molecule has no single
  plane; the map is then drawn on a plane that contains the axis.
  For a molecule that is not flat this entry is greyed out; use
  **Through three atoms**.
- **Through three atoms A, B, C**: the plane through the three atoms you
  choose. Use it for a molecule that is not flat. For ammonia, choose H–N–H to
  see an orbital in the plane of two N–H bonds.
- **Containing the line A–B, ⊥ molecular plane**: the plane that holds the
  straight line between atoms A and B and stands upright on the molecular
  plane. For ethene, choose the two carbons: the slice goes through the C=C
  bond and shows the lobes of the π orbital above and below the molecule.
  A and B can be any pair, not only bonded ones: in benzene, C1 and C3 give a
  slice across the ring along a chord. B starts as an atom bonded to A, and
  atoms that are not bonded to A are marked "(not bonded)" in the list. This
  entry is greyed out for a molecule that is not flat and for a linear one.
- **This view (parallel to the screen)**: the plane parallel to the screen
  through the centre of the molecule, as the 3-D view is now. Choosing it
  takes the view at once. Rotate the molecule in the 3-D view and press
  **Update from view** to take the new view. The plane is stored, not the
  view: it stays where it was when you rotate again.

### Picking atoms in the 3-D view

You do not need the lists. Select three atoms in the 3-D view (Shift-click
adds to the selection) and the plane through them is used; select two atoms
and the plane containing their line is used. The A, B and C lists follow your
picks and can also be used from the keyboard.

## Offset

The slider moves the plane along its own normal, away from the atoms, by up to
5 Å either way. Type a distance in the box beside the slider for a value to
0.01 Å, press Enter, or press **Reset** to return to zero. The caption names
the distance, for example "0.8 Å above plane through C1, C2, C3". The sign
follows the plane's normal; the other side is "below". Choosing a different
plane sets the offset back to zero.

While you drag the slider the map is drawn on a coarse grid, and on a full
grid when you let go. A large molecule can take a moment; a progress bar
appears if so.

An example: for benzene, the **Molecular plane** with an offset of 0.8 Å shows
the density of the π electrons above the ring.

## Reading the map

- The axes show distance in the plane in ångström. The window is the extent of
  the atoms plus 1.5 Å.
- Atoms lying in the plane are black and labelled. Atoms off the plane are
  faint and unlabelled; they only show where the slice sits.
- **Total density** and the degenerate sum use one colour scale, from dark
  purple to green. The first contour is 0.001 and each next contour is double
  the last. The legend lists every level drawn, with the highest value in the
  map.
- **One orbital** uses solid blue lines for positive values and dashed
  vermilion lines for negative values, so the sign survives a black-and-white
  printout. Levels start at ±0.02 and double. A node, where the orbital
  changes sign, is where the two colours meet.
- When the fixed levels (0.001 for a density, ±0.02 for an orbital, each
  double the last) would give fewer than three contours, as for a core orbital
  or a plane that cuts the orbital where it is weak, the levels are taken from
  the plane's own maximum instead: the maximum divided by 2, 4, 8 and so on.
  The legend then says "levels scaled to the plane's max".
- The caption above the buttons names the quantity, the orbital with its
  energy and occupation, which member of a degenerate set it is, the plane
  and the offset.

## Export

**Export .png...** saves the picture as drawn. **Export .csv...** saves every
grid value: the position in the plane, the Cartesian position, and the value,
with the plane and the offset in the header.
