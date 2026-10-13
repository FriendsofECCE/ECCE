// Static dipole polarisability by finite electric field.
#pragma once
#include "basis.hpp"
#include "common.hpp"
#include "molecule.hpp"
#include "scf.hpp"

namespace qm {

constexpr double kFieldStep = 0.001;  // a.u.

// alpha_ij = [mu_i(+F e_j) - mu_i(-F e_j)] / 2F in bohr^3, symmetrised.  Six
// SCFs started from ref (the field-free result for this geometry) and
// converged tighter than an energy needs, so the noise stays far below the
// field-step error.  Throws when one of them does not converge.
Mat static_polarizability(Basis& basis, const Molecule& mol, const ScfOptions& opt, const ScfResult& ref,
                          double step = kFieldStep);

struct PolarInvariants {
  double iso = 0;    // (axx+ayy+azz)/3
  double aniso = 0;  // gamma, the anisotropy
};
PolarInvariants polarizability_invariants(const Mat& alpha);

}  // namespace qm
