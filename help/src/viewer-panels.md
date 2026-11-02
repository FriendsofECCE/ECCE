# Arranging the Viewer's panels

Each result the Viewer shows (Geometry Trace, Vibrations, MOs, Density Map,
Density Profile, spectra and the others) opens as a panel docked beside the
3-D view. Several panels have plots and tables that need more room than a
docked column gives, even on a 1920 × 1200 screen. Any panel can be taken out
of the Viewer into a window of its own, which you can make as large as you
like or move to a second screen.

## Surfaces

The **Surfaces** category in the **Properties** list opens the whole-molecule
surface views, in this order: **Density**, **Spin Density**, **ESP on Density**.
Only views supported by the calculation are offered; **Spin Density** needs
an open shell. These entries use the MOs panel's existing controls. Choose
the view, then click **Compute**; no atom selection is needed.

- **Density**: total electron density ρ(r). The surface encloses the region
  where ρ exceeds the isovalue, in atomic units.
- **Spin Density**: ρ_α − ρ_β, showing where the unpaired electrons are.
  The two colours mark alpha and beta excess.
- **ESP on Density**: electrostatic potential on a density surface, with
  red negative (electron-rich) and blue positive.

The selected view's description appears below the controls and in the
field selector's tooltip. The field selector also offers these views from
**MOs**, including the separate **ESP on Density (atomic charges)**
approximation when atomic charges are available.

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

## Making room in the docked layout

- Drag the divider between the 3-D view and the panel column to make the
  column wider.
- Drag the divider between two docked panels to give one of them more height.
- The Viewer remembers the layout for the next time it opens.
