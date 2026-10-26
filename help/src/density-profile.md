# Density profile

The **Density Profile** panel plots the electron density, and what each orbital
contributes to it, against one length: the distance along a line through two
atoms, or the distance from one atom. It is calculated from the orbital
coefficients and the basis set stored with the calculation, so it works for
every code whose orbitals ECCE reads. Open it from the **Properties** list, in
the Orbitals group beside **MOs** and **Density Map**. It needs a calculation
that has orbitals.

For a relaxed scan or an IRC the profile belongs to the last computed point.
For ECCE-QM it follows the point selected in the **Relaxed Scan** or **IRC
Path** panel instead, and a note at the top of the panel names the point.

The controls are two short rows. The first row has the mode, the atoms, the
quantity and what to show; the second the axes, **Show all** and **More**,
which unfolds the controls used less often (margin, sideways shift, unit,
combine degenerate, clip peaks and the export buttons). Whether **More** is open
is remembered. The orbital list, the plot and the notes beneath it are separated
by handles that can be dragged to give any of them more room.

The first choice is the mode. Atoms selected in the 3-D view set it: select two
atoms (Shift-click adds to the selection) and the plot is **Along a line**
through them, **From** and **to**; select one atom and it is **Around an atom**
with that atom as **Centre**. The lists stay available.

- **Along a line**: the line goes through two atoms, **From** and **to**, and
  continues **Margin** ångström beyond each. The horizontal axis is the position
  along the line, zero at the first atom. A dashed vertical line marks every atom
  the line passes within 0.3 Å of, with its label under the axis. A molecule
  opens in this mode, on the line through its first bond. **Shift line
  sideways** (under **More**) moves the whole line by a distance in ångström,
  out of the molecular plane (the plane of the atoms and the line) or within
  it. A π orbital is exactly zero on the line through both nuclei; shifted
  0.5 Å it shows its two lobes. For a linear molecule any perpendicular is the
  same. The atoms are still marked where they lie along the line.
- **Around an atom**: the values are averaged over a sphere about the **Centre**
  atom, for each radius. A single atom opens in this mode. This is the way to
  see shells.

## What is plotted

**Along a line**

- **Density, n·ψ² per orbital**: the total electron density and, for each
  occupied orbital, its share of it: the occupation n times the square of the
  orbital, ψ². The shares add up to the total at every point; the panel prints
  the largest difference, which is rounding error.
- **ψ² of each orbital**: the square of each chosen orbital, without the
  occupation. Where the curve is large the electron of that orbital is likely to
  be found. Two orbitals can be compared directly even if one is empty, such as
  the bonding orbital of H₂ and the antibonding one above it.
- **ψ, signed**: the orbital itself. A zero line is drawn. Where the curve crosses
  it the orbital changes sign: a node. Parts above and below the line have
  opposite phase, which is what makes an orbital bonding or antibonding.

**Around an atom**

- **Radial distribution**: r² ∮ ψ² dΩ, the probability per unit distance of
  finding the electron at distance r in any direction. The area under the curve
  is how much of the orbital lies about this atom: 1 for an orbital of the atom,
  and the number of electrons for the total density. The r² is the area of the
  sphere, which is why the curve starts at zero even for an s orbital that is
  largest at the nucleus.
- **Spherical average of ψ²**: the same average without the r²: the density an
  electron has at that distance, averaged over the directions. It is largest at
  the nucleus for an s orbital.
- A signed ψ is not offered here: the average of a signed p or d orbital over a
  sphere is zero.

## Axes and zoom

**r axis** (around an atom) is **Linear**, **√r** or **log r**. The square root
spreads the region near the nucleus, where the inner shells are, and the
logarithm shows the weak outer tail. The tick marks are always in r itself
(0.01, 0.05, 0.1, 0.2, 0.5, 1, 2 Å), and on √r and log r the area under a
radial curve is no longer the number of electrons; the panel says so beneath the
plot. Along a line the axis is always linear.

**y axis** is **Linear** or **Log**. Log is for quantities that are never
negative (the density, ψ² and the radial distribution) and is not offered for
the signed ψ; its ticks are powers of ten. Values that are zero cannot be drawn
on a log axis and are left out.

