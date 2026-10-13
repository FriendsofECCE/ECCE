#include "polar.hpp"
#include <algorithm>
#include <cmath>

namespace qm {

Mat static_polarizability(Basis& basis, const Molecule& mol, const ScfOptions& opt, const ScfResult& ref,
                          double step) {
  ScfOptions so = opt;
  so.conv_grad = std::min(so.conv_grad, 1e-8);
  so.conv_energy = std::min(so.conv_energy, 1e-11);
  so.max_iter = std::max(so.max_iter, 200);
  so.on_cycle = nullptr;
  Mat a(3, 3);
  for (int j = 0; j < 3; ++j) {
    double mu[2][3];
    for (int s = 0; s < 2; ++s) {
      for (int k = 0; k < 3; ++k) so.field[k] = 0.0;
      so.field[j] = (s == 0 ? step : -step);
      const ScfResult r = run_scf(basis, mol, so, &ref);
      if (!r.converged) throw Error("SCF did not converge in the electric field");
      for (int i = 0; i < 3; ++i) mu[s][i] = r.dipole[i];
    }
    for (int i = 0; i < 3; ++i) a(i, j) = (mu[0][i] - mu[1][i]) / (2 * step);
  }
  return 0.5 * (a + a.transpose());
}

PolarInvariants polarizability_invariants(const Mat& a) {
  PolarInvariants p;
  p.iso = (a(0, 0) + a(1, 1) + a(2, 2)) / 3.0;
  p.aniso = std::sqrt(0.5 * (std::pow(a(0, 0) - a(1, 1), 2) + std::pow(a(1, 1) - a(2, 2), 2) +
                             std::pow(a(2, 2) - a(0, 0), 2)) +
                      3.0 * (a(0, 1) * a(0, 1) + a(0, 2) * a(0, 2) + a(1, 2) * a(1, 2)));
  return p;
}

}  // namespace qm
