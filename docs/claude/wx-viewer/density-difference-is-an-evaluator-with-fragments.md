---
type: map
title: "A density difference is an Evaluator with fragment Evaluators subtracted inside density()/evaluateAt(); the map draws it as a filled diverging image, the profile as one signed curve"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["include/tdat/DensityPlane.H", "src/tdat/chemistry/DensityPlane.C", "src/apps/builder/DensityFragments.C", "src/apps/builder/DensityMapDraw.C", "src/apps/builder/DensityMapPanel.C", "src/apps/builder/DensityProfilePanel.C", "tests/tdat/densitydifference_test.C", "tests/builder/make_difference_fixtures.py"]
issues: [726, 724]
---
- **Core.** `Evaluator::addFragment` makes `density()` and `evaluateAt(wantRho)` return
  rho(this) - sum rho(fragment), each from its own MOs and basis, so the Map (DENSITY),
  `integrate`, RadialDist and LineProfile "Total" all follow with no change. Orbital
  quantities ignore the fragments. `matchFragment` refuses an atom with no same-element
  atom within 0.05 A or claimed twice; fragments are evaluated at absolute coordinates, so
  a code that reorients its geometry would not match (refused, not silently wrong).
- **Map.** `Map::difference` + `colourLimit` (99th percentile of |z|, the nuclei would
  otherwise swamp the bond region); `paintDensityMap` draws a bilinear image, blue
  gain / red loss, white zero, with a colour bar instead of the contour legend.
- **Profile.** With fragments the panel plots only the signed total (no orbitals,
  ranking, sum check, log axis or peak clipping), with a zero line.
- **Fragments are chosen** from `DensityFragments::candidates` (same folder, and sibling
  folders one level down, with an MO property); hooks `dmdiff NAME...`, `dpdiff NAME...`
  (names as the candidates list gives them; none clears).
- **Fixtures.** `tests/builder/difference-fixtures` (H2 and its two H atoms, cc-pVTZ, same
  positions) from `make_difference_fixtures.py`; `densitydifference_test` checks the sign
  (gain between the nuclei, loss behind them), and that the integral of the difference is
  N(complex) - sum N(fragments) = 0.