The mouse wheel zooms about the pointer (Shift+wheel zooms the vertical axis
only), dragging a rectangle zooms to it, the
right or middle button pans, and a double click returns to the opening view.
**Show all** resets the zoom to show the whole curves, including the peaks that **clip peaks**
cut off at the nuclei.

When an orbital is zero everywhere on the line the plot says so instead of
drawing an empty axis.

The axes are labelled with their units: ψ in length^-3/2, ψ² and the density in
length^-3, the radial distribution in 1/length, where the length is ångström or
bohr, as chosen under **Unit**.

## Which orbitals

**Show** chooses what is drawn.

- **Total + all occupied**: the total, and every occupied orbital's contribution.
  This is the main view. A molecule with more than eight occupied orbitals opens
  instead on **Total + selected orbitals**, with the four orbitals that contribute
  most along the line ticked; the note under the plot says how many. Choosing
  **Total + all occupied** shows every orbital.
- The key above the plot is at most two rows. When there are more curves, the key
  ends with "+N more (hover)"; pointing at a curve names it. The plot keeps its
  height whatever the key holds.
- **Total + selected orbitals**: the total and the contributions of the orbitals
  ticked in the list.
- **Selected orbitals**: the ticked orbitals alone, without the occupation, so
  that each integrates to 1 in the radial view.
- **Total only**.

For ψ and ψ² the choices are **Selected orbitals** and **All occupied
orbitals**.

Changing the quantity keeps the orbitals you see ticked in the list and keeps
the kind of view (a selection stays a selection); with **Selected orbitals** and
nothing ticked the highest occupied orbital is ticked, and the list scrolls to
the first ticked orbital. When the **MO Diagram** is open, clicking a level in it
shows that orbital alone here, as it does in the **Density Map**.

**combine degenerate** draws one curve for each shell. For an atom the orbitals
are named by shell, 1s, 2s, 2p, 3s, 3p, from the angular momentum that dominates
each one, counted up in energy order, so the three 2p orbitals become one curve
"2p". Without a shell name, orbitals with the same energy are joined and called,
for example, "MO 6-7". The orbitals of an atom that are empty are named by their
angular type only, as "s (virtual)" or "p (virtual)": their principal quantum
number is not meaningful in the basis set. Every curve has its own colour, the
total is black and heavier, and when the colours run out the next curves are
dashed. For an unrestricted calculation choose alpha, beta, or
alpha + beta; the contributions add up to the total only for alpha + beta.

**clip peaks** (along a line) cuts the vertical axis, when the peaks are more than three times higher, a little above the
highest value found 0.35 Å or more from every atom. The density at a nucleus is
hundreds of times the density in a bond, and without the cut those peaks flatten
everything else. The curves still reach their peaks; they run off the top of the
plot. Untick it, or press **Show all**, to see the whole height.

## What to look for

- **Shells.** Around an atom, the curve of an orbital with principal quantum
  number n and angular momentum l has n − l maxima: one for 1s, 2p and 3d, two
  for 2s and 3p, three for 3s. The total of argon has a hump for each filled
  shell: 1s very close to the nucleus, then 2s and 2p together, then 3s and 3p.
- **Penetration.** The 2s curve of an atom has a small maximum inside the 1s
  shell in addition to its main one. The 2p curve has only the main one. A 2s
  electron therefore spends part of its time close to the nucleus, feels more of
  its charge and lies lower in energy than a 2p electron.
- **A line through the nucleus.** Along a line through a single nucleus, a 1s
  orbital is largest at the nucleus and falls off as exp(−r / a₀) on each side;
  a 2s orbital has a node, a 2p orbital is zero at the nucleus and has opposite
  sign on the two sides.
- **Between bonded atoms.** In H₂ the bonding orbital, σg, has the same sign
  along the whole line and its square is large between the nuclei: the density
  piles up in the bond. The antibonding orbital, σu*, changes sign at the
  midpoint and is zero there: a node between the atoms. In N₂ the π orbitals are
  zero everywhere on the line through both nuclei, since they have a node along
  the bond axis; the density on the axis comes from the σ orbitals.
- **Near a nucleus** the density has a sharp peak. A basis set made of Gaussian
  functions reproduces it a few per cent too low, and with no sharp point.

## Export

**Export .png...** saves the plot as drawn. **Export .csv...** saves the
position and every curve, with the unit and the atoms on the line in the
header.
