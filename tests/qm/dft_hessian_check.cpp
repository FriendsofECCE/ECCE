// Analytic Hessian and dipole derivatives against central finite
// differences of the analytic gradient and dipole, for any method ecce-qm
// accepts.  Not a ctest (too slow for the full table); qm_unit covers a small
// case.  Usage:
//   qm_dft_hessian_check BASISDIR METHOD BASIS MOLECULE NRAD NANG [STEP] [PRUNE]
// MOLECULE: h2o nh3 c2h4 ch2o benzene (closed shell); oh ch3 nh2 h2o+ phenyl
// (doublets) and o2 (triplet), which take the unrestricted reference; METHOD
// uhf means hf unrestricted.  With STEP = 0 only the analytic run is timed.  Prints max |dH|, max |d freq|, max |d dmu| and |d IR| and timings.
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include "basis.hpp"
#include "gradient.hpp"
#include "hessian.hpp"
#include "scf.hpp"
#include "vibration.hpp"

using namespace qm;

int main(int argc, char** argv) {
  if (argc < 7) { std::cerr << "usage: qm_dft_hessian_check BASISDIR METHOD BASIS MOL NRAD NANG [STEP] [PRUNE]\n"; return 2; }
  BasisLibrary lib(argv[1]);
  const std::string method = argv[2], bas = argv[3], name = argv[4];
  const double step = argc > 7 ? std::atof(argv[7]) : 0.002;
  const bool prune = argc > 8 ? std::atoi(argv[8]) != 0 : true;
  // Slightly off-equilibrium geometries (bohr), so the gradient terms count.
  std::map<std::string, std::vector<std::array<double, 4>>> geo = {
      {"h2o", {{8, 0, 0, 0.2216}, {1, 0, 1.4309, -0.8867}, {1, 0, -1.37, -0.9}}},
      {"nh3", {{7, 0, 0, 0.12}, {1, 1.77, 0, -0.55}, {1, -0.9, 1.5, -0.5}, {1, -0.85, -1.55, -0.6}}},
      {"c2h4", {{6, 0, 0, 1.26}, {6, 0, 0.1, -1.27}, {1, 0, 1.75, 2.33}, {1, 0.1, -1.75, 2.35}, {1, 0, 1.72, -2.36}, {1, 0, -1.74, -2.33}}},
      {"ch2o", {{6, 0, 0, -1.0}, {8, 0, 0, 1.28}, {1, 0, 1.77, -2.1}, {1, 0.05, -1.75, -2.08}}},
      {"benzene", {}},
      {"oh", {{8, 0, 0, 0}, {1, 0, 0.1, 1.85}}},
      {"ch3", {{6, 0, 0, 0.05}, {1, 2.04, 0, -0.1}, {1, -1.0, 1.78, 0}, {1, -1.05, -1.75, 0.05}}},
      {"nh2", {{7, 0, 0, 0.25}, {1, 0, 1.55, -0.95}, {1, 0, -1.5, -1.0}}},
      {"o2", {{8, 0, 0, 1.15}, {8, 0, 0.05, -1.14}}},
      {"h2o+", {{8, 0, 0, 0.2}, {1, 0, 1.5, -0.85}, {1, 0, -1.45, -0.9}}},
      {"phenyl", {}}};
  for (int k = 0; k < 6; ++k) {
    const double a = k * M_PI / 3, rc = 2.6366, rh = 4.6834;
    geo["benzene"].push_back({6, rc * std::cos(a), rc * std::sin(a), 0});
    geo["benzene"].push_back({1, rh * std::cos(a), rh * std::sin(a), 0});
    geo["phenyl"].push_back({6, rc * std::cos(a), rc * std::sin(a), 0});
    if (k) geo["phenyl"].push_back({1, rh * std::cos(a), rh * std::sin(a), 0});
  }
  Molecule m;
  for (auto& r : geo.at(name)) { Atom at; at.Z = static_cast<int>(r[0]); for (int k = 0; k < 3; ++k) at.x[k] = r[k + 1]; m.atoms.push_back(at); }
  const std::map<std::string, std::pair<int, int>> open = {
      {"oh", {0, 2}}, {"ch3", {0, 2}}, {"nh2", {0, 2}}, {"o2", {0, 3}}, {"h2o+", {1, 2}}, {"phenyl", {0, 2}}};
  ScfOptions so;
  so.method = method == "uhf" ? "hf" : method;
  if (open.count(name)) {
    m.charge = open.at(name).first;
    m.multiplicity = open.at(name).second;
  }
  if (open.count(name) || method == "uhf") so.reference = Reference::Unrestricted;
  so.grid_radial = std::atoi(argv[5]);
  so.grid_angular = std::atoi(argv[6]);
  so.grid_prune = prune;
  so.conv_grad = 1e-9;
  so.conv_energy = 1e-12;
  so.max_iter = 300;
  Basis b0(m, lib, bas, 1);
  const ScfResult ref = run_scf(b0, m, so);
  if (!ref.converged) { std::cerr << "SCF did not converge\n"; return 1; }
  using clk = std::chrono::steady_clock;
  auto t0 = clk::now();
  const Derivatives ana = rhf_hessian(b0, m, so, ref);
  const double ta = std::chrono::duration<double>(clk::now() - t0).count();
  std::printf("%s %s %s grid %d/%d%s nbf %d <S2> %.4f: analytic %.1f s\n", name.c_str(), method.c_str(), bas.c_str(),
              so.grid_radial, so.grid_angular, prune ? "" : " unpruned", b0.nbf(), ref.s2, ta);
  if (step <= 0) return 0;
  auto point = [&](const Molecule& mm) {
    Basis b(mm, lib, bas, 1);
    ScfResult r = run_scf(b, mm, so, &ref);
    if (!r.converged) throw Error("displaced SCF did not converge");
    PointEval pe;
    pe.grad = scf_gradient(b, mm, so, r);
    for (int k = 0; k < 3; ++k) pe.dipole[k] = r.dipole[k];
    return pe;
  };
  t0 = clk::now();
  const Derivatives num = finite_difference_derivatives(m, step, point);
  const double tn = std::chrono::duration<double>(clk::now() - t0).count();
  const VibResult va = analyse_vibrations(m, ana), vn = analyse_vibrations(m, num);
  double df = 0, dir = 0, irmax = 0;
  for (size_t k = 0; k < va.freq.size() && k < vn.freq.size(); ++k) {
    df = std::max(df, std::fabs(va.freq[k] - vn.freq[k]));
    dir = std::max(dir, std::fabs(va.ir[k] - vn.ir[k]));
    irmax = std::max(irmax, vn.ir[k]);
  }
  std::printf("  max|dH| %.2e Eh/bohr2  max|dfreq| %.3f cm-1  max|ddmu| %.2e au  max|dIR| %.3f km/mol (largest %.1f)  fd %.1f s\n",
              (ana.hessian - num.hessian).cwiseAbs().maxCoeff(), df, (ana.dmu - num.dmu).cwiseAbs().maxCoeff(), dir, irmax, tn);
  std::printf("  freq:");
  for (double f : va.freq) std::printf(" %.1f", f);
  std::printf("\n");
  return 0;
}
