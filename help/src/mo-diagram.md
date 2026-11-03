# The MO diagram

The MO diagram draws the molecular orbitals of a calculation as energy levels
and connects each one to the orbitals of the parts it is built from, so you
can see which orbitals bond, which antibond and which do not take part. Open
it from the **Properties** list, beside the **MOs** panel. It is
experimental: read it as a model of the bonding, and check a surprising level
against the orbital itself.

For a relaxed scan or an IRC the diagram belongs to the last computed point.
For ECCE-QM it follows the point selected in the **Relaxed Scan** or **IRC
Path** panel instead, as do the **MOs** panel and its orbital list.

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

## Core orbitals

Tick **Show core orbitals** to draw the core orbitals, which are otherwise left
out. They sit below a break in the energy axis, labelled **core, not to scale**,
each as a non-bonding level named by its shell, for example F 1s, with a line to
the atom it belongs to. An orbital counts as core when it is at least 90 % the
orbitals of one element with the same angular momentum and lies among the
deepest of them. The option is on by default for molecules of up to five atoms
and off for larger ones; your choice is remembered. A one-atom calculation has
its own view and does not offer it.

## Shell names

Orbitals described by their atomic contributions, here in the hover text and
under the diagram and in the **Main atoms** column of the MOs table, are named by
element and shell with the principal quantum number: F 1s, F 2s, F 2p, H 1s.
The number comes from the element: the deepest orbitals made of one element's
functions take its core shells (1s, 2s, 2p, ...) in order, and the others take
the valence shell, the period of the element for s and p. When an element occurs
more than once its atoms are numbered, H(1) 1s and H(2) 1s.

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

Clicking a level also writes its description under the diagram. Untick
**Describe clicked level** to hide that text; hovering a level still shows it
as a tip. On the right-hand column, labels of levels that lie close together
are moved apart vertically and joined to their level by a thin line.

## A single atom

For a calculation on one atom there is nothing to correlate, so the panel
shows the atom's orbitals filled with electrons, the way aufbau is drawn in a
textbook. The staircase has one column per principal shell n, with s, p and d
subshells stepping to the right inside it; the lowest energy is at the bottom.
The boxes fit the panel, and empty subshells have dashed grey outlines. Arrows show the
electrons: in a partly filled subshell the orbitals take one electron each
before any takes a second (Hund's rule). The lowest empty subshells are drawn
empty. The electron configuration, for example O: 1s² 2s² 2p⁴, is written
above the drawing. The rows are in the right order but not to scale, and a
subshell is placed by the mean energy of its orbitals.

Tick **Energies to scale** to draw the orbital energies to scale on an eV
axis instead. The core levels are below a break in the axis and are not to
scale, with the lowest at the bottom. The s, p and d columns form a centred
group. Energies appear to the right of the levels in grey; degenerate levels
share one value. A value is omitted if it would overlap another label or
level; hovering still shows the energy. In a restricted open-shell
calculation (ROKS or ROHF) the orbitals of a partly filled subshell do not
have the same energy, and this drawing shows the splitting. For an
unrestricted calculation up arrows are the alpha electrons and down arrows the
beta electrons.

Click a box or a level and its orbital is drawn in the 3-D view by the **MOs**
panel, as in the diagram of a molecule.
