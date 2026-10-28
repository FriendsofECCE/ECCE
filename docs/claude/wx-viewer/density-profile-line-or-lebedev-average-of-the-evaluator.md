---
type: map
title: "The Density Profile panel plots DensityPlane::Evaluator along a line through two atoms (LineProfile) or as a Lebedev spherical average about an atom (RadialDist); the evaluator works in bohr, atom shell labels are inferred, aug- sets are refused by ECCE-QM"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["include/tdat/RadialDist.H", "include/tdat/LineProfile.H", "src/tdat/chemistry/RadialDist.C", "src/tdat/chemistry/LineProfile.C", "src/apps/lineprofile/lineprofile.C", "src/apps/builder/DensityProfilePanel.C", "src/apps/builder/XYPlotCanvas.H", "src/apps/builder/BuilderPlotShot.C", "tests/tdat/radialdist_test.C", "tests/apps/densityprofile_panel_test.py", "tests/apps/densityprofile_pick_test.py", "tests/builder/make_radialdist_fixtures.py", "help/src/density-profile.md"]
issues: []
---
- **Core.** `RadialDist::compute` averages a `Spec` (the total density, or a
  weighted sum of |psi|^2 over members) over a Lebedev grid about a centre and
  multiplies by 4 pi r^2 (`Options::radial`; else the plain average).
  `LineProfile::compute` samples the same specs along a line (psi signed, psi^2,
  density). The Lebedev tables are `src/qm/lebedev_data.inc` included by
  relative path (self-contained, no engine link). `Evaluator::orbital/density`
  take Angstrom coordinates but return **bohr** units; both cores convert.
- **Contributions.** `orbitalSpecs` makes one curve per occupied orbital with
  weight n_i (the density's share), `combine` joins a shell label or an
  equal-energy set; `maxSumError` checks they add up to the total (1e-13 for
  Ar). Unrestricted: only alpha + beta adds up.
- **Shell labels** ("1s", "2s", "2p") exist only for a single atom's occupied
  MOs: dominant (>=90%) angular momentum counted up in energy order, a
  degenerate set one shell. ECCE-QM stores no ORBSYM for atoms.
- **Maxima** are local maxima above 1% of the curve's top, parabolic refinement.
- **Gaussian basis has no cusp**: along a line through a nucleus, psi_1s
  normalised at s = 0 is 5% off exp(-|s|/a0); normalised at 0.25 A it is within
  2% out to 3 A (cc-pVQZ). The density at the nucleus is ~6% low.
- **Pitfall: aug- sets.** The library's plain `AUG-CC-PVTZ.BAS` holds only the
  diffuse functions and ecce-qm refuses aug- names ("only the diffuse
  augmentation"); `cc-pVQZ` is the large hydrogen basis in the fixtures
  (`make_radialdist_fixtures.py` via `tests/modiagram/makecalc.py`).
- **Plot.** `XYPlotCanvas::addVLine` draws a dashed line with its label in its
  own row under the tick labels (a label that would touch the previous one is
  dropped); `setZeroLine` marks psi = 0. The key sits above the plot.
- **Hooks** `dpopen dpmode dppick dpselect dpatoms dpmargin dpcentre dpquantity dpshow dpmo dpspin
  dpunit dpcombine dpcsv dppng dpstate dpclose` (`Builder::densityProfileCommand`),
  pictures with `plotshot <name> 0 0 Density Profile:docked`;
  `densityprofile_panel_test.py` runs Ne, Ar, H2 and N2 at 1366x768 and 1700x1000.
- **Viewer picks (#580).** The panel is a `VizPropertyPanel` + `ecce::Listener`
  (`drawsInViewer() == false`, like the Density Map): two picked atoms set the
  line, one the centre. `dpselect` sets `m_atomHighLight` and publishes
  `SelectionChanged` (the real event path); `dppick` calls `pickAtoms` directly.
- **Key and defaults (#581).** `XYPlotCanvas::setKeyMaxRows(2)` caps the key and
  ends it with "+N more (hover)"; the plot keeps >= 110 px whatever the key wants.
  A molecule with more than 8 occupied orbitals opens on Total + the 4 largest
  integrals along the line (`rankLargest`, re-ranked until the user touches
  Show, the list or the mode).
- **Atom average is threaded (#629).** `RadialDist::compute` runs on a detached
  `std::thread` on copies of the specs/options and a shared evaluator
  (`p_eval` is a `shared_ptr`); the result returns via `wxTheApp->CallAfter`
  and is drawn only if its `AverageJob` is still `p_job` (a newer request
  cancels it) and the panel is alive (`p_alive`). The progress dialog is made
  by a timer after 0.5 s. Hooks call `waitForCompute()` after each command, so
  `dpstate`/`dpcsv` see the finished curves. `compute` evaluates the basis once
  per point for all specs (`Evaluator::evaluateAt`), same bits as `value()`.
