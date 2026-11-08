---
type: rule
title: "MOPAC gets the level diagram only, no composition (#911)"
area: mo-diagram
paths: [src/apps/builder/MoDiagramPanel.C]
issues: [911]
---
`isRelevant()` offers the panel for MOPAC, but `noCompositionFor()` switches
off the fragment columns, the model diagram and the tooltip composition: the
orthonormal NDDO coefficients have nothing to project on, and the Slater
fallback produced columns with norms of 0.6 to 1.5. Levels use the stored
ORBSYM labels (`computeFullGroupLabels` fails cleanly on the empty basis
config) and ORBOCC. The note about it is the panel's own status line.
Test: `tests/apps/modiagram_mopac_test.py`.

MOPAC SCF leaves a degenerate pair up to ~1e-4 Hartree apart (NH3 1e: 9e-5), so its levels are grouped with a 1e-3 Hartree tolerance instead of 1.5e-5; with the tight value the pair drew as "1e" + "2e" and the virtual pair was misnumbered "3e".
