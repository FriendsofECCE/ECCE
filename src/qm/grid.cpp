#include "grid.hpp"
#include <algorithm>
#include <cmath>
#include "common.hpp"
#include "lebedev_data.inc"

namespace qm {

std::vector<int> lebedev_sizes() {
  std::vector<int> v;
  for (const auto& e : leb_registry) v.push_back(e.npts);
  return v;
}

namespace {

// Treutler-Ahlrichs radial scaling xi (Z = 1..36); 1.0-1.3 beyond.
double ta_xi(int Z) {
  static const double xi[] = {0, 0.8, 0.9, 1.8, 1.4, 1.3, 1.1, 0.9, 0.9, 0.9, 0.9,
                              1.4, 1.3, 1.3, 1.2, 1.1, 1.0, 1.0, 1.0, 1.5, 1.4};
  if (Z < static_cast<int>(sizeof(xi) / sizeof(xi[0]))) return xi[Z];
  return 1.3;
}

// Bragg-Slater radii (angstrom) for Becke's size adjustment.
double bragg(int Z) {
  static const double r[] = {0, 0.35, 0.35, 1.45, 1.05, 0.85, 0.70, 0.65, 0.60, 0.50, 0.45,
                             1.80, 1.50, 1.25, 1.10, 1.00, 1.00, 1.00, 1.00};
  if (Z < static_cast<int>(sizeof(r) / sizeof(r[0]))) return r[Z];
  return 1.3;
}

double becke_s(double nu) {
  for (int i = 0; i < 3; ++i) nu = 1.5 * nu - 0.5 * nu * nu * nu;
  return 0.5 * (1.0 - nu);
}

}  // namespace

Grid make_grid(const Molecule& mol, int nrad, int nang) {
  const LebEntry* leb = nullptr;
  for (const auto& e : leb_registry)
    if (e.npts == nang) leb = &e;
  if (!leb) throw Error("no Lebedev grid with " + std::to_string(nang) + " points");
  const int na = static_cast<int>(mol.atoms.size());

  // Becke size-adjustment parameters a_ij.
  std::vector<double> aij(na * na, 0.0), rij(na * na, 0.0);
  for (int i = 0; i < na; ++i)
    for (int j = 0; j < na; ++j) {
      double dx = mol.atoms[i].x[0] - mol.atoms[j].x[0], dy = mol.atoms[i].x[1] - mol.atoms[j].x[1],
             dz = mol.atoms[i].x[2] - mol.atoms[j].x[2];
      rij[i * na + j] = std::sqrt(dx * dx + dy * dy + dz * dz);
      if (i != j) {
        double chi = bragg(mol.atoms[i].Z) / bragg(mol.atoms[j].Z);
        double u = (chi - 1) / (chi + 1), a = u / (u * u - 1);
        aij[i * na + j] = std::max(-0.5, std::min(0.5, a));
      }
    }

  Grid g;
  std::vector<double> P(na);
  for (int A = 0; A < na; ++A) {
    const double xi = ta_xi(mol.atoms[A].Z);
    for (int ir = 1; ir <= nrad; ++ir) {
      const double th = ir * kPi / (nrad + 1), xr = std::cos(th);
      const double r = xi / std::log(2.0) * std::pow(1 + xr, 0.6) * std::log(2.0 / (1 - xr));
      const double drdx = xi / std::log(2.0) *
                          (0.6 * std::pow(1 + xr, -0.4) * std::log(2.0 / (1 - xr)) + std::pow(1 + xr, 0.6) / (1 - xr));
      const double wr = kPi / (nrad + 1) * std::sin(th) * std::sin(th) / std::sqrt(1 - xr * xr) * drdx * r * r;
      for (int ia = 0; ia < leb->npts; ++ia) {
        const double* p = leb->data[ia];
        const double px = mol.atoms[A].x[0] + r * p[0], py = mol.atoms[A].x[1] + r * p[1],
                     pz = mol.atoms[A].x[2] + r * p[2];
        double wp = 1.0;
        if (na > 1) {
          std::vector<double> d(na);
          for (int k = 0; k < na; ++k) {
            double dx = px - mol.atoms[k].x[0], dy = py - mol.atoms[k].x[1], dz = pz - mol.atoms[k].x[2];
            d[k] = std::sqrt(dx * dx + dy * dy + dz * dz);
          }
          double sum = 0;
          for (int i = 0; i < na; ++i) {
            double pr = 1;
            for (int j = 0; j < na && pr > 1e-30; ++j) {
              if (i == j) continue;
              double mu = (d[i] - d[j]) / rij[i * na + j];
              pr *= becke_s(mu + aij[i * na + j] * (1 - mu * mu));
            }
            P[i] = pr;
            sum += pr;
          }
          wp = sum > 0 ? P[A] / sum : 0.0;
        }
        const double w = 4 * kPi * wr * p[3] * wp;
        if (w == 0) continue;
        g.x.push_back(px); g.y.push_back(py); g.z.push_back(pz); g.w.push_back(w);
      }
    }
  }
  return g;
}

}  // namespace qm
