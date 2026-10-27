# Localised orbitals: the valence-bond view

The orbitals a calculation gives are spread over the whole molecule. The
localised orbitals describe the same electrons as sigma bonds, pi bonds, lone
pairs and core orbitals, the way organic chemistry draws them. They are
available for ECCE-QM calculations; for an open shell, the alpha and the beta
orbitals are localised separately and listed in two blocks.

## Show them

1. Open a finished ECCE-QM calculation and choose **Properties > MOs**.
2. In the box next to **Compute**, choose **Localised MO** instead of **MO**.
3. The table lists the localised orbitals in order of energy, the lowest at
   the bottom as in an orbital diagram. The **Bond type**, **Atoms** and
   **Hybrids** columns name each one.
4. Click a column heading to sort the table by it (core, sigma, pi, lone
   pair under **Bond type**, or by **E (Hartree)**); click again to reverse.
5. Select a row and click **Compute** to draw it.

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

Untick **Localised orbitals** under **Runtype Details** in the Calculation
Editor to switch this off.

## Spin density

Choose **Spin Density** next to **Compute** (open-shell calculations) and the
table gives way to two tables: the singly occupied orbitals, which make the
spin density, and the Mulliken spin population on each atom, worked out from
the orbital coefficients. If the populations cannot be worked out, only the
orbitals are listed and the panel says so. Choose another type to get the table
back.

In the orbital table, **Main atoms** names the largest contributions to the
orbital and adds **other** for the rest; hold the pointer over the cell to see
every atom with at least 1 %. The spin population table gives each atom's spin
population in electrons and its **Share (%)** of the total absolute spin, with
the atoms that carry most spin first.

Computing the surface can take minutes on a large cluster. A window with a
**Cancel** button appears when it runs longer than a moment.
