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
and [Density Profile](density-profile.md).

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

## Making room in the docked layout

- Drag the divider between the 3-D view and the panel column to make the
  column wider.
- Drag the divider between two docked panels to give one of them more height.
- The Viewer remembers the layout for the next time it opens.
