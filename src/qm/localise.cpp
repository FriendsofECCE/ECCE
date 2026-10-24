#include "localise.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <tuple>

namespace qm {

namespace {

int core_orbitals(int Z) {
  if (Z <= 2) return 0;
  if (Z <= 10) return 1;
  if (Z <= 18) return 5;
  if (Z <= 36) return 9;
  return 18;
}

// Rotating pairs until nothing gains: for orbitals i, j the sum over atoms of
// the squared populations is a + b cos(4t) + c sin(4t) in the angle t.
int pipek_mezey(const Basis& basis, int natm, Mat& X, Mat& U) {
  const int nbf = static_cast<int>(X.rows()), n = static_cast<int>(X.cols());
  std::vector<int> atom_of(nbf);
  for (const Shell& s : basis.shells())
    for (int k = 0; k < s.nao; ++k) atom_of[s.ao0 + k] = s.atom;
  U = Mat::Identity(n, n);
  int sweep = 0;
  for (; sweep < 200; ++sweep) {
    double biggest = 0;
    for (int i = 0; i < n; ++i)
      for (int j = i + 1; j < n; ++j) {
        double P = 0, R = 0;
        std::vector<double> qii(natm, 0), qjj(natm, 0), qij(natm, 0);
        for (int m = 0; m < nbf; ++m) {
          const int a = atom_of[m];
          qii[a] += X(m, i) * X(m, i);
          qjj[a] += X(m, j) * X(m, j);
          qij[a] += X(m, i) * X(m, j);
        }
        for (int a = 0; a < natm; ++a) {
          const double d = 0.5 * (qii[a] - qjj[a]);
          P += d * d - qij[a] * qij[a];
          R += 2 * d * qij[a];
        }
        if (std::hypot(P, R) < 1e-14) continue;
        const double t = 0.25 * std::atan2(R, P);
        if (std::fabs(t) < 1e-10) continue;
        biggest = std::max(biggest, std::fabs(t));
        const double c = std::cos(t), s = std::sin(t);
        for (Mat* M : {&X, &U}) {
          for (int m = 0; m < M->rows(); ++m) {
            const double xi = (*M)(m, i), xj = (*M)(m, j);
            (*M)(m, i) = c * xi + s * xj;
            (*M)(m, j) = -s * xi + c * xj;
          }
        }
      }
    if (biggest < 1e-8) break;
  }
  return sweep;
}

std::string hyb(double n) {
  if (n < 0) return "s";
  if (n > 40) return "p";
  char b[32];
  std::snprintf(b, sizeof b, "sp%.1f", n);
  return b;
}

}  // namespace

Localised localise_occupied(const Molecule& mol, const Basis& basis, const ScfResult& r,
                            const std::vector<int>* atom_number) {
  if (r.reference != Reference::Restricted) throw Error("localisation needs a closed-shell (restricted) wavefunction");
  const int nbf = basis.nbf(), nocc = r.nalpha, natm = static_cast<int>(mol.atoms.size());
  Eigen::SelfAdjointEigenSolver<Mat> es(r.S);
  Vec sq = es.eigenvalues().cwiseMax(0.0).cwiseSqrt();
  const Mat Sh = es.eigenvectors() * sq.asDiagonal() * es.eigenvectors().transpose();
  const Mat Cocc = r.C_a.leftCols(nocc);
  Mat X = Sh * Cocc, U;
  Localised out;
  out.sweeps = pipek_mezey(basis, natm, X, U);
  out.C = Cocc * U;
  const bool have_f = r.F_a.rows() == nbf;

  auto name = [&](int a) {
    return std::string(element_symbol(mol.atoms[a].Z)) + std::to_string(atom_number ? (*atom_number)[a] + 1 : a + 1);
  };
  std::vector<LocOrbital> orbs(nocc);
  std::vector<std::vector<double>> pop(nocc, std::vector<double>(natm, 0.0));
  for (int i = 0; i < nocc; ++i) {
    for (const Shell& s : basis.shells())
      for (int k = 0; k < s.nao; ++k) pop[i][s.atom] += X(s.ao0 + k, i) * X(s.ao0 + k, i);
    orbs[i].energy = have_f ? out.C.col(i).dot(r.F_a * out.C.col(i)) : 0.0;
  }
  std::vector<int> core_left(natm);
  for (int a = 0; a < natm; ++a) core_left[a] = core_orbitals(mol.atoms[a].Z);
  std::vector<int> by_energy(nocc);
  std::iota(by_energy.begin(), by_energy.end(), 0);
  std::sort(by_energy.begin(), by_energy.end(), [&](int a, int b) { return orbs[a].energy < orbs[b].energy; });
  for (int i = 0; i < nocc; ++i) {
    std::vector<int> idx(natm);
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(), [&](int a, int b) { return pop[i][a] > pop[i][b]; });
    for (int a : idx)
      if (pop[i][a] > 0.05 || orbs[i].atoms.empty()) { orbs[i].atoms.push_back(a); orbs[i].pops.push_back(pop[i][a]); }
  }
  for (int i : by_energy) {
    LocOrbital& o = orbs[i];
    if (o.pops[0] > 0.9 && core_left[o.atoms[0]] > 0) { o.kind = LocOrbital::Core; --core_left[o.atoms[0]]; }
    else if (o.atoms.size() >= 2 && o.pops[0] + o.pops[1] >= 0.85 && o.pops[1] >= 0.2) o.kind = LocOrbital::Bond;
    else if (o.pops[0] >= 0.7) o.kind = LocOrbital::LonePair;
    else o.kind = LocOrbital::Multicentre;
    if (o.kind == LocOrbital::Bond && o.atoms[0] > o.atoms[1]) {
      std::swap(o.atoms[0], o.atoms[1]);
      std::swap(o.pops[0], o.pops[1]);
    }
  }

