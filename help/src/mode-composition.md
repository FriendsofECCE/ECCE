# Mode composition of vibrations

The **Vibrational Frequencies** panel can tell you what each normal mode is
made of: how much of its motion is a bond stretch, how much an angle bend.
This is how the amide bands are assigned: amide I is mostly the C=O stretch,
amide II mostly the N-H bend together with the C-N stretch.

## The table

Click **Composition...** beside the Graph/Table choice. The window lists every
vibration with its frequency and its largest contributions, in percent, for
example `C2-O3 stretch 80 %`. Atoms are numbered as in the structure, starting
at 1, so `C2-O3` is the bond between atoms 2 and 3.

The automatic set of coordinates is every bond, and the bends at every atom
other than hydrogen. Bends that move a hydrogen on carbon are left out, since
they overlap everything else around that carbon; add them by hand if you need
them. To add coordinates, type atom numbers in the box and press **Apply**:

- two atoms, `3-4`: the stretch of that pair;
- three atoms, `9-4-2`: the bend at the middle atom;
- four atoms, `1-2-4-5`: the torsion about the middle bond.

Separate several with commas or spaces.

## Colouring the spectrum

Select a bond (two atoms) in the 3-D view and the peaks of the modes that lie
mostly along that bond are drawn thick and in amber in the spectrum; the
stronger the share, the stronger the colour. Modes below 10 % stay as they
were. Select a larger group of atoms, such as a CH3 group, the amide C(=O)NH
or a carboxylate, and the share of every stretch, bend and torsion lying
entirely within those atoms is added up. The key in the spectrum says what is
highlighted. Select fewer than two atoms to clear it. Clicking a peak still
selects the mode.

## What the percentages are

For each mode ECCE finds how its atoms move along each chosen coordinate and
shares the mode's kinetic energy out over the coordinates, taking into account
that coordinates which overlap (the C=O stretch and the neighbouring C-C-N
bend, say) move together. The shares add up to 100 % over the chosen set.

This is **not a full potential energy distribution (PED)**. A PED needs a
complete, non-redundant set of coordinates and the force constants of the
calculation, and ECCE uses neither. The percentages show which motions
dominate a mode and are good for naming a band; do not expect them to match a
published PED to the percent, and treat shares of a few percent as noise. A
full PED may follow later.
