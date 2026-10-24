# Localised orbitals: the valence-bond view

The orbitals a calculation gives are spread over the whole molecule. The
localised orbitals describe the same electrons as sigma bonds, pi bonds, lone
pairs and core orbitals, the way organic chemistry draws them. They are
available for ECCE-QM calculations with a closed-shell wavefunction (RHF or
RKS); other codes and open-shell calculations do not have them.

## Show them

1. Open a finished ECCE-QM calculation and choose **Properties > MOs**.
2. In the box next to **Compute**, choose **Localised MO** instead of **MO**.
3. The table now lists the localised orbitals: the core orbitals first, then
   the bonds and lone pairs atom by atom. The **Bond** column names each one.
4. Select a row and click **Compute** to draw it.

## Reading the names

- `core C1`: the 1s-like orbital of one atom.
- `sigma C1-C2 (sp1.6/sp1.6)`: a bond along the line between the two atoms.
  The numbers give the mix of s and p on each atom: sp1 is half s, sp2 is a
  third s, sp3 a quarter. A hydrogen has only s.
- `pi C1-C2`: a bond made of p orbitals across the line between the atoms.
- `lone pair O2 (sp1.2)`: two electrons on one atom. `(p)` is a pure p orbital.
- `delocalised ...`: an orbital that belongs to more than two atoms.

The names are worked out from how much of each orbital sits on each atom, so
they are a description, not a measurement.

## Same wavefunction

Localising mixes the occupied orbitals among themselves. The total electron
density and the energy are not changed; only the way the electrons are
divided into orbitals. Empty orbitals are not localised, and their energies
are not defined for the localised set, so the list shows only occupied ones.

Calculations of ECCE-QM can switch this off with the input keyword
`localise false`.
