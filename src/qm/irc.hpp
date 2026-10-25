// Intrinsic reaction coordinate by the Gonzalez-Schlegel second-order method:
// in mass-weighted Cartesians, each new point is the minimum of the energy on
// a hypersphere of radius step/2 around a pivot half a step downhill from the
// previous point.  The first step leaves the transition state along its
// imaginary mode.
#pragma once
#include <functional>
#include <vector>
#include "molecule.hpp"
#include "optimizer.hpp"

namespace qm {

struct IrcOptions {
  double step = 0.1;       // bohr*sqrt(amu), between points
  int max_points = 20;     // per direction
  int max_micro = 25;      // constrained-optimisation iterations per point
  double rms_grad = 5e-4, max_grad = 2e-3;  // the path ends when the gradient is below both
  double tol_rms = 1e-4, tol_max = 3e-4;    // constrained optimisation: gradient across the sphere
  int min_points = 5;      // no gradient or energy stop before this many points
  int hessian_every = 0;   // recompute the Hessian every n points (0: Bofill updates only)
};

struct IrcPoint {
  Molecule mol;
  double energy = 0;
  double s = 0;            // signed path length from the TS, bohr*sqrt(amu)
  double max_grad = 0, rms_grad = 0;   // Cartesian, hartree/bohr
};

// One direction (sign +1 or -1 along the imaginary mode of H, the Cartesian
// Hessian at ts).  The TS itself is not part of the result.  on_point is
// called with every accepted point; hessian(mol) is only used when
// hessian_every > 0.  why (if given) receives the reason the path ended.
// Throws Error when H has no imaginary mode.
std::vector<IrcPoint> irc_path(const Molecule& ts, const Mat& H, int sign, const IrcOptions& opt,
                               const std::function<OptEval(const Molecule&)>& eval,
                               const std::function<Mat(const Molecule&)>& hessian,
                               const std::function<void(const IrcPoint&)>& on_point, std::string* why = nullptr);

}  // namespace qm