  for (int i = 0; i < nocc; ++i) {
    LocOrbital& o = orbs[i];
    const int ncentre = o.kind == LocOrbital::Bond ? 2 : 1;
    // s and p populations per centre; p split along and across the bond axis
    double axis[3] = {0, 0, 0};
    if (ncentre == 2) {
      double len = 0;
      for (int k = 0; k < 3; ++k) {
        axis[k] = mol.atoms[o.atoms[1]].x[k] - mol.atoms[o.atoms[0]].x[k];
        len += axis[k] * axis[k];
      }
      len = std::sqrt(len);
      for (double& v : axis) v /= len;
    }
    double par = 0, perp = 0, stot = 0;
    for (int c = 0; c < ncentre; ++c) {
      double sp = 0, pp = 0;
      for (const Shell& s : basis.shells()) {
        if (s.atom != o.atoms[c]) continue;
        if (s.l == 0)
          for (int k = 0; k < s.nao; ++k) sp += X(s.ao0 + k, i) * X(s.ao0 + k, i);
        if (s.l == 1) {
          const double v[3] = {X(s.ao0, i), X(s.ao0 + 1, i), X(s.ao0 + 2, i)};
          const double dp = v[0] * axis[0] + v[1] * axis[1] + v[2] * axis[2];
          const double all = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
          pp += all;
          par += dp * dp;
          perp += all - dp * dp;
        }
      }
      stot += sp;
      double n = pp > 1e-4 ? (sp > 1e-4 ? pp / sp : 99.0) : -1.0;
      if (mol.atoms[o.atoms[c]].Z == 1) n = -1.0;
      o.hybrid_n.push_back(n);
    }
    switch (o.kind) {
      case LocOrbital::Core: o.label = "core " + name(o.atoms[0]); break;
      case LocOrbital::LonePair:
        o.label = "lone pair " + name(o.atoms[0]) + " (" + hyb(o.hybrid_n[0]) + ")";
        break;
      case LocOrbital::Bond:
        o.pi = perp > par && perp > stot;
        o.label = std::string(o.pi ? "pi " : "sigma ") + name(o.atoms[0]) + "-" + name(o.atoms[1]);
        if (!o.pi) o.label += " (" + hyb(o.hybrid_n[0]) + "/" + hyb(o.hybrid_n[1]) + ")";
        break;
      default:
        o.label = "delocalised";
        for (size_t c = 0; c < o.atoms.size(); ++c) o.label += (c ? "-" : " ") + name(o.atoms[c]);
    }
  }

  // core first, then bonds and lone pairs by atom, the delocalised ones last
  std::vector<int> ord(nocc);
  std::iota(ord.begin(), ord.end(), 0);
  auto key = [&](int i) {
    const LocOrbital& o = orbs[i];
    const int a1 = o.kind == LocOrbital::Bond ? o.atoms[1] : o.atoms[0];
    return std::make_tuple(o.kind != LocOrbital::Core, o.kind == LocOrbital::Multicentre, o.atoms[0], a1,
                           static_cast<int>(o.kind), o.pi, o.energy);
  };
  std::stable_sort(ord.begin(), ord.end(), [&](int a, int b) { return key(a) < key(b); });
  Mat C2(nbf, nocc);
  for (int k = 0; k < nocc; ++k) {
    C2.col(k) = out.C.col(ord[k]);
    out.orb.push_back(orbs[ord[k]]);
  }
  out.C = C2;
  return out;
}

}  // namespace qm
