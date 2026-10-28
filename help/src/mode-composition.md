# Mode composition of vibrations

The **Vibrational Frequencies** panel can tell you what each normal mode is
made of: how much of its motion is a bond stretch, how much an angle bend.
This is how the amide bands are assigned: amide I is mostly the C=O stretch,
amide II mostly the N-H bend together with the C-N stretch.

## The table

Click **Composition...** beside the Graph/Table choice. The window is a
read-only table for assigning bands and for pasting into a report. Each
vibration has its frequency, its short label (the one in the frequency
table's Mode column) and its largest contributions, in percent, for example
`C2-O3 stretch 80 %`. Every mode's cell lists its three largest
contributions, also when all of them are small, as in a large molecule with
many coordinates; the percentages say how small. The last column fills the
window and grows with it; hold the pointer over a cell to read all of it.
Atoms are numbered as in the structure, starting at 1, so `C2-O3` is the bond
between atoms 2 and 3.

There is nothing to type. The set of coordinates is every bond, the bends at
every atom other than hydrogen, and one torsion about each bond (through the
heaviest neighbour at each end), the same torsions the Mode column uses, so a
torsion is called a torsion in both. The Mode column also uses every bend and
the out-of-plane motions, grouped by equivalence, so its percentages differ.

**Include C-H bends** adds the bends that move a hydrogen on carbon (H-C-X and
H-C-H) to the set. They overlap the stretches and bends around that carbon,
so with them the shares of the other coordinates become smaller; the choice
is remembered.

**Copy table** puts the table on the clipboard, tab-separated (Mode, cm-1,
Mode type, contributions), ready to paste into a spreadsheet or a document.
**Export .csv...** writes the same table to a comma-separated file.

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
bend, say) move together. The shares add up to 100 % over the chosen set, so
they change when the set changes (for example with the C-H bends).

This is **not a full potential energy distribution (PED)**. A PED needs a
complete, non-redundant set of coordinates and the force constants of the
calculation, and ECCE uses neither. The percentages show which motions
dominate a mode and are good for naming a band; do not expect them to match a
published PED to the percent, and treat shares of a few percent as noise. A
full PED may follow later.


## Where else a mode is described

- **Spectrum window** (**Spectrum...**): under the plot a strip shows the
  selected mode: its number, frequency, irrep, IR intensity and short label,
  and below them its contributions in two columns. Hovering a peak shows the
  mode number and frequency, the short label and the IR intensity and Raman
  activity.
- **Panel**: the selected mode's line sits in its own bordered box under the
  plot, above Width and Freq. factor. It wraps; nothing is cut off.
  When the Freq. factor is not 1 it also gives the scaled wavenumber, for
  example 1842.8 cm⁻¹ (scaled 921.4).
- **Width and Freq. factor**: the factor multiplies every wavenumber in the
  plot. It starts at 1 each time the panel opens and is not remembered; the
  width is. A decimal comma is accepted in both fields, so 0,96 means 0.96.
- **Frequency table**: wavenumbers are shown with one decimal and intensities
  with two. The Mode column can be dragged wider; hover a Mode cell for the
  label and the shares.
