---
type: map
title: "ELF, LOL and NCI are MoPanel field types computed by ComputeMoCmd::computeFieldView; NCI rides the ESP colour-field path with its own ramp, and its density cut-off is applied when the surface is built"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["src/viz/propsgcommands/ComputeMoCmd.C", "src/viz/propsgcommands/IsoSurfaceCmd.C", "src/apps/builder/MoPanel.C", "src/wxviz/viewer/SceneScript.C", "include/viz/ComputeMoCmd.H", "tests/apps/density_fields_3d_test.py", "help/src/electron-density-views.md"]
issues: [798, 799, 800]
---
- **Field types** are `ELF_FIELD_TYPE`, `LOL_FIELD_TYPE`, `NCI_FIELD_TYPE`
  (`include/viz/ComputeMoCmd.H`); MoPanel lists them after Density and Spin
  Density whenever Density is offered. `ComputeMoCmd::execute` branches to
  `computeFieldView` before the MO loop: `DensityPlane::Evaluator::fieldsAt`
  per grid node, planes dealt out to threads, progress and Cancel through the
  same monitor as the exact ESP (`ECCE_FIELD_THREADS` sets the count). A failed
  or cancelled compute removes the cached grid (see the failed-compute entry).
- **NCI uses the ESP colour field.** The grid's field is s (`rdg`), its colour
  field sign(lambda2)rho; `IsoSurfaceCmd` takes the colour-mapped branch and
  picks `nciRampColor` (blue, green at 0, red) over the fixed
  -0.035..0.02 range. The legend is the ESP bar with the NCI ramp and the three
  words; `p_espDrawnType` names whichever colour-mapped type is on screen.
- **The density cut-off is not in the grid.** |sign(lambda2)rho| is rho, so
  `IsoSurfaceCmd` (parameter `rhoCutoff`) writes s = 100 into the lattice
  where rho is above it. Changing the cut-off re-runs `IsoSurfaceCmd`
  (`rebuildEspSurface`), no recompute; the isovalue goes through `IsoValueCmd`
  as for any density.
- **ELF and LOL are zeroed where rho < 1e-3.** In the Gaussian tail D/D0 goes
  to 0 and ELF to 1, so a 0.8 surface otherwise appears at the box edge (a
  detached cap beyond the molecule). The 1e-12 guard of the wx-free
  `DensityFields::elf` is for division only.
- **NCI refines the grid to 0.1 A** (at most about 6 million points;
  `ECCE_NCI_SPACING`): s falls below 0.5 only within ~0.1 A of an H-bond's
  critical point, and the Density grid (0.2 A) gave an empty surface.
- **Low s is inside.** The NCI surface encloses the low-s region, so
  `isosurf1->flipNormals` is set; an ELF blob is high inside like a density.
- **ELF/LOL/NCI isovalue is a box, not the log slider.** The slider's panel is
  hidden for these types and kept in step (`log10`) so `rebuildEspSurface`
  still reads it.
- **Hooks.** Scene command `field elf|lol|nci [res] [iso] [cutoff]` (computes
  and draws as the panel does; `espdump` writes an NCI grid, s and
  sign(lambda2)rho per node); `mofield NCI` + `mopanel` + `mobar` press
  the panel. `tests/apps/density_fields_3d_test.py` (water dimer) checks
  triangles for each view and a blue (< 0) sign(lambda2)rho shell between the
  donor H and acceptor O.
