#include "tscheck.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace qm {
namespace {

// Covalent radii (angstrom) for the main-group and first-row transition
// elements; anything else gets 1.5.
double covalent_radius(int Z) {
  static const double r[] = {0, 0.31, 0.28, 1.28, 0.96, 0.84, 0.76, 0.71, 0.66, 0.57, 0.58, 1.66, 1.41, 1.21, 1.11,
                             1.07, 1.05, 1.02, 1.06, 2.03, 1.76, 1.70, 1.60, 1.53, 1.39, 1.39, 1.32, 1.26, 1.24,
                             1.32, 1.22, 1.22, 1.20, 1.19, 1.20, 1.20, 1.16};
  return Z >= 1 && Z < static_cast<int>(sizeof r / sizeof *r) ? r[Z] : 1.5;
}

double dist_ang(const Molecule& m, int i, int j) {
  double s = 0;
  for (int k = 0; k < 3; ++k) s += (m.atoms[i].x[k] - m.atoms[j].x[k]) * (m.atoms[i].x[k] - m.atoms[j].x[k]);
  return std::sqrt(s) * kBohrToAngstrom;
}

std::string label(const Molecule& m, int i) { return std::string(element_symbol(m.atoms[i].Z)) + std::to_string(i + 1); }

// Overlap of the unit-length mode with the unit gradient of a coordinate,
// by central differences of the coordinate along the mode.
double overlap(const Molecule& m, const GeoConstraint& c, const Mat& modes, int col) {
  const int n = static_cast<int>(m.atoms.size());
  auto value = [&](const Vec& d, double h) {
    Molecule q = m;
    for (int a = 0; a < n; ++a)
      for (int k = 0; k < 3; ++k) q.atoms[a].x[k] += h * d(3 * a + k);
    const Atom* a = q.atoms.data();
    auto vec = [&](int i, int j, double* v) { for (int k = 0; k < 3; ++k) v[k] = a[j].x[k] - a[i].x[k]; };
    double u[3], v[3], w[3];
    if (c.kind == GeoConstraint::Bond) { vec(c.atoms[0], c.atoms[1], u); return std::sqrt(u[0]*u[0] + u[1]*u[1] + u[2]*u[2]); }
    if (c.kind == GeoConstraint::Angle) {
      vec(c.atoms[1], c.atoms[0], u); vec(c.atoms[1], c.atoms[2], v);
      const double d = u[0]*v[0] + u[1]*v[1] + u[2]*v[2];
      return std::acos(d / std::sqrt((u[0]*u[0] + u[1]*u[1] + u[2]*u[2]) * (v[0]*v[0] + v[1]*v[1] + v[2]*v[2])));
    }
    vec(c.atoms[0], c.atoms[1], u); vec(c.atoms[1], c.atoms[2], v); vec(c.atoms[2], c.atoms[3], w);
    auto cross = [](const double* p, const double* q, double* o) { o[0] = p[1]*q[2]-p[2]*q[1]; o[1] = p[2]*q[0]-p[0]*q[2]; o[2] = p[0]*q[1]-p[1]*q[0]; };
    double n1[3], n2[3]; cross(u, v, n1); cross(v, w, n2);
    double vn = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]), m1[3];
    cross(n1, v, m1);
    return std::atan2((m1[0]*n2[0] + m1[1]*n2[1] + m1[2]*n2[2]) / vn, n1[0]*n2[0] + n1[1]*n2[1] + n1[2]*n2[2]);
  };
  // |B.mode| / |B| with B the coordinate's Cartesian gradient: B.mode is the
  // derivative along the mode; |B| from the same differences along each axis.
  const double h = 1e-3;
  const Vec mode = modes.col(col);
  const double dq = (value(mode, h) - value(mode, -h)) / (2 * h);
  double b2 = 0;
  for (int i = 0; i < 3 * n; ++i) {
    Vec e = Vec::Zero(3 * n);
    e(i) = 1;
    const double g = (value(e, h) - value(e, -h)) / (2 * h);
    b2 += g * g;
  }
  return b2 > 0 ? std::fabs(dq) / std::sqrt(b2) : 0.0;
}

}  // namespace

std::vector<std::string> check_saddle(const Molecule& start, const Molecule& end, const VibResult& vib,
                                      const OptOptions& opt, double min_overlap) {
  std::vector<std::string> out;
  const int n = static_cast<int>(end.atoms.size());
  if (n < 2 || static_cast<int>(start.atoms.size()) != n || vib.modes.cols() < 1 || vib.modes.rows() != 3 * n) return out;

  struct Bond { int i, j; double change; };
  std::vector<Bond> bonds;
  for (int i = 0; i < n; ++i)
    for (int j = i + 1; j < n; ++j) {
      const double sum = covalent_radius(end.atoms[i].Z) + covalent_radius(end.atoms[j].Z);
      const double d0 = dist_ang(start, i, j), d1 = dist_ang(end, i, j);
      if (d0 < 1.5 * sum && d1 > 2.0 * sum)
        out.push_back("the bond " + label(end, i) + "-" + label(end, j) + " is broken in the saddle found (" +
                      std::to_string(d0).substr(0, 4) + " to " + std::to_string(d1).substr(0, 4) + " A): check the structure");
      if (d0 < 1.5 * sum || d1 < 1.5 * sum) bonds.push_back({i, j, std::fabs(d1 - d0)});
    }

  std::vector<GeoConstraint> coords;
  std::string name;
  if (opt.has_ts_follow) {
    coords.push_back(opt.ts_follow);
    const int na = opt.ts_follow.kind == GeoConstraint::Bond ? 2 : opt.ts_follow.kind == GeoConstraint::Angle ? 3 : 4;
    name = opt.ts_follow.kind == GeoConstraint::Bond ? "the bond " : opt.ts_follow.kind == GeoConstraint::Angle ? "the angle " : "the torsion ";
    for (int k = 0; k < na; ++k) name += (k ? "-" : "") + label(end, opt.ts_follow.atoms[k]);
  } else {
    std::sort(bonds.begin(), bonds.end(), [](const Bond& a, const Bond& b) { return a.change > b.change; });
    // Bonds that moved less than 0.05 A say nothing about where the search went.
    for (size_t k = 0; k < bonds.size() && k < 2 && bonds[k].change > 0.05; ++k) {
      GeoConstraint c;
      c.kind = GeoConstraint::Bond;
      c.atoms[0] = bonds[k].i;
      c.atoms[1] = bonds[k].j;
      coords.push_back(c);
      name += std::string(k ? " or " : "the bond ") + label(end, bonds[k].i) + "-" + label(end, bonds[k].j);
    }
  }
  if (!coords.empty()) {
    double best = 0;
    for (const auto& c : coords) best = std::max(best, overlap(end, c, vib.modes, 0));
    if (best < min_overlap) out.push_back("the saddle found does not move " + name + ": check the structure");
  }
  return out;
}

}  // namespace qm
