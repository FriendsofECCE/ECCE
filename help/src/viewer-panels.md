# Arranging the Viewer's panels

Each result the Viewer shows (Geometry Trace, Vibrations, MOs, Density Map,
Density Profile, spectra and the others) opens as a panel docked beside the
3-D view. Several panels have plots and tables that need more room than a
docked column gives, even on a 1920 × 1200 screen. Any panel can be taken out
of the Viewer into a window of its own, which you can make as large as you
like or move to a second screen.

## Surfaces

The **Surfaces** entry under **Orbitals** in the **Properties** list opens
the whole-molecule surface views. Pick one in the drop-down, in this order:
**Density**, **Spin Density**, **ESP on Density**, **ELF**, **LOL**, **NCI**
(and **ESP on Density (atomic charges)** when atomic charges are available).
Only views supported by the calculation are offered; **Spin Density** needs
an open shell. Each view shows only its own settings (isovalue, and for NCI
the density cut-off) and then **Compute**; there is no orbital table and no
atom selection.

- **Density**: total electron density ρ(r). The surface encloses the region
  where ρ exceeds the isovalue, in atomic units.
- **Spin Density**: ρ_α − ρ_β, showing where the unpaired electrons are.
  The two colours mark alpha and beta excess.
- **ESP on Density**: electrostatic potential on a density surface, with
  red negative (electron-rich) and blue positive.
- **ELF**: electron localisation function, 0 to 1; values near 1 mark
  localised pairs: bonds, lone pairs, core shells.
- **LOL**: localised orbital locator, 0 to 1; as ELF, with sharper
  separation between basins.
- **NCI**: reduced density gradient at low density, coloured by
  sign(λ2)ρ: blue hydrogen bonds and other attraction, green weak contacts
  with little density overlap, red steric repulsion.

The selected view's description appears below the controls and in the
drop-down's tooltip. The drop-down in **MOs** lists orbitals only.

**Density Map** and **Density Profile** remain under **Orbitals**. They plot
a plane or a line through chosen atoms; see [Density Map](density-map.md)
and [Density Profile](density-profile.md); [Density of States (OPDOS)](density-of-states-opdos.md) plots the density of states and the overlap population between two atom groups.

## Stress tensor of a periodic calculation

Open **Properties > Geometry > Stress Tensor** for a Quantum ESPRESSO or
ML potential result. The table shows the symmetric Cartesian tensor in
GPa, with **tension positive**. ECCE negates the stored compression-positive
tensor: QE prints stress with compression positive (its pressure is the
mean diagonal); MLP stores minus ASE's tension-positive stress, in kbar.
One GPa is 10 kbar. CP2K stress ingestion is not available.

With a unit cell, three principal axes pass through the cell centre.
Each is a pair of arrows: **blue outward for tension**, **vermillion inward
for compression**. Their lengths are proportional to the absolute principal
values on one common scale; labels beside the axes give GPa and kbar.
An exactly zero principal stress has no arrow. Degenerate principal values
have no unique directions within their shared eigenspace.

Below the table are pressure **p = −tr(σ)/3**, von Mises stress
**√[((σ₁−σ₂)²+(σ₂−σ₃)²+(σ₃−σ₁)²)/2]**, and maximum shear
**(σmax−σmin)/2**, all in GPa. Pressure also appears in kbar and agrees
with stored PRESSURE to the code's output rounding. The words about
shrinking and expanding use the normal stress **v·σ·v / |v|²** along each
cell vector a, b and c, including oblique cells. Tension indicates a tendency
to shrink on release, compression to expand; this is a qualitative stress
reading, not a prediction of strain without elastic constants.

**Show in viewer** hides or restores the arrows; closing the panel or
showing another result removes them. The tensor and arrows refer to the
**final geometry only**, including for vc-relax. Selecting a trace step does
not supply a stress tensor for that step. A directional stress surface is
not available.

## A panel in its own window

