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

std::vector<std::array<double, 4>> lebedev_rule(int npts) {
  for (const auto& e : leb_registry)
    if (e.npts == npts) {
      std::vector<std::array<double, 4>> v(npts);
      for (int i = 0; i < npts; ++i) for (int c = 0; c < 4; ++c) v[i][c] = e.data[i][c];
      return v;
    }
  throw Error("no Lebedev grid with " + std::to_string(npts) + " points");
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

double becke_s(double nu, double* derivative = nullptr) {
  double ds = -0.5;
  for (int i = 0; i < 3; ++i) {
    ds *= 1.5 * (1 - nu * nu);
    nu = 1.5 * nu - 0.5 * nu * nu * nu;
  }
  if (derivative) *derivative = ds;
  return 0.5 * (1.0 - nu);
}

}  // namespace

BeckePartition::BeckePartition(const Molecule& mol) : mol_(mol) {
  const int na = static_cast<int>(mol.atoms.size());
  aij_.assign(na * na, 0.0);
  rij_.assign(na * na, 0.0);
  for (int i = 0; i < na; ++i)
    for (int j = 0; j < na; ++j) {
      double r2 = 0;
      for (int c = 0; c < 3; ++c) {
        const double d = mol.atoms[i].x[c] - mol.atoms[j].x[c];
        r2 += d * d;
      }
      rij_[i * na + j] = std::sqrt(r2);
      if (i == j) continue;
      const double chi = bragg(mol.atoms[i].basis_Z()) / bragg(mol.atoms[j].basis_Z());
      const double u = (chi - 1) / (chi + 1);
      aij_[i * na + j] = std::max(-0.5, std::min(0.5, u / (u * u - 1)));
    }
}

double BeckePartition::weight(const std::array<double, 3>& point, int owner,
                              std::vector<double>* gradient) const {
  const int na = static_cast<int>(mol_.atoms.size()), nd = 3 * na;
  if (gradient) gradient->assign(nd, 0.0);
  if (na == 1) return 1.0;
  std::vector<double> dist(na);
  std::vector<std::array<double, 3>> unit(na);
  for (int i = 0; i < na; ++i) {
    double r2 = 0;
    for (int c = 0; c < 3; ++c) {
      unit[i][c] = point[c] - mol_.atoms[i].x[c];
      r2 += unit[i][c] * unit[i][c];
    }
    dist[i] = std::sqrt(r2);
    if (gradient) for (int c = 0; c < 3; ++c)
      unit[i][c] = dist[i] > 0 ? unit[i][c] / dist[i] : 0.0;
  }
  double sum = 0, own = 0;
  std::vector<double> dp(gradient ? nd : 0), dsum(dp.size(), 0.0), down(dp.size());
  for (int i = 0; i < na; ++i) {
    double pr = 1;
    std::fill(dp.begin(), dp.end(), 0.0);
    for (int j = 0; j < na && pr > 1e-30; ++j) {
      if (i == j) continue;
      const double r = rij_[i * na + j], a = aij_[i * na + j];
      const double mu = (dist[i] - dist[j]) / r;
      double ds = 0;
      const double s = becke_s(mu + a * (1 - mu * mu), gradient ? &ds : nullptr);
      if (gradient) {
        for (double& v : dp) v *= s;
        const double scale = pr * ds * (1 - 2 * a * mu) / r;
        for (int c = 0; c < 3; ++c) {
          const double u = (mol_.atoms[i].x[c] - mol_.atoms[j].x[c]) / r;
          dp[3 * i + c] += scale * (-unit[i][c] - mu * u);
          dp[3 * j + c] += scale * (unit[j][c] + mu * u);
        }
      }
      pr *= s;
    }
    sum += pr;
    if (gradient) for (int k = 0; k < nd; ++k) dsum[k] += dp[k];
    if (i == owner) { own = pr; down = dp; }
  }
  const double w = sum > 0 ? own / sum : 0.0;
  if (gradient && sum > 0) {
    for (int k = 0; k < nd; ++k) (*gradient)[k] = (down[k] - w * dsum[k]) / sum;
    // The point follows its centre: d/dr = -sum_A d/dR_A at fixed r.
    for (int c = 0; c < 3; ++c) {
      double shift = 0;
      for (int i = 0; i < na; ++i) shift += (*gradient)[3 * i + c];
      (*gradient)[3 * owner + c] -= shift;
    }
  }
  return w;
}

Grid make_grid(const Molecule& mol, int nrad, int nang, bool prune) {
  const LebEntry* leb = nullptr;
  for (const auto& e : leb_registry)
    if (e.npts == nang) leb = &e;
  if (!leb) throw Error("no Lebedev grid with " + std::to_string(nang) + " points");
  const int na = static_cast<int>(mol.atoms.size());

  // Angular size by radial region, in units of the atom's Bragg radius, as a
  // fraction of nang rounded up to a tabulated Lebedev size (SG-1 style).
  struct Region { double rmax, frac; };
  static const Region regions[] = {{0.25, 0.05}, {0.5, 0.2}, {1.0, 0.5}, {1e30, 1.0}};
  auto leb_for = [&](double r, double rbragg) {
    if (!prune) return leb;
    double frac = 1.0;
    for (const Region& g : regions)
      if (r < g.rmax * rbragg) { frac = g.frac; break; }
    for (const auto& e : leb_registry)
      if (e.npts >= frac * nang) return &e;
    return leb;
  };

  const BeckePartition partition(mol);

  Grid g;
  for (int A = 0; A < na; ++A) {
    const double xi = ta_xi(mol.atoms[A].basis_Z());
    const double rbragg = bragg(mol.atoms[A].basis_Z()) / 0.529177210903;
    for (int ir = 1; ir <= nrad; ++ir) {
      const double th = ir * kPi / (nrad + 1), xr = std::cos(th);
      const double r = xi / std::log(2.0) * std::pow(1 + xr, 0.6) * std::log(2.0 / (1 - xr));
      const double drdx = xi / std::log(2.0) *
                          (0.6 * std::pow(1 + xr, -0.4) * std::log(2.0 / (1 - xr)) + std::pow(1 + xr, 0.6) / (1 - xr));
      const double wr = kPi / (nrad + 1) * std::sin(th) * std::sin(th) / std::sqrt(1 - xr * xr) * drdx * r * r;
      const LebEntry* la = leb_for(r, rbragg);
      for (int ia = 0; ia < la->npts; ++ia) {
        const double* p = la->data[ia];
        const double px = mol.atoms[A].x[0] + r * p[0], py = mol.atoms[A].x[1] + r * p[1],
                     pz = mol.atoms[A].x[2] + r * p[2];
        const double wp = partition.weight({px, py, pz}, A);
        const double w = 4 * kPi * wr * p[3] * wp;
        if (w == 0) continue;
        g.x.push_back(px); g.y.push_back(py); g.z.push_back(pz); g.w.push_back(w);
        g.atom.push_back(A); g.atomic_w.push_back(4 * kPi * wr * p[3]);
      }
    }
  }
  return g;
}

}  // namespace qm
