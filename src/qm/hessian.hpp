// Analytic second derivatives of the closed-shell RHF energy (coupled-perturbed
// HF), and the dipole derivatives that go with them.  Open-shell references and
// Kohn-Sham functionals use the finite-difference path in vibration.hpp.
#pragma once
#include "basis.hpp"
#include "molecule.hpp"
#include "scf.hpp"
#include "vibration.hpp"

namespace qm {

// True when rhf_hessian applies: method hf, restricted closed shell.
bool analytic_hessian_available(const ScfOptions& opt, const ScfResult& r);

// Hessian (hartree/bohr^2, 3N x 3N) and dipole derivatives (a.u., 3 x 3N) at
// the geometry r was converged for.  Needs the two-electron integrals in
// core (throws qm::Error when they do not fit).  dalpha is left empty.
Derivatives rhf_hessian(Basis& basis, const Molecule& mol, const ScfOptions& opt, const ScfResult& r);

}  // namespace qm
