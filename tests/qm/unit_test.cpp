// qm_unit: checks of the pieces that have an independent answer.
//  * Lebedev rules integrate monomials exactly (analytic values);
//  * AO values on the grid, integrated pairwise, reproduce libcint's overlap
//    matrix (two independent routes to <i|j>: quadrature and libcint);
//  * AO gradients agree with finite differences of the AO values.
#include <cmath>
#include <iostream>
#include "ao.hpp"
#include "basis.hpp"
#include "grid.hpp"
#include "integrals.hpp"

using namespace qm;

static int fails = 0;
static void check(bool ok, const std::string& what, double val = 0) {
  std::cout << (ok ? "ok    " : "FAIL  ") << what << "  " << val << "\n";
  if (!ok) ++fails;
}

int main(int argc, char** argv) {
  std::string bdir = argc > 1 ? argv[1] : "";
  BasisLibrary lib(bdir);

  // Lebedev: integral of x^2 y^2 z^2 over the unit sphere / 4pi = 1/105 (needs degree 6).
  {
    Molecule m;
    Atom a; a.Z = 1;
    m.atoms.push_back(a);
    for (int n : lebedev_sizes()) {
      if (n < 26) continue;  // degree >= 7
      Grid g = make_grid(m, 1, n);  // one radial shell: weights = 4 pi r^2 w_r * w_ang
      // recover the angular weights: normalise by their sum
      double s = 0, v = 0;
      for (size_t i = 0; i < g.size(); ++i) {
        double r = std::sqrt(g.x[i] * g.x[i] + g.y[i] * g.y[i] + g.z[i] * g.z[i]);
        double x = g.x[i] / r, y = g.y[i] / r, z = g.z[i] / r;
        s += g.w[i];
        v += g.w[i] * x * x * y * y * z * z;
      }
      check(std::fabs(v / s - 1.0 / 105.0) < 1e-14, "lebedev " + std::to_string(n) + " x2y2z2", v / s - 1.0 / 105.0);
    }
  }

  // AO overlap by quadrature vs libcint, and normalisation, on water.
  for (const char* bname : {"def2-SVP", "6-31G*", "STO-3G"}) {
    for (int sph : {1, 0}) {
      Molecule m;
      m.atoms = {{8, {0, 0, 0.2216}}, {1, {0, 1.4309, -0.8867}}, {1, {0, -1.4309, -0.8867}}};
      Basis b(m, lib, bname, sph);
      OneElectron oe = one_electron(b);
      Grid g = make_grid(m, 120, 590);
      std::vector<double> xyz(3 * g.size());
      for (size_t i = 0; i < g.size(); ++i) { xyz[3 * i] = g.x[i]; xyz[3 * i + 1] = g.y[i]; xyz[3 * i + 2] = g.z[i]; }
      AoValues ao;
      eval_ao(b, xyz.data(), static_cast<int>(g.size()), 1, ao);
      Mat W = Eigen::Map<Eigen::VectorXd>(g.w.data(), g.size());
      Mat S = ao.phi.transpose() * (W.asDiagonal() * ao.phi);
      double err = (S - oe.S).cwiseAbs().maxCoeff();
      check(err < 2e-6, std::string("overlap quadrature vs libcint ") + bname + (sph ? " sph" : " cart"), err);
      if (sph) {
        double dn = 0;
        for (int i = 0; i < b.nbf(); ++i) dn = std::max(dn, std::fabs(oe.S(i, i) - 1.0));
        check(dn < 1e-12, std::string("AO norm ") + bname, dn);
      }
      // gradient by central differences at a few points
      double gerr = 0;
      for (int p = 0; p < 20; ++p) {
        size_t i = (p * 7919) % g.size();
        double h = 1e-6, pt[3] = {g.x[i], g.y[i], g.z[i]};
        AoValues a0, ap, am;
        eval_ao(b, pt, 1, 1, a0);
        for (int c = 0; c < 3; ++c) {
          double q[3] = {pt[0], pt[1], pt[2]};
          q[c] += h; eval_ao(b, q, 1, 0, ap);
          q[c] -= 2 * h; eval_ao(b, q, 1, 0, am);
          Mat& an = c == 0 ? a0.dx : (c == 1 ? a0.dy : a0.dz);
          gerr = std::max(gerr, ((ap.phi - am.phi) / (2 * h) - an).cwiseAbs().maxCoeff());
        }
      }
      check(gerr < 1e-6, std::string("AO gradient vs finite difference ") + bname, gerr);
    }
  }
  return fails ? 1 : 0;
}
