---
type: pitfall
title: "ECCE-QM analytic RHF Hessian: libcint derivative sign, ordered-pair double counting, scope"
area: codereg
section: "ECCE-QM engine"
paths: ["src/qm/hessian.cpp", "src/qm/main.cpp", "tests/qm/unit_test.cpp"]
issues: [394]
---
`rhf_hessian` is used for closed-shell HF frequency runs only (not DFT,
UHF/ROHF, Raman, or when `numeric_hessian true` / `ECCE_QM_NUMERIC_HESSIAN`
is set); everything else uses the finite-difference path.
Three silent traps, each found by comparing against a finite-difference oracle
piece by piece (nuclear, S, T, V, 2e skeleton separately):
* libcint's `ip` integrals are grad on the function; a derivative with respect
  to the nucleus is minus that (once per derivative; second derivatives of
  two grads keep the plus sign). Forgetting it in the derivative Fock matrix
  flips the CPHF right-hand side and gives Hessian errors of order 100.
* A sum over every ordered AO pair already counts (i,j) and (j,i) as separate
  terms of E, so after mapping the derivative on j onto the pair (j,i) the
  one-electron second derivatives carry a factor 2. The two-electron loop uses
  the 8-fold symmetry instead: 2 ipip1, 2 ipvip1, 4 ip1ip2.
* `W = D F D / 2` holds only for converged canonical orbitals; it is used so
  that W's derivative needs no orbital-energy derivative.
`rhf_hessian` keeps 3N full n x n matrices (Fock and overlap derivatives);
the in-core `EriStore` limit applies, and main falls back to finite
differences on `qm::Error`.
