---
type: map
title: "The Radial Distribution panel is a Lebedev spherical average of DensityPlane::Evaluator in wx-free RadialDist; the evaluator works in bohr, atom shell labels are inferred, aug- sets are refused by ECCE-QM"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["include/tdat/RadialDist.H", "src/tdat/chemistry/RadialDist.C", "src/apps/radialdist/radialdist.C", "src/apps/builder/RadialDistPanel.C", "src/apps/builder/BuilderPlotShot.C", "tests/tdat/radialdist_test.C", "tests/apps/radialdist_panel_test.py", "tests/builder/make_radialdist_fixtures.py", "help/src/radial-distribution.md"]
issues: []
---
- **Core.** `RadialDist::compute` averages |psi|^2 (one MO) or rho (total) over
  a Lebedev grid about a centre and multiplies by 4 pi r^2. The Lebedev tables
  are `src/qm/lebedev_data.inc` included by relative path (self-contained data,
  no engine link). `Evaluator::orbital/density` take Angstrom coordinates but
  return **bohr** units, so the curve is per bohr and is divided by 0.529 for an
  Angstrom grid; the `r` grid is in the output unit.
- **Maxima** are local maxima above 1% of the curve's top with a parabolic
  refinement (error ~3e-5 bohr on a 0.01 bohr grid); the first-row atoms show
  n - l maxima (Ne 2s: 0.043 and 0.360 A, inside the 1s maximum at 0.055 A).
- **Oracles in `radialdist_test`**: H 1s in cc-pVQZ against 4 r^2 exp(-2r)
  (maximum at 0.5295 A, curve within 0.2%), Ne cc-pVTZ populations 1.000 and
  density 10.000. The fixtures are real ECCE-QM runs
  (`make_radialdist_fixtures.py`, via `tests/modiagram/makecalc.py` with
  `--basis/--multiplicity` and single atoms).
- **Pitfall: aug- sets.** The library's plain `AUG-CC-PVTZ.BAS` holds only the
  diffuse functions and the `-AGG` file is refused by ecce-qm ("only the diffuse
  augmentation"); use `cc-pVQZ` for a large hydrogen basis.
- **Shell labels** ("1s", "2s", "2p") exist only for a single atom's occupied
  MOs: the dominant (>=90%) angular momentum, counted up in energy order, with
  a degenerate set one shell. ECCE-QM stores no ORBSYM for atoms; a code that
  does has its own label shown instead.
- **Script hooks** `rdopen rdclose rdcentre rdmo rdtotal rdspin rdunit rdcsv
  rdpng rdstate` (`Builder::radialDistCommand`), pictures with
  `plotshot <name> 0 0 Radial Distribution:docked`; `radialdist_panel_test.py`
  runs them on neon at 1366x768 and 1700x1000.
