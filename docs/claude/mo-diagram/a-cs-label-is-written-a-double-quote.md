---
type: pitfall
title: "Compare irrep labels with canonicalIrrep(), never toupper(): codes write A\" for A''"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["src/apps/builder/MoDiagramPanel.C", "src/tdat/chemistry/SymmetryAnalysis.C"]
issues: [132]
---
**Compare irrep labels with `MoDiagram::canonicalIrrep()`, never
`toupper()`.** ORCA and Gaussian print a double prime as `A"`, the
character tables as `A''`. The panel's subgroup cross-check upper-cased
both sides and so counted every `A''` orbital as a disagreement; its
all-or-nothing rule then refused the full-group labels for every
Cs-labelled run. ORCA labels NH3 (C3v) in Cs, so a hand-built NH3 with
"Use symmetry" on drew no fragment columns at all, with the misleading
remedy "Use Find Symmetry". `ECCE_DEBUG_MOSYM_LOG` now names each
disagreeing orbital.

The geometry the diagram analyses for a finished calculation is the
code's own symmetrised one (last GEOMTRACE step), not the Builder's
input, so an unsymmetrised input is not by itself a reason for a lower
group. Real-run fixtures: `tests/modiagram/fixtures/orca-nh3-usesym`
and siblings, made by `tests/modiagram/makecalc.py`.
