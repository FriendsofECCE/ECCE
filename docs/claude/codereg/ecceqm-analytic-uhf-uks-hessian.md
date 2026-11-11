---
type: pitfall
title: "ECCE-QM analytic UHF/UKS Hessian: spin channels, polarised fxc, why ROHF/ROKS stay numerical"
area: codereg
section: "ECCE-QM engine"
paths: ["src/qm/hessian.cpp", "src/qm/xc.cpp", "src/qm/main.cpp", "tests/qm/unit_test.cpp", "tests/qm/dft_hessian_check.cpp"]
issues: [394]
---
`rhf_hessian` handles an unrestricted reference (UHF, UKS with LDA, GGA,
global hybrids) with the same code as the closed shell, written over spin
channels: one channel (the alpha orbitals, weight 2) for a closed shell, alpha
and beta (weight 1) for UHF/UKS. Every per-spin piece (D, F, W = D F D, the
exchange part of the derivative Fock matrices, the CPHF/CPKS unknowns) is one
spin's; totals are sum weight x channel. Keep it that way: a closed-shell
special case written separately would drift.
* CPHF/CPKS: alpha and beta blocks of one perturbation are one vector in the
  conjugate gradients (one alpha, one rz, convergence over both). The
  operator is symmetric in the plain sum inner product, so CG applies.
  G_s(D1) = J(D1_a + D1_b) - exx K(D1_s) + fxc_s(D1_a, D1_b).
* XC (`XcIntegrator::hessian_terms`, `kernel`) take a list of densities:
  {total} for a closed shell (libxc unpolarised), {D_a, D_b} for UKS (libxc
  polarised). `xc_first`/`xc_respond` hold the chain rule from libxc's
  (rho_s, sigma_aa, sigma_ab, sigma_bb) derivatives to df/drho_s and
  Gamma_s = df/d(grad rho_s); libxc layout: v2rho2 (aa, ab, bb),
  v2rhosigma at 3s+k, v2sigma2 upper triangle (aa.aa aa.ab aa.bb ab.ab ab.bb
  bb.bb).
* ROHF/ROKS keep finite differences: their orbitals diagonalise a composite
  Fock operator, not one Fock matrix per spin, so W = D F D and the per-spin
  canonical CPHF denominators do not hold; it needs its own response theory.
* As for RKS, the grid is held fixed and the finite-difference oracle moves it;
  OH PBE/6-31G* is the worst case at the default 99/590 (max |dH| 9.5e-5,
  1.2e-5 at 150/974), frequencies still within 0.02 cm-1. Judge on a fine grid.
* Reference numbers (99/590 pruned, step 0.002 bohr; OH, CH3, NH2, O2
  triplet, H2O+ x UHF/SVWN/PBE/B3LYP x 6-31G*/def2-SVP): max |dH| 8e-7 to
  9.5e-5, frequencies within 0.12 cm-1, dipole derivatives within 3.5e-5 au,
  IR within 0.002 km/mol. Phenyl radical UB3LYP/6-31G* frequency run
  (`ecce-qm`, 4 threads on a 2-core box): 353 s analytic, 732 s finite
  differences; frequencies and IR within 0.08.
* The TS search takes the analytic Hessian at every step wherever
  `analytic_hessian_available` holds (main.cpp, `ts_hessian_every` default).
  It is decided before any SCF has run, so it is judged from the input's
  reference and the molecule's electron counts, not from a ScfResult (an
  empty one has nalpha 0 and silently turned the default off).
