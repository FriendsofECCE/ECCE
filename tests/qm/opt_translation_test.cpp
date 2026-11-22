// A gradient with a net translation added (as the DFT grid leaves one) must
// not keep the optimiser from converging, and must not move the bond.
#include <cmath>
#include <cstdio>
#include "optimizer.hpp"

int main() {
  using namespace qm;
  const double re = 3.0, k = 0.4, drift = -3.05e-4;   // bohr, Eh/bohr^2, Eh/bohr
  Molecule m;
  m.atoms.resize(2);
  m.atoms[0].Z = 3;
  m.atoms[1].Z = 9;
  m.atoms[1].x[2] = re;
  auto bond = [](const Molecule& q) { return q.atoms[1].x[2] - q.atoms[0].x[2]; };
  auto eval = [&](const Molecule& q) {
    OptEval e;
    const double d = bond(q) - re;
    e.energy = 0.5 * k * d * d;
    e.grad.assign(6, 0.0);
    e.grad[2] = -k * d + drift;
    e.grad[5] = k * d + drift;
    return e;
  };
  OptOptions opt;
  const OptResult r = optimize_geometry(m, opt, eval, nullptr);
  const double dr = std::fabs(bond(r.mol) - re);
  std::printf("converged=%d evaluations=%d bond change=%.2e\n", r.converged, r.evaluations, dr);
  if (!r.converged || r.evaluations > 3 || dr > 1e-9) { std::printf("FAIL\n"); return 1; }
  std::printf("PASS\n");
  return 0;
}
