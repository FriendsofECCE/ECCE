---
type: pitfall
title: "ECCE-QM analytic RKS Hessian: fixed grid, what it agrees with, which functionals"
area: codereg
section: "ECCE-QM engine"
paths: ["src/qm/hessian.cpp", "src/qm/xc.cpp", "src/qm/ao.cpp", "tests/qm/unit_test.cpp", "tests/qm/dft_hessian_check.cpp"]
issues: [394]
---
`rhf_hessian` also does closed-shell Kohn-Sham (LDA, GGA, global hybrids:
svwn, pbe, b3lyp, pbe0). `xc_analytic_hessian_supported` decides from libxc
(family LDA/GGA/HYB_GGA, fxc present, `cam_omega == 0`), so a meta-GGA or a
range-separated hybrid added to `functional_by_name` later stays on finite
differences without further code. UKS, CPCM, D3, a field and IVO virtuals
(non-canonical, the CPKS denominators assume canonical) also stay on finite
differences.
* The exact-exchange part is the RHF code with every exchange term scaled by
  `exx`; the XC part is `XcIntegrator::hessian_terms` (skeleton second
  derivatives and dVxc/dx at fixed density) and `XcIntegrator::kernel`
  (fxc response, called inside every CPKS iteration). GGAs need third AO
  derivatives (`AoValues::d3`, `ao_d3_index`).
* libxc must be built with fxc: the vendored build sets `DISABLE_FXC OFF`;
  Debian's libxc-dev has it.
* No grid-weight derivatives (the gradient has none either). The finite-
  difference oracle moves the grid with the atoms, the analytic Hessian holds
  it fixed, so their difference is the grid response: H2O/PBE/6-31G* max |dH|
  3.3e-3 at 50/194, 6e-5 at 75/302, 2.8e-6 at the default 99/590 (frequencies
  0.18, 0.07, 0.008 cm-1). Do not judge the code on a coarse grid.
* Benchmark program: `qm_dft_hessian_check` (EXCLUDE_FROM_ALL; `ninja
  qm_dft_hessian_check`), arguments in its header.
* Reference numbers (99/590 pruned, step 0.002 bohr, H2O/NH3/C2H4/CH2O x
  SVWN/PBE/B3LYP x 6-31G*/def2-SVP): max |dH| 1.7e-6 to 7.2e-5 (largest for
  CH2O), frequencies within 0.06 cm-1, dipole derivatives within 1.7e-5 au.
  Benzene B3LYP/6-31G* frequency run on 4 threads: 191 s analytic, 594 s
  finite differences.
