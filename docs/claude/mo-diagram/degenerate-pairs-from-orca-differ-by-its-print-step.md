---
type: pitfall
title: "ORCA prints orbital energies to 1e-5, so a degenerate set is not 1e-5 tight (#301)"
area: mo-diagram
paths: [src/tdat/chemistry/SymmetryAnalysis.C, src/apps/builder/MoDiagramPanel.C, scripts/parsers/orca.mo]
issues: [301]
---
`orca.mo` derives ORBENG from the MOLECULAR ORBITALS block, which carries five
decimals. Two orbitals of one e set can therefore differ by exactly 1e-5 from
rounding alone (BF3's e', benzene's e sets), and a strict `< 1.0e-5` grouping
split them: the single components cannot be labelled, so every ORCA run
without UseSym drew BF3 and benzene with no labels at all (and no `pi system
only`). The grouping tolerance in `SymmetryAnalysis::fullLabelSpectrum` and
`MoDiagram::groupByIrrep` is 1.5e-5, above the print step; Cr(CO)6's t1u and
eg (4e-4 apart) are still told apart.
