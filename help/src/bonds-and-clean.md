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

## Distorting the bonds to one atom

To start a small distortion, such as a Jahn-Teller elongation or a polaron
around a metal ion, select the centre atom and choose **Edit > Distort
Bonds...**. The dialog lists every atom bonded to it with its present
distance and a field for the change in Å: positive stretches the bond,
negative shrinks it. Each neighbour moves along the bond, and the centre does
not move. Atoms hanging off a neighbour, such as the hydrogens of a water
ligand, move with it, so the ligand keeps its shape.

**All the same** fills every field with one value. **Jahn-Teller** fills the
fields for an atom with six bonds: the pair of trans bonds closest to the
chosen axis (x, y, z, or the longest pair) changes by the value, and the other
four by minus half of it. Press OK to apply. It is one step in **Edit > Undo**.

This only moves atoms to seed a distorted geometry. Run **Clean** or an
optimisation afterwards and the structure will relax towards its own minimum.

## Taking coordinates from another calculation

To put the cluster from one calculation together with the counter-ion from
another, choose **Edit > Take Coordinates From...**. Pick a calculation in
the same folder, and give the atoms to take from it as numbers from 1, such
as `1-12,40`; the field starts with the atoms currently selected. The two
structures must have the same number of atoms in the same element order. If
they do not, nothing changes and the message names the first atom that
differs.

The chosen atoms get the other structure's coordinates and every other atom
keeps its own. If the two were built in different frames, tick **Align
first**: the other structure is fitted onto this one over the atoms that are
not taken, by rotation and translation only (a mirror image is never fitted),
and the RMSD of that fit is shown. At least three atoms must remain outside
the chosen set. It is one step in **Edit > Undo**.
