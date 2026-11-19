// Rigid forces must not prevent convergence or hide an internal force.
#include <algorithm>
#include <cmath>
#include <iostream>
#include "gradient.hpp"
#include "optimizer.hpp"
#include "scf.hpp"

using namespace qm;

namespace {
int failures = 0;
void check(bool ok, const char* what) {
  std::cout << (ok ? "PASS " : "FAIL ") << what << std::endl;
  if (!ok) ++failures;
}

double bond(const Molecule& m) {
  double r2 = 0;
  for (int k = 0; k < 3; ++k)
    r2 += std::pow(m.atoms[1].x[k] - m.atoms[0].x[k], 2);
  return std::sqrt(r2);
}

void model_tests() {
  Molecule lif;
  lif.atoms = {{3, {2, -1, 4}}, {9, {3, 1, 6}}};
  const double r0 = bond(lif);
  auto harmonic = [=](const Molecule& m) {
    const double r = bond(m), dr = r - r0;
    OptEval e;
    e.energy = 0.1 * dr * dr;
    e.grad.resize(6);
    for (int k = 0; k < 3; ++k) {
      const double g = 0.2 * dr * (m.atoms[1].x[k] - m.atoms[0].x[k]) / r;
      e.grad[k] = -g - 3.05e-4;
      e.grad[3 + k] = g - 3.05e-4;
    }
    return e;
  };
  OptOptions opt;
  opt.max_steps = 12;
  OptStep last;
  auto record = [&](const OptStep& st) { last = st; };
  const auto at_minimum = optimize_geometry(lif, opt, harmonic, record);
  check(at_minimum.converged && at_minimum.evaluations == 1 &&
        std::fabs(bond(at_minimum.mol) - r0) < 1e-12 && last.max_grad < 1e-12,
        "tilted LiF model: uniform force converges without moving");
  check(last.grad == harmonic(lif).grad && at_minimum.grad == last.grad,
        "full Cartesian gradient retained");

  Molecule displaced = lif;
  for (int k = 0; k < 3; ++k)
    displaced.atoms[1].x[k] += 0.1 * (lif.atoms[1].x[k] - lif.atoms[0].x[k]) / r0;
  const auto relaxed = optimize_geometry(displaced, opt, harmonic, record);
  check(relaxed.converged && relaxed.evaluations <= 8 &&
        std::fabs(bond(relaxed.mol) - r0) < 5e-4,
        "LiF model: internal stretch relaxes with uniform force");
  opt.max_steps = 1;
  check(!optimize_geometry(displaced, opt, harmonic, record).converged,
        "internal stretch force still prevents convergence");

  Molecule water;
  water.atoms = {{8, {2, -1, 4}}, {1, {3, 1, 4}}, {1, {1, 0, 6}}};
  const Eigen::Vector3d omega(0.002, -0.003, 0.004);
  Eigen::Vector3d center = Eigen::Vector3d::Zero();
  for (const auto& a : water.atoms) center += Eigen::Map<const Eigen::Vector3d>(a.x) / 3;
  auto rigid = [&](const Molecule& m) {
    OptEval e;
    for (const auto& a : m.atoms) {
      const Eigen::Vector3d g = omega.cross(Eigen::Map<const Eigen::Vector3d>(a.x) - center)
                               + Eigen::Vector3d(0.001, -0.002, 0.003);
      for (int k = 0; k < 3; ++k) e.grad.push_back(g(k));
    }
    return e;
  };
  check(optimize_geometry(water, opt, rigid, record).converged && last.max_grad < 1e-12,
        "nonlinear molecule: translation and rotation removed");
  auto hessian = [](const Molecule& m) -> Mat {
    Mat H = Mat::Identity(3 * m.atoms.size(), 3 * m.atoms.size());
    H(0, 0) = -10;
    return H;
  };
  check(optimize_ts(water, opt, rigid, hessian, record).converged && last.max_grad < 1e-12,
        "TS force criteria remove translation and rotation");

  GeoConstraint fixed;
  fixed.kind = GeoConstraint::Fix;
  fixed.atoms[0] = 0;
  opt.constraints.push_back(fixed);
  check(!optimize_geometry(lif, opt, harmonic, record).converged,
        "fixed atom anchors molecule: uniform force remains significant");
}

void lif_test(const std::string& basis_dir) {
  Molecule lif;
  lif.atoms = {{3, {0, 0, 0}}, {9, {0, 0, 1.587 / kBohrToAngstrom}}};
  BasisLibrary lib(basis_dir);
  ScfOptions scf;
  scf.method = "b3lyp";
  ScfResult previous;
  bool have_previous = false;
  auto evaluate = [&](const Molecule& m) {
    Basis b(m, lib, "aug-cc-pVDZ", true);
    const ScfResult r = run_scf(b, m, scf, have_previous ? &previous : nullptr);
    if (!r.converged) throw Error("LiF SCF did not converge");
    previous = r;
    have_previous = true;
    return OptEval{r.energy, scf_gradient(b, m, scf, r)};
  };
  OptOptions opt;
  opt.max_steps = 12;
  OptStep last;
  const auto result = optimize_geometry(lif, opt, evaluate, [&](const OptStep& st) { last = st; });
  const double r = bond(result.mol) * kBohrToAngstrom;
  const double net = 0.5 * (result.grad[2] + result.grad[5]);
  std::cout << "LiF B3LYP/aug-cc-pVDZ: evaluations=" << result.evaluations
            << " bond=" << r << " A mean_z_gradient=" << net
            << " max_gradient=" << last.max_grad << " hartree/bohr" << std::endl;
  check(result.converged && result.evaluations <= 8, "LiF converges in few evaluations");
  check(std::fabs(r - 1.587) < 1e-3, "LiF retains 1.587 A bond");
  check(std::fabs(net) > opt.rms_grad, "LiF retains a net force above convergence threshold");
  check(std::fabs(last.max_grad - 0.5 * std::fabs(result.grad[5] - result.grad[2])) < 1e-10,
        "LiF criteria measure the bond gradient");
}
}  // namespace

int main(int argc, char** argv) {
  try {
    model_tests();
    lif_test(argc > 1 ? argv[1] : "");
  } catch (const std::exception& e) {
    std::cerr << "FAIL " << e.what() << std::endl;
    return 1;
  }
  return failures ? 1 : 0;
}
