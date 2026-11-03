# ELF, LOL and NCI in the 3-D view

The **MOs** panel draws three more views of the electron density as surfaces
in the 3-D view, next to **Density** and **Spin Density**: **ELF**,
**LOL** and **NCI**. They are calculated from the orbital
coefficients and the basis set stored with the calculation, so they are
offered for every calculation that has orbitals, occupations and a basis set.
Choose one in the type list, or under **Properties > Surfaces**, and press **Compute**. The grid is the one the
other density views use (set it with the grid button), and a long
calculation shows a progress bar with a **Cancel** button.

Open-shell calculations (UHF, UKS, ROHF) are treated spin by spin.

## ELF: the electron localisation function

ELF compares the kinetic energy of the electrons at a point with that of a
uniform electron gas of the same density. It runs from 0 to 1. A value near 1
means that electrons of opposite spin are paired and kept apart from other
pairs: core shells, bonds and lone pairs show up as separate basins. A value
near 0.5 is what the uniform gas has; a value near 0 means the electrons are
not localised.

- **Isovalue** (default 0.80): the surface is drawn where ELF equals this
  value. At 0.80 the cores, bonds and lone pairs of a small molecule are
  separate blobs; lower it towards 0.5 and they merge into one body, raise it
  and only the most strongly localised regions remain. Type a value between 0
  and 1 in the box and press Enter.

## LOL: the localised orbital locator

LOL answers the same question as ELF from the same ingredients, with a
different formula, so it has a similar meaning and also runs from 0 to 1.
It is often sharper in the bonding region between two atoms, and it does not
depend on the density gradient in the same way, so the two can differ in
detail. Read it like ELF: high values lie in localised pairs.

- **Isovalue** (default 0.50): where the surface is drawn. LOL values are
  generally lower than ELF values for the same feature, so a lower default is
  used. The box works as for ELF.

Both functions are meaningless where there is almost no electron density, so
they are left out wherever the density is below 0.001 atomic units, the value
at the usual van der Waals surface.

The surface of both views has the same colour and transparency as the density
surface, set by the colour buttons of the panel.

## NCI: non-covalent interactions

NCI shows where two parts of a system interact without a covalent bond
between them. It uses the reduced density gradient

s = |grad rho| / (2 (3 pi^2)^(1/3) rho^(4/3)),

which is small both at a bond and wherever two electron clouds meet and the
density has a flat region between them. Covalent bonds and atomic cores are
removed by showing the surface only where the electron density is low. What
remains is a thin surface in each region of weak interaction.

The surface is coloured by sign(lambda2) rho: the density times the sign of
the middle eigenvalue of the density Hessian. The sign says whether the
density is being pulled together or pushed apart across the region, and the
density says how strong the interaction is. The colour scale is fixed, from
-0.035 to +0.02 atomic units, and is drawn as a bar under the panel:

- **blue**: strongly attractive, such as a hydrogen bond;
- **green**: weak and close to zero, such as van der Waals contact;
- **red**: repulsive, such as steric crowding inside a ring or a cage.

A hydrogen bond therefore appears as a small blue to blue-green disc between
the hydrogen and the acceptor atom, a van der Waals contact as a broad green
sheet, and a strained ring as a red patch in its middle.

- **Isovalue** (default 0.50): the value of s the surface is drawn at. A
  lower value gives tighter, thinner patches around the interaction; a higher
  one gives larger, more connected surfaces. Type a value and press Enter.
- **Density cut-off** (default 0.05 atomic units): the surface is left out
  wherever the electron density is above this value. Raising it brings back
  the regions near the bonds; lowering it keeps only the weakest contacts.
  Press Enter to apply it; the surface is rebuilt without a new calculation.

The colour bar also repeats the words above, and the surface and the colours
update when the isovalue or the cut-off changes.
