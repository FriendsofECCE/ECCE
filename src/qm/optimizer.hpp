// Geometry optimisation in Cartesian coordinates: BFGS with a Lindh model
// Hessian as the starting guess, rational function steps in the space with
// translation and rotation removed, and a trust radius.
#pragma once
#include <functional>
#include <string>
#include <vector>
#include "molecule.hpp"

namespace qm {

struct OptOptions {
  int max_steps = 100;           // energy+gradient evaluations
  // Convergence, all of them at once.  The step limits apply to the step
  // the optimiser would take next.  "normal" and "tight" are ORCA's
  // Opt and TightOpt thresholds (hartree, bohr).
  double max_grad = 3e-4, rms_grad = 1e-4;
  double max_step = 4e-3, rms_step = 2e-3;
  double d_energy = 5e-6;
  double trust = 0.3, trust_max = 1.0;  // bohr, length of the whole step
  void set_convergence(const std::string& name);  // "normal" or "tight"; throws
};

struct OptEval {
  double energy = 0;
  std::vector<double> grad;      // hartree/bohr, 3 per atom
};

struct OptStep {
  int index = 0;                 // 1-based evaluation number
  Molecule mol;                  // geometry the energy and gradient belong to
  double energy = 0;
  std::vector<double> grad;
  double max_grad = 0, rms_grad = 0;
  double max_step = 0, rms_step = 0;  // of the step proposed from here; 0 if converged
  double d_energy = 0;           // against the previous accepted step
};

struct OptResult {
  bool converged = false;
  int evaluations = 0;
  Molecule mol;                  // final geometry (that of the last evaluation kept)
  std::vector<double> grad;
  double energy = 0;
};

// eval is called at each trial geometry; on_step is called for every
// accepted step, including the last, with the criteria it was judged by.
OptResult optimize_geometry(const Molecule& start, const OptOptions& opt,
                            const std::function<OptEval(const Molecule&)>& eval,
                            const std::function<void(const OptStep&)>& on_step);

// Lindh's model Hessian (stretches, bends, torsions), in Cartesians, bohr
// and hartree.  Exposed for tests.
Mat lindh_hessian(const Molecule& mol);

}  // namespace qm
