#include "charges.hpp"
#include <algorithm>
#include <cmath>
#include "cint_wrap.hpp"

namespace qm {

namespace {

// van der Waals radii in Angstrom (Bondi), used where a scheme does not name
// its own radius for an element.
double bondi(int Z) {
  static const double r[] = {0,    1.20, 1.40, 1.82, 1.53, 1.92, 1.70, 1.55, 1.52, 1.47, 1.54,
                             2.27, 1.73, 1.84, 2.10, 1.80, 1.80, 1.75, 1.88};
  return Z >= 1 && Z < static_cast<int>(sizeof(r) / sizeof(r[0])) ? r[Z] : 2.0;
}

double chelpg_radius(int Z) {
  switch (Z) {
    case 1: return 1.45;
    case 6: return 1.50;
    case 7: case 8: case 9: return 1.70;
    default: return bondi(Z);
  }
}

double mk_radius(int Z) {
  switch (Z) {
    case 1: return 1.20;
    case 6: case 7: return 1.50;
    case 8: return 1.40;
    case 9: return 1.35;
    default: return bondi(Z);
  }
}

double dist(const double* a, const double* b) {
  const double d0 = a[0] - b[0], d1 = a[1] - b[1], d2 = a[2] - b[2];
  return std::sqrt(d0 * d0 + d1 * d1 + d2 * d2);
}

}  // namespace

std::vector<double> loewdin_charges(const Molecule& mol, const Basis& basis, const ScfResult& r) {
  const int n = basis.nbf();
  Eigen::SelfAdjointEigenSolver<Mat> es(r.S);
  const Vec s = es.eigenvalues();
  Vec sq(n);
  for (int i = 0; i < n; ++i) sq(i) = std::sqrt(std::max(s(i), 0.0));
  const Mat Sh = es.eigenvectors() * sq.asDiagonal() * es.eigenvectors().transpose();
  const Mat Q = Sh * (r.D_a + r.D_b) * Sh;
  std::vector<double> q(mol.atoms.size());
  for (size_t a = 0; a < q.size(); ++a) q[a] = mol.atoms[a].Z;
  for (const Shell& sh : basis.shells())
    for (int k = 0; k < sh.nao; ++k) q[sh.atom] -= Q(sh.ao0 + k, sh.ao0 + k);
  return q;
}

std::vector<std::array<double, 3>> chelpg_points(const Molecule& mol) {
  const double step = 0.3 * kAngstromToBohr, rmax = 2.8 * kAngstromToBohr;
  double lo[3], hi[3];
  for (int k = 0; k < 3; ++k) {
    lo[k] = 1e30;
    hi[k] = -1e30;
    for (const auto& a : mol.atoms) {
      lo[k] = std::min(lo[k], a.x[k]);
      hi[k] = std::max(hi[k], a.x[k]);
    }
    lo[k] -= rmax;
    hi[k] += rmax;
  }
  std::vector<double> rad;
  for (const auto& a : mol.atoms) rad.push_back(chelpg_radius(a.Z) * kAngstromToBohr);
  std::vector<std::array<double, 3>> out;
  for (double x = lo[0]; x <= hi[0]; x += step)
    for (double y = lo[1]; y <= hi[1]; y += step)
      for (double z = lo[2]; z <= hi[2]; z += step) {
        const double p[3] = {x, y, z};
        bool ok = true, near = false;
        for (size_t a = 0; a < mol.atoms.size() && ok; ++a) {
          const double d = dist(p, mol.atoms[a].x);
          if (d < rad[a]) ok = false;
          if (d <= rmax) near = true;
        }
        if (ok && near) out.push_back({x, y, z});
      }
  return out;
}

std::vector<std::array<double, 3>> mk_points(const Molecule& mol) {
  const double scales[4] = {1.4, 1.6, 1.8, 2.0};
  const double density = 1.0;  // points per square Angstrom
  const double golden = kPi * (3.0 - std::sqrt(5.0));
  std::vector<std::array<double, 3>> out;
  for (size_t a = 0; a < mol.atoms.size(); ++a)
    for (double sc : scales) {
      const double rA = sc * mk_radius(mol.atoms[a].Z);  // Angstrom
      const int n = std::max(1, static_cast<int>(std::lround(4 * kPi * rA * rA * density)));
      for (int i = 0; i < n; ++i) {
        // Fibonacci spiral: equal-area latitude bands, golden-angle longitude.
        const double zc = 1.0 - (2.0 * i + 1.0) / n, rho = std::sqrt(std::max(0.0, 1 - zc * zc));
        const double phi = golden * i;
        const double d[3] = {rho * std::cos(phi), rho * std::sin(phi), zc};
        double p[3];
        for (int k = 0; k < 3; ++k) p[k] = mol.atoms[a].x[k] + rA * kAngstromToBohr * d[k];
        bool ok = true;
        for (size_t b = 0; b < mol.atoms.size() && ok; ++b)
          if (b != a && dist(p, mol.atoms[b].x) < sc * mk_radius(mol.atoms[b].Z) * kAngstromToBohr) ok = false;
        if (ok) out.push_back({p[0], p[1], p[2]});
      }
    }
  return out;
}

std::vector<double> esp_at_points(Basis& basis, const Molecule& mol, const ScfResult& r,
                                  const std::vector<std::array<double, 3>>& pts) {
  const int ns = basis.nshl();
  const bool sp = basis.spherical();
  const auto& sh = basis.shells();
  const Mat Dt = r.D_a + r.D_b;
  std::vector<double> v(pts.size(), 0.0);
  const int np = static_cast<int>(pts.size());
#pragma omp parallel
  {
    // PTR_RINV_ORIG is the operator centre, so each thread moves it in a
    // private copy of env.
    std::vector<double> env = basis.env();
    std::vector<double> buf, cache;
#pragma omp for schedule(dynamic, 16)
    for (int k = 0; k < np; ++k) {
      for (int c = 0; c < 3; ++c) env[PTR_RINV_ORIG + c] = pts[k][c];
      double e = 0;
      for (int I = 0; I < ns; ++I)
        for (int J = 0; J <= I; ++J) {
          int shls[2] = {I, J};
          const int ni = sh[I].nao, nj = sh[J].nao;
          buf.assign(static_cast<size_t>(ni) * nj, 0.0);
          auto f = sp ? int1e_rinv_sph : int1e_rinv_cart;
          size_t cs = f(nullptr, nullptr, shls, basis.atm().data(), basis.natm(), basis.bas().data(), ns, env.data(), nullptr, nullptr);
          cache.resize(cs + 16);
          f(buf.data(), nullptr, shls, basis.atm().data(), basis.natm(), basis.bas().data(), ns, env.data(), nullptr, cache.data());
          const double w = I == J ? 1.0 : 2.0;
          for (int j = 0; j < nj; ++j)
            for (int i = 0; i < ni; ++i) e += w * Dt(sh[I].ao0 + i, sh[J].ao0 + j) * buf[j * ni + i];
        }
      double nuc = 0;
      for (const auto& a : mol.atoms) nuc += a.Z / dist(pts[k].data(), a.x);
      v[k] = nuc - e;
    }
  }
  return v;
}

std::vector<double> fit_esp_charges(const Molecule& mol, const std::vector<std::array<double, 3>>& pts,
                                    const std::vector<double>& v) {
  const int na = static_cast<int>(mol.atoms.size());
  if (pts.size() < static_cast<size_t>(na)) throw Error("too few points to fit ESP charges");
  Mat A = Mat::Zero(na + 1, na + 1);
  Vec b = Vec::Zero(na + 1);
  Vec inv(na);
  for (size_t k = 0; k < pts.size(); ++k) {
    for (int a = 0; a < na; ++a) inv(a) = 1.0 / dist(pts[k].data(), mol.atoms[a].x);
    A.topLeftCorner(na, na) += inv * inv.transpose();
    b.head(na) += v[k] * inv;
  }
  A.col(na).head(na).setOnes();
  A.row(na).head(na).setOnes();
  b(na) = mol.charge;
  const Vec x = A.colPivHouseholderQr().solve(b);
  return std::vector<double>(x.data(), x.data() + na);
}

ChargeSets compute_charges(Basis& basis, const Molecule& mol, const ScfResult& r, const ChargeOptions& opt) {
  ChargeSets out;
  if (opt.loewdin) out.loewdin = loewdin_charges(mol, basis, r);
  if (opt.chelpg) {
    auto pts = chelpg_points(mol);
    out.chelpg_points = static_cast<int>(pts.size());
    out.chelpg = fit_esp_charges(mol, pts, esp_at_points(basis, mol, r, pts));
  }
  if (opt.mk) {
    auto pts = mk_points(mol);
    out.mk_points = static_cast<int>(pts.size());
    out.mk = fit_esp_charges(mol, pts, esp_at_points(basis, mol, r, pts));
  }
  return out;
}

}  // namespace qm
