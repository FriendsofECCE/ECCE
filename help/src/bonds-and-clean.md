# Bonds in the Builder

You do not choose a bond order in the Builder. A bond is **covalent** or
**metal-π**, and the only thing you set on an atom is its geometry, that is
how many nubs it has. A trigonal planar carbon has three, a linear carbon
two, a tetrahedral carbon four.

Every covalent bond is drawn as one stick, in every display style, however
the structure was made or read.

## Building

- **Atom** mode adds an atom of the element and geometry shown in the Build
  panel. Its first bond is covalent.
- **Bond** mode joins two atoms you select with a covalent bond. The tool's
  drop-down offers one other kind, **Metal-π**, for a π ligand over a metal
  (see [Organometallic complexes](organometallics.md)).

To build ethene, give two carbons the trigonal planar geometry and bond them;
for ethyne use the linear geometry. Ethane cannot be given a triple bond,
because tetrahedral carbons have no spare valence to put in one.

## What Clean assumes

Force Field Clean needs a bond order to know a bond's natural length, so it
works one out from the atoms alone, by this rule. A nub or hydrogen counts as
a neighbour. Each carbon, nitrogen, oxygen and sulfur atom has as many spare
valences as its usual valence (4, 3, 2, 2) minus its neighbours, and bonds
between atoms with spare valence are raised until none is left.

| Atoms | Result |
|---|---|
| two trigonal carbons (or C and N) | double bond |
| two linear carbons | triple bond |
| linear C and terminal N | triple bond (nitrile) |
| terminal O or S on a trigonal C | double bond (C=O, C=S) |
| linear C with two terminal O | two double bonds (CO2) |
| six trigonal C or N in a ring | alternating double bonds, drawn and treated as aromatic (1.5) |

Anything the rule cannot settle stays single and the atom is selected after
Clean: a radical, a sulfur or phosphorus atom with more than two neighbours,
and so on. Metals take no part.

Clean says what it did in the log: for example *Clean treated as multiple
bonds: C1=C2 double (both trigonal). Not resolved, treated as single and
selected: C5.* After the optimisation it also compares each bond with a typical
length for its elements and order, and names those more than 0.15 Å
off. This is a warning only; nothing is blocked.

The orders Clean used can be drawn: **Options > Advanced > Show Bond Orders
Used by Clean**. It is off by default.

## Hydrogen bonds

**Options > Show Hydrogen Bonds** draws a dashed line from a hydrogen to the
atom it is hydrogen-bonded to, and redraws the lines at every step of a
trajectory or geometry trace. The test is the one `gmx hbond` uses: the donor
D (the N, O, F or S the hydrogen is bonded to) and the acceptor A (N, O, F, or
a chloride or bromide ion with no covalent bond) are at most 3.5 Å apart, and
the angle H-D...A is at most 30°. A C-H...O contact is not drawn, because C is
not a donor. In a structure with a unit cell the nearest periodic image of the
acceptor is used; a line to an image is drawn to the acceptor in the cell.

**Options > Hydrogen Bond Settings...** changes the distance and the angle.
The values are remembered by the Builder.
