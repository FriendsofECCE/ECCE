# Choosing Quantum ESPRESSO settings

This page is for readers who know molecular codes with localised basis sets,
such as ORCA or NWChem, and want to know how to choose the settings of a
plane-wave calculation. [Your first Quantum ESPRESSO
calculation](qe-first-calculation.md) shows where the settings are; this page
says what values to give them.

## What is different

| With a localised basis set | With plane waves |
| --- | --- |
| You choose a basis set by name. | You choose one number, the **Wavefunction Cutoff:**, and raise it until the result stops changing. |
| Empty space costs nothing. | Empty space costs as much as space with atoms in it: the cell is filled with plane waves. |
| All electrons are treated. | A pseudopotential replaces the core electrons. Only the valence electrons are treated. |
| The molecule is alone. | The structure repeats in all three directions. A molecule is a crystal with a lot of vacuum. |
| | A crystal also needs a grid of **k-points**. |

Two things follow:

- Absolute energies mean nothing outside the calculation. The −45.68 Hartree
  of eight silicon atoms is the energy of the valence electrons with these
  pseudopotentials. Compare only energies computed with the same
  pseudopotentials, the same cutoff and, for crystals, comparable k-point
  grids.
- Every number on this page is a starting value. You check it by changing it
  and watching the result, one setting at a time.

## How to check a setting

Run the same calculation twice, once with the setting increased, and compare
the quantity you care about (an energy difference, a bond length, a lattice
constant). If it changes by less than you need, the smaller value is good
enough. A common target is 1 meV per atom for energies, which is 0.07 mRy or
0.04 mHartree.

Check energy differences, not total energies. A total energy can still be
moving while the difference you want has settled.

## Wavefunction cutoff

- Start from the cutoff recommended for your pseudopotentials. ECCE uses the
  SSSP set, whose recommended values are 30 Ry for silicon and 60 Ry for
  hydrogen, for example. With several elements, take the highest of their
  values.
- Leave **Charge Density Cutoff:** unticked. ECCE then writes 8 times the
  wavefunction cutoff for ultrasoft and PAW pseudopotentials and 4 times for
  norm-conserving ones, which is the usual choice.
- To check: raise the cutoff by about a quarter and compare.
- A cell optimisation (**Optimize cell** in **Runtype Details...**) needs a
  higher cutoff than a single energy, about 30 % more. Too low a cutoff makes
  the stress wrong and the cell too small.

## k-points

The k-points sample the electrons across the repeating crystal. A larger cell
needs fewer k-points in that direction, because the cell itself already
covers more of the crystal.

- **Molecule in a box:** choose `Gamma point only`. The molecule does not
  repeat in any meaningful way, and the Gamma point is the fastest choice.
- **Crystal:** choose `Monkhorst-Pack grid`. As a first guess, make each grid
  number times its cell length about 25 Å for an insulator or
  semiconductor, and about 50 Å for a metal:

  | Cell length | Insulator (about 25 Å) | Metal (about 50 Å) |
  | --- | --- | --- |
  | 3 Å | 8 | 16 |
  | 5.43 Å (silicon) | 4 or 5 | 9 or 10 |
  | 10 Å | 3 | 5 |
  | 20 Å | 1 or 2 | 3 |

- **Surface (slab):** use 1 in the direction across the vacuum, and the rule
  above in the two directions along the surface.
- **Offset:** leave the offsets at `0`. That grid contains the Gamma point,
  which hexagonal cells need. A shifted grid (offsets `1`) can converge faster
  for cubic insulators.
- To check: add 2 to each grid number and compare. For silicon in the
  tutorial, going from 4 by 4 by 4 to 8 by 8 by 8 lowers the energy by
  0.2 mRy per atom.

## Metals: smearing

A metal has partly filled bands, and a sharp occupation makes the
self-consistent field oscillate. Smear the occupations:

- **Occupations:** `Smearing (metal)`, **Smearing:**
  `Marzari-Vanderbilt (cold)`, **Width:** `0.01` to `0.02` Ry.
- A larger width converges faster, but changes the energy. Check by halving
  it.
- Use `Fixed (insulator)` only when you know there is a band gap. An
  insulator computed with smearing is fine, only slower.

## Molecules: the box

- Leave about 10 Å of vacuum between the molecule and its copy in the
  next cell. A box of 10 Å around a water molecule leaves about 8.
- A larger box costs time in proportion to its volume, so check by making the
  box 2 to 3 Å larger and comparing.
- A charged molecule interacts with its copies even across a large box, and
  pw.x adds a uniform background charge to make the cell neutral. Its energy
  depends on the box size. Compare charged molecules only in the same box, or
  work with neutral species where you can.

## Spin

**Spin:** `Unrestricted` alone does not make a calculation magnetic. Give a
**Starting Magnetization:** as well (between −1 and 1, `0.5` is a good start).
Without it, a calculation that could be magnetic often ends up non-magnetic.
For iron, nickel and their oxides, check that the final magnetisation in the
output is what you expect.

## Pseudopotentials

- Use pseudopotentials made for the functional you want. The SSSP set is
  made for PBE, and the functional is taken from the pseudopotential files
  unless you change **input_dft:**.
- Use the same pseudopotentials for every calculation whose energies you
  compare, for example the reactants and products of a reaction.

## A checklist before a production run

1. Cutoff at least the recommended value; raised for **Optimize cell**.
2. k-points checked by one larger grid, or `Gamma point only` for a molecule
   in a box.
3. Smearing for metals.
4. About 10 Å of vacuum around molecules and across slabs.
5. A starting magnetisation for anything that may be magnetic.
6. The same settings for every calculation you compare.
