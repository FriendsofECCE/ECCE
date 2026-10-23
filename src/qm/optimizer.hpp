// Geometry optimisation in Cartesian coordinates: BFGS with a Lindh model
// Hessian as the starting guess, rational function steps in the space with
// translation and rotation removed, and a trust radius.  Constraints: the
// step is taken in the null space of the constraint gradients and the
// constraint values are then restored by Newton iterations.
#pragma once
#include <functional>
#include <string>
#include <vector>
#include "molecule.hpp"

namespace qm {

// A geometry constraint.  atoms are 0-based; value in bohr (bond) or radians
// (angle, torsion); has_value false keeps the value of the starting geometry.
struct GeoConstraint {
  enum Kind { Bond, Angle, Torsion, Fix } kind = Bond;
  int atoms[4] = {0, 0, 0, 0};
  bool has_value = false;
  double value = 0;
};

struct OptOptions {
  int max_steps = 100;           // energy+gradient evaluations
  // Convergence, all of them at once.  The step limits apply to the step
  // the optimiser would take next.  "normal" and "tight" are ORCA's
  // Opt and TightOpt thresholds (hartree, bohr).
  double max_grad = 3e-4, rms_grad = 1e-4;
  double max_step = 4e-3, rms_step = 2e-3;
  double d_energy = 5e-6;
  double trust = 0.3, trust_max = 1.0;  // bohr, length of the whole step
  int ts_hessian_every = 0;      // optimize_ts: recompute the Hessian every n steps (0: update only)
  // Held exactly at every step.  Dummy atoms that no constraint names stay
  // where they are; one that a bond, angle or torsion names moves with it.
  std::vector<GeoConstraint> constraints;
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

// Transition-state search: partitioned rational function steps (uphill along
// one Hessian mode, downhill along the rest) from the Hessian that hessian()
// computes at the start geometry, kept up to date by Bofill updates.  The
// followed mode starts as the lowest and afterwards is the one that overlaps
// most with the previous step's.  Convergence as in optimize_geometry; no
// constraints.  hessian(mol) is called after eval(mol) at the same geometry,
// returns 3N x 3N in hartree/bohr^2.
OptResult optimize_ts(const Molecule& start, const OptOptions& opt,
                      const std::function<OptEval(const Molecule&)>& eval,
                      const std::function<Mat(const Molecule&)>& hessian,
                      const std::function<void(const OptStep&)>& on_step);

// Lindh's model Hessian (stretches, bends, torsions), in Cartesians, bohr
// and hartree.  Exposed for tests.
Mat lindh_hessian(const Molecule& mol);

}  // namespace qm
