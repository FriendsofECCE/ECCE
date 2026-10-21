# Stacking layers: the Layer Builder

The Layer Builder (**Tools > Layer Builder**) turns one layer into a stack
of layers. Use it for graphite, layered oxides, clays and layered hydroxides.
It only draws the structure. It knows nothing about force fields or about
the code that will run the calculation, and it proposes no spacings, shifts
or counts: you give them.

## Before you start

The structure in the Builder, with its cell, is the layer. It needs a cell
(**Tools > Periodic Builder**, then **Create Lattice** or open a periodic
CAR file) and the vectors a and b must lie in the plane of the layer. The
layer can be a unit cell made with **Generate**, or a larger one made with
**Replicate**. A replicated layer is used as it is shown, replication
included.

## The fields

In the Layer Builder panel:

- **Layers**: the number of layers in the new cell.
- **Spacing (Å)**: the distance from one layer to the next, measured along
  the normal of the a-b plane. The new cell has the same a and b, and its
  c has the height **Layers x Spacing** along that normal. If the old c
  was tilted, its sideways part is kept, so the angles of a tilted cell
  stay close to what they were.
- **Shift a** and **Shift b**: how far each layer is displaced sideways
  from the one below it, as fractions of a and b. The shift adds up: layer
  *i* is displaced *i* times the shift. Two layers in AB stacking need the
  shift once; for ABC stacking use three layers.
- **Ion**, **n**, **q**, **min Å** (two rows): an element symbol, how many
  of them go in **each** gap between layers, their charge, and the smallest
  distance each may have to any atom already there. The charge is stored as
  the atom's charge.
- **Water**, **min Å**: how many water molecules go in each gap, and the
  smallest distance from a water oxygen to any atom already there. Each
  water is a whole molecule with an O-H distance of 0.9572 A and an H-O-H
  angle of 104.52 degrees, in a random orientation.
- **Seed**: ions and water are placed at random. The same seed on the same
  layer gives the same placement; change the seed for another one.

The ion and water counts start at zero, so **Build Layers** with only the
spacing gives a bare stack. The minimum distances start at 3.5 A for ions
and 2.0 A for water. These are starting values to change, not
recommendations.

A stack of N layers has N gaps: the gap between the top layer and the
periodic image of the first layer is one of them.

## Build Layers

**Build Layers** replaces the structure with the stack, in the new cell, and
puts every atom inside the cell. Bonds are found again for the whole
structure, so the bonds within each layer are the ones the layer had. If
there is no room for what you asked for (for example 500 waters in a narrow
gap), ECCE says so and leaves the structure as it was; lower the count or
the minimum distance, or increase the spacing.

**Edit > Undo** undoes the whole operation, including the new cell. Build
Layers again with the same fields (including the seed) gives the same
result.

## Examples

- Graphite, Bernal (AB) stacking: draw one graphene layer in a hexagonal
  cell with a = b = 2.464 A and gamma = 120 degrees, with a c of any
  length. Set **Layers** to 2, **Spacing** to 3.355, **Shift a** to 0.6667
  and **Shift b** to 0.3333. The new c is 6.71 A.
- A clay with sodium ions and water: use a layer of the mineral, set
  **Layers**, the basal spacing for **Spacing**, the shift for your
  structure, the element and number of ions for each gap, and the number of
  waters for each gap.

## Limits

- The two ion rows cover up to two kinds of ion; a third kind has to be
  added by hand afterwards.
- Water placement is random and does not relax: it avoids overlaps, nothing
  more. Minimise and equilibrate before using it.
- Atoms are wrapped into the cell with the **Fold** rules of the Periodic
  Builder; bonds are not drawn across the cell faces.
