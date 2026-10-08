# Molecules for teaching

The Structure Library has a library `Teaching`, for the molecules of a first
chemistry laboratory. In it the folder `Diatomics` holds six small molecules:
`C2`, `N2`, `O2`, `CO`, `HF` and `He2`.

Each is drawn at its measured bond length (the distance between the two
nuclei in the molecule at rest):

| Molecule | Bond length (Angstrom) |
|---|---|
| C2 | 1.243 |
| N2 | 1.098 |
| O2 | 1.208 |
| CO | 1.128 |
| HF | 0.917 |
| He2 | 3.0 |

The lengths are the equilibrium values of the NIST Chemistry WebBook
("Constants of Diatomic Molecules", from Huber and Herzberg), rounded to
0.001 Angstrom. Two helium atoms do not form a bond; 3.0 Angstrom is only a
distance at which to look at them.

## Why use these instead of drawing the molecule

When you draw a bond in the Builder, ECCE places the second atom at the sum of
the two atoms' single-bond radii, which is a long bond for a molecule such as
N2 or CO (1.50 Angstrom instead of about 1.1). **Build > Clean** shortens it,
but only as far as the bond order you have set allows. The order of the
orbitals, and so which one is the highest occupied, can depend on the bond
length: for N2 and CO the order at the drawn length differs from the order
at the measured one. Starting from the library avoids that, and every
student starts from the same structure.

## Use a molecule from the library

1. In the Builder, choose **Mode > Add Structure** (Ctrl+7), or click the
   button with the tooltip **Import from Structure Library** in the Mode
   Toolbar.
2. In the **Structure Library** panel, choose `Teaching` in the
   **Libraries** drop-down, then open the folder `Diatomics` by
   double-clicking it.
3. Click a molecule in the list. A preview appears.
4. Click in the empty 3D view. The molecule is added there.

## Set the spin of O2

The library stores the atoms and the bond, not the spin. O2 in its ground
state is a triplet, so for O2 you must set the spin yourself:

1. In the Calculation Editor, set **Spin Mult.:** to 3.

All the other molecules in the folder have all electrons paired, which is the
default (**Spin Mult.:** 1).