- **Take a panel out:** drag its title bar (the grey bar with the panel's
  name) away from the Viewer and let go. The panel becomes a window of its
  own; resize it from its edges.
- **Put every panel back:** choose **Tools > Dock Floating Panels**. Dragging
  a window back onto the Viewer is unreliable on Linux, so use this menu
  item instead.
- **Fold a panel:** the triangle at the right of the title bar, beside the
  cross, folds the panel to its title and opens it again, to give the other
  panels room.
- **Close a panel:** the cross at the right of the title bar. Open it again
  from the **Properties** menu.

The 3-D view and a floating panel stay linked: picking atoms, choosing a
mode or an orbital, or stepping through a trajectory updates both.

## Cutting the structure open

The **Clipping plane** panel (Tools menu in the Builder) removes everything on
one side of a plane, so the inside of a cluster, a cell or an orbital surface
can be seen. Tick **Clip**, then choose the plane:

- **Plane normal to x**, **y** or **z**: the Cartesian axes.
- **Miller plane (h k l)**: a plane of the unit cell; only for a periodic
  structure.
- **Plane of three selected atoms**: select three atoms first.

The **Offset** slider moves the plane across the structure, in angstrom from
its middle (from the three atoms' centre for an atom plane). **Flip side**
keeps the other half. The plane cuts atoms, bonds, orbital and density
surfaces and the unit cell, and a saved picture shows the cut.

## Sphere and cylinder quality

New display styles choose smooth atom spheres and bond cylinders automatically:
quality 90 for up to 2,000 drawn atoms, 70 for up to 4,000, and 50 for larger
structures. Small structures can gain further detail when enlarged; large MD
boxes keep the lower default for responsive rotation. Equivalent atoms drawn
at cell boundaries follow the style's quality too.

In **Render > Custom Style...**, releasing **Sphere quality** or **Cylinder
quality** sets that style's quality explicitly. Your chosen value is kept,
including 50, and takes precedence over automatic quality. Changing another
style setting leaves automatic quality active. Previously saved quality values
are also kept. PNG export and printing use the same scene quality; automatic
detail can increase for a larger output image.

## Transparent atoms

In the Display panel (and in **Render > Custom Style...**), the **Atom Opacity** slider (0 to 100 %,
default 100) makes the atom spheres of a style see-through, so atoms behind
them show. The setting belongs to the style: use **Update** on the selected
atoms to give only those atoms a style with another opacity. Bonds stay
opaque, and a saved picture shows the opacity. Transparent atoms are drawn
back to front; where bonded and non-bonded atoms overlap in the view the
order between those two groups is approximate.

## Making room in the docked layout

- Drag the divider between the 3-D view and the panel column to make the
  column wider.
- Drag the divider between two docked panels to give one of them more height.
- The Viewer remembers the layout for the next time it opens.

## Display settings

**Render > Display Settings...** opens the Display panel in the Builder's side
column. It gathers the rendering settings that are otherwise spread over the
Render menu and the right-click menus, and every change it makes is the same
change the old place makes.

**Style** holds Display style, Color scheme, Ball and Stick Scale, Cylinder
Radius, Sphere Quality and Cylinder Quality. The two quality sliders have an
**Automatic (by atom count)** check box: when it is ticked the detail follows the
number of atoms drawn; moving the slider unticks it and keeps your value.
**More... (Custom Style)** opens the full Custom Style dialog.

**Colors** has the background and foreground colors, and the colors of
orbital and density surfaces: a positive and a negative lobe color, the four
presets (R/G, R/B, B/Y, L/D), Solid, Mesh or Contour, and an opacity slider.
These surface settings are shared by orbitals, densities and cube files, and
are remembered the next time you start ECCE. The right-click menus of the
orbital and cube panels still work and change the same settings. Cell edges
sets the color of the unit cell edges.

**Labels** has the Atom Labels choice, bond, residue and summary text labels,
the axes, and the a/b/c labels on the unit cell.

**Reset to defaults** returns the style, background and surface colors,
opacity, surface type and cell settings to their original values.
