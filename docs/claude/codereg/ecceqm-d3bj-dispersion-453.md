---
type: map
title: "ECCE-QM D3(BJ): C++ code, upstream data converted at build time, where it is wired"
area: codereg
section: "ECCE-QM"
paths: ["src/qm/d3.cpp", "src/qm/d3.hpp", "src/qm/tools/d3_data_gen.cpp", "third_party/simple-dftd3-data", "src/qm/scf.cpp", "src/qm/gradient.cpp", "src/qm/hessian.cpp", "scripts/codereg/ecceqmtheory.py", "scripts/codereg/ecceqmtheory.dlg", "scripts/parsers/ai.ecceqm", "scripts/parsers/ecceqm.desc", "tests/qm/d3_test.cpp", "tests/qm/gen_d3bj_fixture.py"]
issues: [453]
---
D3(BJ) is ECCE's own C++ (`src/qm/d3.cpp`); no Fortran. The reference data
(C6 table, reference CNs, r4/r2, covalent radii) are upstream simple-dftd3 /
mctc-lib files kept byte-identical in `third_party/simple-dftd3-data`;
`qm_d3_data_gen` (`src/qm/tools/d3_data_gen.cpp`) pulls the array literals
out at build time into `d3_data.inc` in the build tree and fails the build if
an array length changes. Do not edit the bundled files; update them as a
whole (README there).

* Input keyword `dispersion d3bj` sets `ScfOptions::dispersion`. The energy
  is fixed per geometry and is added inside the SCF energy like E_nuc
  (`ScfResult::e_disp`), so every path that uses the SCF energy and
  `scf_gradient()` (optimiser, scan, TS, IRC, finite-difference Hessian)
  includes it with no further change. `analytic_hessian_available()` is false
  with dispersion: the RHF analytic Hessian has no D3 term.
* Parameters (s6, s8, a1, a2) exist for hf, pbe, b3lyp, pbe0; none for svwn,
  which the engine refuses and the dialog greys out (`.py` CheckDependency,
  `.dlg` `enable_if`). Keep the two dialogs in step (`tests/dialogs`).
* No distance cut-offs (CN or pair): a molecule is finite and the energy stays
  smooth; simple-dftd3/ORCA cut at 40/60 bohr, which only matters for very
  large molecules. No three-body (ATM) term, as ORCA's D3BJ default.
* Output: `dispersion D3(BJ)` and `energy_dispersion` (already in
  `energy_total`) -> `EDISP` (the key QE already uses, "Components" group).
  Summary: a toggleSet `TheorySummary` in `ECCE-QM.edml`.
* Oracle: ORCA 6.1.1 `! <method> D3BJ def2-SVP EnGrad` minus the same run
  without D3BJ; `tests/qm/gen_d3bj_fixture.py` makes `tests/qm/d3bj_orca.txt`
  from the trimmed outputs in `tests/qm/d3bj_orca/`; `ctest -R qm_d3` also
  checks the analytic gradient against finite differences.
