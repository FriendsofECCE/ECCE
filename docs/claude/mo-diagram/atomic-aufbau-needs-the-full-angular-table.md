---
type: pitfall
title: "Atomic Aufbau needs the basis's polarization shells, even when they are empty"
area: mo-diagram
paths: ["data/client/cap/ECCE-QM.edml", "src/tdat/chemistry/BasisAngularNorm.C", "src/tdat/chemistry/RadialDist.C", "src/apps/builder/MoDiagramPanel.C", "tests/apps/modiagram_aufbau_test.py"]
issues: [817]
---
`RadialDist::atomOrbitalLabels` builds `DensityPlane::Evaluator` before
`shellLabels` analyses the angular shares. The evaluator needs every basis
function, including empty polarization shells. ECCE-QM's spherical
`MOOrdering` ended at f, but the stored Cr and Cu examples use
aug-cc-pVTZ-NR with g functions. `BasisFlatten` left those functions empty;
`MoAoBasis::flatBasis` refused them and the evaluator failed before any
share threshold, shell counting or Aufbau d handling ran.

The angular table now covers s through i, in libcint's order and signs.
`BasisAngularNorm` normalizes g/h/i by integrating their squared polynomial;
the old constant 1 gave non-unit virtual orbitals even with the full table.
`MoDiagramPanel::aufbauFailure` carries the evaluator/basis or model reason
to the scene command instead of calling every failure a non-atomic
calculation. The raw Cr/Cu example outputs describe def2-TZVPD, while their
stored copies describe aug-cc-pVTZ-NR; inspect the stored calculation when
debugging the panel.

The app test includes optional Ca, Cr and Cu cases, skipped when their
example directories are absent. These examples are maintained separately.
