---
type: pitfall
title: "ORCA's built-in cc-pVXZ is segmented; the basis ECCE stores is the general contraction, so the MO coefficients do not fit it"
area: codereg
paths: ["scripts/parsers/wrORCAGBS.pm", "scripts/parsers/ORCA.expt", "scripts/parsers/orca.basispattern", "src/dsm/edsiimpl/ICalcUtils.C", "src/viz/propsgcommands/ComputeMoCmd.C", "src/apps/builder/MoDiagramPanel.C", "tests/basis/code_contraction_test.py", "src/tdat/chemistry/MoAoBasis.C", "src/tdat/chemistry/ResultsFromCalc.C", "data/admin/basissets/cc-pVTZ.BAS"]
issues: [942]
---
`wrORCAGBS.pm` writes a library basis by name (`NewGTO H "cc-pVTZ" end`) and
`ORCA.expt` imports one by name, so ORCA uses its own copy of the basis while
the calculation stores ECCE's (`data/admin/basissets`). For cc-pVXZ the two
differ: ECCE's is the general contraction (H cc-pVTZ 1s over 5 primitives,
C 1s/2s over 10), ORCA's drops from each contraction every primitive that is
also an uncontracted function of the same l (H 1s over 3, C 1s/2s over 8;
coefficients otherwise the same, renormalised). The function count and the
span agree, so nothing fails loudly, but ORCA's MO coefficients belong to
other functions: results.h5 fails its check (H2 cc-pVTZ scan examples:
tr(PS) 2.45 for N = 2) and every view built from `flatBasis` (Density Map,
Density Profile, MO composition) is off. def2 and Pople bases are segmented
in both and pass.

How to see it: the output always prints, per element,
`5s2p1d contracted to 3s2p1d pattern {311/11/1}` (ECCE's would be {511});
`Print[P_Basis] 2` prints ORCA's full basis ("BASIS SET IN INPUT FORMAT").
Zeroing the duplicated primitives' coefficients in a stored
`BasisSet.ecce_basisset` makes `ecce-results write` pass (2.000005).
An element written explicitly (one with an ECP) keeps ECCE's general form,
so a blanket "ORCA segments" rule in the flattener would be wrong for it.

**What reads it now.** `orca.basispattern` stores the printed patterns as
BASISPATTERN (PropString, "C 8811/311/11/1|H 311/11/1").
`ICalcUtils::applyCodeContraction(calc, config)` zeroes, per element, the
duplicated primitives' coefficients in a fresh `gbsConfig()` copy, but only
when the stored contraction differs from the pattern and dropping them gives
it exactly (same order); otherwise the basis is left and the checks report
it. It is called wherever primitives are flattened for orbitals:
`MoAoBasis::flatBasis` (results.h5, Density Map/Profile, MO composition,
`ecce-density-field`), `ComputeMoCmd::execute` (isosurfaces, also for the
as-stored fallback) and the MO diagram's symmetry flatten in
`MoDiagramPanel.C`. Readers that only count or label functions (MoCoeffs,
Mulliken, MoAoOrder) do not need it. A calculation parsed before
BASISPATTERN existed has none: re-parse it. Not every cc set differs:
ECCE's cc-pVDZ.BAS is already segmented, cc-pVTZ.BAS is general.
`tests/basis/code_contraction_test.py` (ctest `gbscodecontraction`) compares
the result with ORCA's own `Print[P_Basis] 2` for H and C.
