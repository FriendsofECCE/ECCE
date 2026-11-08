# Arranging the Viewer's panels

Each result the Viewer shows (Geometry Trace, Vibrations, MOs, Density Map,
Density Profile, spectra and the others) opens as a panel docked beside the
3-D view. Several panels have plots and tables that need more room than a
docked column gives, even on a 1920 × 1200 screen. Any panel can be taken out
of the Viewer into a window of its own, which you can make as large as you
like or move to a second screen.

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

## Making room in the docked layout

- Drag the divider between the 3-D view and the panel column to make the
  column wider.
- Drag the divider between two docked panels to give one of them more height.
- The Viewer remembers the layout for the next time it opens.
