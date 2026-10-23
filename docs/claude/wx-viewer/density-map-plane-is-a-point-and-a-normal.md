---
type: map
title: "The Density Map's plane is one model (point + normal) in wx-free DensityPlane; picks, the view and the offset all set it, and the panel draws it in the viewer as a StyledPlane"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["include/tdat/DensityPlane.H", "src/tdat/chemistry/DensityPlane.C", "src/apps/builder/DensityMapPanel.C", "src/apps/builder/BuilderPlotShot.C", "src/apps/builder/MoPanel.C", "tests/tdat/densityplane_test.C", "tests/builder/densitymap_shot.C", "tests/builder/densitymap_viewer_capture.py"]
issues: [215]
---
- **Model.** `Plane` = origin, orthonormal u/v, normal n, plus `offset` and `base`.
  `makePlane` (molecular / three atoms / two atoms perpendicular to the molecular
  plane), `makeViewPlane` (camera right/up, through the atoms' centroid, stored as
  a plane, not as the view) and `offsetPlane` (always from the base plane, so
  offsets replace, not add). Window = atoms' extent + 1.5 A (`window()`).
- **Picks** come from the `SelectionChanged` event (`frag->m_atomHighLight`, Nubs
  skipped): 3 atoms = through them, 2 = through the bond standing on the
  molecular plane. Viewer and calculation atoms are matched by index; a count
  mismatch is ignored. The drop-downs follow the picks and stay the keyboard route.
- **The orbital follows the MOs panel**: `MoPanel::selectMo` and the grid-select
  handler call `tellDensityMap()` -> `DensityMapPanel::followMo`. The panel is a
  `VizPropertyPanel` with `drawsInViewer() == false` only to get `getFW()`; taking
  viz focus would switch the MOs panel's surface off (see the MO diagram entry).
- **Plane in the viewer**: `SGContainer::addPlane(new SGPlane, "DensityMapPlane")`
  (the Slice tool's StyledPlane facility, transparent), corners from `window()`;
  removed in `Show(false)` and the destructor. The viewer's fragment is assumed to
  be in the stored (last-step) frame.
- **Degenerate sets** use 1.5e-5 Eh, the MO diagram's `groupByIrrep` tolerance.
  The sum is of psi^2 (cylindrical for a linear molecule), not of psi; a single
  degenerate component is an arbitrary rotation inside the set and the caption says
  which one it is.
- **Grid**: full 0.05 A (0.1 A above 20 atoms); while the slider is dragged
  `max(0.15, 2*full)`; the full grid is computed on release.
- **Hooks** (Builder scene commands, `BuilderPlotShot.C`): `densitymap`, `dmpick`,
  `dmoffset X [drag]`, `dmview`, `dmviewcurrent`, `dmfollow N`, `dmcsv`, `dmpng`,
  `dmstate`, `dmhide`; `atomclick` clicks real atoms. Headless without the Builder:
  `densitymap_shot` (`--plane`, `--sum`, `--offset`, `--view`) and `densityplane`.
- **Test fixtures**: `tests/builder/densitymap-fixtures` are local-data-mode copies of
  the modiagram n2 and ethene calculations (`make_densitymap_fixtures.py`); benzene
  and water are the mocomp fixtures. The water electron count needs a grid with no
  point on the nucleus and h <= 0.05 A: the tightest 1s primitive is sharper than
  a 0.2 A grid, which gave 8.8 electrons.
