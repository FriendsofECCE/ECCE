# Radial distribution

The **Radial Distribution** panel plots how far from a chosen atom the
electrons of an orbital are found, and how the electron density as a whole
is spread out in shells. It is calculated from the orbital coefficients and
the basis set stored with the calculation, so it works for every code whose
orbitals ECCE reads. Open it from the **Properties** list, in the Orbitals
group beside **MOs** and **Density Map**. It needs a calculation that has
orbitals.

## What the curves mean

For an orbital ψ, the curve is

P(r) = r² ∮ |ψ(r, Ω)|² dΩ

the probability per unit distance of finding the electron at distance r from
the centre, in any direction. The integral over all directions is an average
over a sphere of radius r; the factor r² is the area of that sphere. The
area is the reason the curve is zero at the nucleus even for an s orbital
that is largest there: the sphere has no volume at r = 0. The area under the
curve is the share of the orbital found about the centre. For an orbital of
an atom that is 1.

For the total density the curve is

D(r) = r² ∮ ρ(r, Ω) dΩ

and its area is the number of electrons. Tick **per electron** to divide it
by that number, which draws it on about the same scale as one orbital.

Distances are in ångström, or in bohr if you choose **bohr** under **Unit**.
The vertical axis is then per ångström or per bohr.

## What to look for

- **Number of maxima.** An orbital with principal quantum number n and
  angular momentum l has n − l maxima: one for 1s, 2p and 3d, two for 2s and
  3p, three for 3s. The maxima are the shells.
- **Penetration.** Overlay the 2s and 2p orbitals of an atom. The 2p curve has
  one maximum, outside the 1s shell. The 2s curve has a small second maximum
  inside it, close to the 1s maximum. A 2s electron spends part of its time
  nearer the nucleus than a 2p electron does, feels more of the nuclear
  charge there, and lies lower in energy. That is why 2s is lower than 2p in
  every many-electron atom, and the same holds for 3s, 3p and 3d.
- **Shielding.** Compare the position of the outermost maximum with the
  1s maximum in the same atom. The inner electrons sit much closer in; the
  outer electrons see the nuclear charge reduced by them.
- **Shell structure of the total density.** Tick **Total density**. The curve
  has one hump for each filled shell, the innermost much the tallest in
  absolute terms.
- **In a molecule.** The curves are then measured from the centre you pick,
  and they are averages over a sphere that also cuts through the neighbouring
  atoms, so the curve is no longer that of an atomic orbital. The area under
  an orbital's curve is its share about that centre out to the plotted
  radius, which is less than 1. Use it to see how far an orbital reaches from
  one atom. Pick a different **Centre** to compare atoms.

## The panel

- **Centre**: the atom the distances are measured from. The default is the
  only atom, or else the atom nearest the centre of mass.
- **alpha / beta**: for an unrestricted calculation, which set of orbitals
  the list shows.
- **Unit**: ångström or bohr.
- **Total density** and **per electron**: the total density, and whether to
  divide it by the number of electrons.
- **The list**: every orbital with its number, symmetry label if the code
  gives one, energy and occupation. Tick the ones to draw. For a single atom
  the occupied orbitals are ticked at first; for a molecule, the HOMO.

Each curve has its own colour and a legend entry such as "MO 2 (2s)", and the
maxima are marked with a dot. Beneath the plot, one line per curve gives the
distances of its maxima and the area under it. The area is a check: it should
be 1 for an orbital about its own atom, and the number of electrons for the
total density. It is smaller if the curve is cut off at 6 ångström with part
of the orbital beyond it.

Drag a rectangle in the plot to zoom, double-click it to see everything again.

## Export

**Export .png...** saves the plot as drawn. **Export .csv...** saves the
distance and every curve, with the unit in the header.
