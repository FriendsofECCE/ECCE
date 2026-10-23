# The MO diagram

The MO diagram draws the molecular orbitals of a calculation as energy levels
and connects each one to the orbitals of the parts it is built from, so you
can see which orbitals bond, which antibond and which do not take part. Open
it from the **Properties** list, beside the **MOs** panel. It is
experimental: read it as a model of the bonding, and check a surprising level
against the orbital itself.

Click a level and its orbital is drawn in the 3-D view by the **MOs** panel.
Click an atom or a bond in the 3-D view first to choose the part the diagram
is built around.

## Columns

The molecule is in the middle. Its parts are at the sides. A part is a
fragment: a central atom and its ligands, two halves of the molecule, or a
group of equivalent atoms, depending on the construction the diagram chose;
the line under the diagram names it. Energy runs upwards. Lines join a level
of the molecule to the fragment levels it is made from.

With the calculation as a column of its own, the model is drawn beside your
calculation and the two are matched level by level. Each matched level shows
how close the match is as an overlap between 0 and 1.

## Labels

Each level has two labels, one on each side.

- **On the left**, the label from the construction: how the level is built
  from the parts, for example a bonding combination, written σ, π or δ, with a
  star for an antibonding level.
- **On the right**, the symmetry label of the level in the point group of the
  molecule, for example a₁ or e.

The letters σ, π and δ follow the nodal-plane rule for the bond or axis the
diagram is built around: an orbital with no nodal plane containing the axis is
σ, with one nodal plane is π, with two is δ.

- **\*** after a label marks an antibonding level: the parts overlap with
  opposite signs.
- **nb** marks a non-bonding level: the parts barely overlap in it.
- A level with no partner on the other side is drawn without a line. That can
  be correct, for example for a level that is bonding but has no antibonding
  partner in the model.

## Lines

A line joins a level of the molecule to a level of a fragment. Its weight shows
the share of that fragment level in the molecule's level: a heavy line is a
large share, a thin line a small one. Shares below ten per cent get no line,
so a level can appear less connected than it is.

## Electrons

Arrows on a level show its electrons. Degenerate levels, which have the same
energy, are drawn as one level with the number of orbitals it holds. For an
open-shell molecule alpha and beta electrons are drawn separately.

## Under the diagram

The text under the diagram says what was built, how many levels are hidden,
and, only if something is wrong, why: an incomplete pairing, a wrong electron
count, or a fragment level that is poorly covered. **About this diagram...**
explains how the labels and lines were chosen.
