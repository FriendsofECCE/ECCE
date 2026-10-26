#include "solvent.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include "cint_wrap.hpp"
#include "grid.hpp"

namespace qm {

namespace {

// Gaussian exponent of a surface charge: zeta = kZetaN / sqrt(area).  The
// Gaussian self-energy zeta*sqrt(2/pi) then equals the point-charge diagonal
// 1.07*sqrt(4 pi/area) of Klamt & Schuurmann (J. Chem. Soc. Perkin 2, 799,
// 1993) at 4.75; Scalmani & Frisch tabulate 4.8-4.9 for 110-590 points
// (value taken from memory, not re-checked against the paper).
constexpr double kZetaN = 4.84;
// Points whose exposed fraction is below this carry no charge.
constexpr double kMinExposure = 1e-8;

}  // namespace

bool solvent_epsilon(const std::string& name, double& eps) {
  struct E { const char* n; double e; };
  // Static dielectric constants at 298 K, from the Minnesota Solvent
  // Descriptor Database (Winget, Dolney, Giesen, Cramer, Truhlar; 2010 and
  // 2021 updates), which lists water as 78.355.
  static const E t[] = {{"water", 78.3553},   {"methanol", 32.613},  {"ethanol", 24.852},
                        {"acetonitrile", 35.688}, {"dmso", 46.826}, {"chloroform", 4.7113},
                        {"toluene", 2.3741},  {"hexane", 1.8819}};
  std::string l;
  for (char c : name) l += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  for (const E& e : t)
    if (l == e.n) { eps = e.e; return true; }
  return false;
}

std::string solvent_names() { return "water, methanol, ethanol, acetonitrile, dmso, chloroform, toluene, hexane"; }

double cavity_radius(int Z) {
  // Bondi (J. Phys. Chem. 68, 441 (1964)) and Mantina et al. (J. Phys. Chem. A
  // 113, 5806 (2009)); H = 1.10 and the values for Z <= 35 are the radii ORCA
  // 6.1.1 prints for CPCM divided by its 1.2 scaling.
  static const double r[] = {0,    1.10, 1.40, 1.82, 1.53, 1.92, 1.70, 1.55, 1.52, 1.47, 1.54,   // H-Ne
                             2.27, 1.73, 1.84, 2.10, 1.80, 1.80, 1.75, 1.88, 2.75, 2.31,          // Na-Ca
                             0,0,0,0,0,0,0,0,0,0,                                                  // Sc-Zn
                             1.87, 2.11, 1.85, 1.90, 1.85, 2.02};                                  // Ga-Kr
  if (Z >= 1 && Z < static_cast<int>(sizeof(r) / sizeof(r[0])) && r[Z] > 0) return r[Z];
  throw Error(std::string("no solvent cavity radius for element ") + element_symbol(Z));
}

Cpcm::Cpcm(const Molecule& mol, double eps, double radius_scale, int nang) {
  if (eps < 1.0) throw Error("the dielectric constant must be at least 1");
  f_ = (eps - 1.0) / eps;  // CPCM; (eps-1)/(eps+1/2) is COSMO's, no better here
  const auto rule = lebedev_rule(nang);
  const int na = static_cast<int>(mol.atoms.size());
  std::vector<double> R(na);
  for (int i = 0; i < na; ++i) R[i] = radius_scale * cavity_radius(mol.atoms[i].Z) * kAngstromToBohr;
  for (int i = 0; i < na; ++i)
    for (const auto& q : rule) {
      std::array<double, 3> p;
      for (int c = 0; c < 3; ++c) p[c] = mol.atoms[i].x[c] + R[i] * q[c];
      const double area = 4.0 * kPi * R[i] * R[i] * q[3];
      const double zeta = kZetaN / std::sqrt(area);
      // Gaussian-smoothed step at every other sphere's surface; the width is
      // that of the surface charge itself.
      double F = 1.0;
      for (int j = 0; j < na && F >= kMinExposure; ++j) {
        if (j == i) continue;
        const double dx = p[0] - mol.atoms[j].x[0], dy = p[1] - mol.atoms[j].x[1], dz = p[2] - mol.atoms[j].x[2];
        F *= 0.5 * (1.0 + std::erf(zeta * (std::sqrt(dx * dx + dy * dy + dz * dz) - R[j])));
      }
      if (F < kMinExposure) continue;
      pos_.push_back(p);
      zeta_.push_back(zeta);
      F_.push_back(F);
    }
  const int n = size();
  // Charge-charge Coulomb matrix of unit-normalised Gaussians; column l is
  // scaled by F_l so that q_l = F_l p_l stays finite as a point is switched off.
  Mat M(n, n);
  for (int k = 0; k < n; ++k) {
    M(k, k) = zeta_[k] * std::sqrt(2.0 / kPi);
    for (int l = 0; l < k; ++l) {
      const double dx = pos_[k][0] - pos_[l][0], dy = pos_[k][1] - pos_[l][1], dz = pos_[k][2] - pos_[l][2];
      const double r = std::sqrt(dx * dx + dy * dy + dz * dz);
      const double z = zeta_[k] * zeta_[l] / std::sqrt(zeta_[k] * zeta_[k] + zeta_[l] * zeta_[l]);
      const double s = std::erf(z * r) / r;
      M(k, l) = s * F_[l];
      M(l, k) = s * F_[k];
    }
  }
  lu_.compute(M);
}

Vec Cpcm::nuclear_potential(const Molecule& mol) const {
  Vec v = Vec::Zero(size());
  for (int k = 0; k < size(); ++k)
    for (const Atom& a : mol.atoms) {
      const double dx = pos_[k][0] - a.x[0], dy = pos_[k][1] - a.x[1], dz = pos_[k][2] - a.x[2];
      v(k) += a.Z / std::sqrt(dx * dx + dy * dy + dz * dz);
    }
  return v;
}

Vec Cpcm::charges(const Vec& V) const {
  Vec p = lu_.solve(-f_ * V);
  for (int k = 0; k < size(); ++k) p(k) *= F_[k];
  return p;
}

CpcmField::CpcmField(Basis& basis, const Molecule& mol, double eps, double radius_scale)
    : basis_(basis), cav_(mol, eps, radius_scale), vnuc_(cav_.nuclear_potential(mol)) {
  const int n = basis.nbf(), np = cav_.size();
  if (static_cast<double>(np) * n * n * 8.0 <= 1.0e9) {
    P_.resize(np, static_cast<Eigen::Index>(n) * n);
    cached_ = true;
#pragma omp parallel
    {
      std::vector<double> env = basis_.env(), buf, cache;
      Mat m;
#pragma omp for schedule(dynamic, 8)
      for (int k = 0; k < np; ++k) {
        point_matrix(k, env, buf, cache, m);
        P_.row(k) = Eigen::Map<const Eigen::RowVectorXd>(m.data(), m.size());
      }
    }
  }
}

// PTR_RINV_ORIG is the operator centre; every thread moves it in its own env.
void CpcmField::point_matrix(int k, std::vector<double>& env, std::vector<double>& buf, std::vector<double>& cache, Mat& out) const {
  Basis& b = basis_;
  const auto& sh = b.shells();
  const int ns = b.nshl(), n = b.nbf();
  const bool sp = b.spherical();
  out.setZero(n, n);
  for (int c = 0; c < 3; ++c) env[PTR_RINV_ORIG + c] = cav_.points()[k][c];
  auto f = sp ? int1e_rinv_sph : int1e_rinv_cart;
  for (int I = 0; I < ns; ++I)
    for (int J = 0; J <= I; ++J) {
      int shls[2] = {I, J};
      const int ni = sh[I].nao, nj = sh[J].nao;
      buf.assign(static_cast<size_t>(ni) * nj, 0.0);
      const size_t cs = f(nullptr, nullptr, shls, b.atm().data(), b.natm(), b.bas().data(), ns, env.data(), nullptr, nullptr);
      cache.resize(cs + 16);
      f(buf.data(), nullptr, shls, b.atm().data(), b.natm(), b.bas().data(), ns, env.data(), nullptr, cache.data());
      for (int j = 0; j < nj; ++j)
        for (int i = 0; i < ni; ++i) {
          out(sh[I].ao0 + i, sh[J].ao0 + j) = buf[j * ni + i];
          out(sh[J].ao0 + j, sh[I].ao0 + i) = buf[j * ni + i];
        }
    }
}

Vec CpcmField::potential(const Mat& Dt) {
  const int np = cav_.size();
  Vec v = vnuc_;
  if (cached_) {
    v -= P_ * Eigen::Map<const Vec>(Dt.data(), Dt.size());
    return v;
  }
  Vec e(np);
#pragma omp parallel
  {
    std::vector<double> env = basis_.env(), buf, cache;
    Mat m;
#pragma omp for schedule(dynamic, 8)
    for (int k = 0; k < np; ++k) {
      point_matrix(k, env, buf, cache, m);
      e(k) = (Dt.array() * m.array()).sum();
    }
  }
  return v - e;
}

Mat CpcmField::fock(const Vec& q) {
  const int n = basis_.nbf(), np = cav_.size();
  Mat F = Mat::Zero(n, n);
  if (cached_) {
    Vec f = P_.transpose() * q;
    return -Eigen::Map<const Mat>(f.data(), n, n);
  }
#pragma omp parallel
  {
    std::vector<double> env = basis_.env(), buf, cache;
    Mat m, acc = Mat::Zero(n, n);
#pragma omp for schedule(dynamic, 8)
    for (int k = 0; k < np; ++k) {
      point_matrix(k, env, buf, cache, m);
      acc += q(k) * m;
    }
#pragma omp critical
    F -= acc;
  }
  return F;
}

}  // namespace qm
