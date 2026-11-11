---
type: pitfall
title: "ECCE-QM analytic Hessian in CPCM (and D3): which cavity terms, how it is assembled, what it agrees with"
area: codereg
section: "ECCE-QM engine"
paths: ["src/qm/hessian.cpp", "src/qm/solvent.cpp", "src/qm/solvent.hpp", "tests/qm/unit_test.cpp", "tests/qm/dft_hessian_check.cpp"]
issues: [394]
---
`rhf_hessian` takes a CPCM solvent (RHF/RKS, UHF/UKS) and D3(BJ);
`analytic_hessian_available` refuses only a field, IVOs, ROHF/ROKS and
functionals without an analytic kernel. It is the derivative of the analytic
CPCM gradient (ecceqm-cpcm-solvent.md), so it carries the same cavity model:
* **Cavity response included**: points ride rigidly with their atom
  (operator-centre derivatives by translation invariance, as for nuclear
  attraction), and the switching function F_k of every point depends on every
  other atom (first and second derivatives of the erf-step product,
  `switch_derivatives`). Sphere radii and Gaussian exponents are fixed, so
  tessera areas do not change in this model; only exposure does. Points
  dropped at F < 1e-8 are the one discontinuity, as in the gradient.
* Fixed-density part, E_pol = q.V/2 with A q = -f V:
  q.V'' + q.A''.q/(2f) - u_x.A^-1.u_y/f, u_x = f V'_x + A'_x q.
  `Cpcm::add_hessian` holds the nuclear V'' and A''; the electronic V'' is
  `add_second` with the q-weighted point operators grouped per atom;
  `Cpcm::first_derivatives` gives the nuclear part of u, the electronic V'
  is added in hessian.cpp from the same `iprinv` pass.
* Derivative Fock matrix gains -sum_k (q'_k L_k + q_k L'_k) with
  q' = -A^-1 u (same for both spins). CPHF/CPKS gains the reaction field of
  the total first-order density (`CpcmField::response`, batched through the
  cached point-integral table as two GEMMs).
* `Cpcm::solve` is A^-1 = diag(F) M^-1, reusing the SCF's LU of M.
* D3(BJ) is **not** analytic: its Hessian block is central differences
  (1e-4 bohr) of `d3bj_energy`'s analytic gradient, cheap because no SCF is
  involved. The analytic C6(CN) chain rule was not worth it.
* Reference numbers (water, eps 78.3553, 6-31G*, 99/590, step 0.002 bohr,
  FD of the analytic CPCM gradient): table on #394. `qm_dft_hessian_check
  ... STEP PRUNE EPS D3` reproduces it; `glycine` is the timing case.
