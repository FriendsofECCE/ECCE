#include "scf.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include "integrals.hpp"

namespace qm {

namespace {

// Pulay DIIS on a stack of matrices.
class Diis {
 public:
  explicit Diis(int max = 8) : max_(max) {}
  void push(const Mat& f, const Mat& e) {
    F_.push_back(f);
    E_.push_back(e);
    if (static_cast<int>(F_.size()) > max_) { F_.erase(F_.begin()); E_.erase(E_.begin()); }
  }
  Mat extrapolate() const {
    const int n = static_cast<int>(F_.size());
    if (n < 2) return F_.back();
    Mat B = Mat::Zero(n + 1, n + 1);
    for (int i = 0; i < n; ++i)
      for (int j = 0; j <= i; ++j) B(i, j) = B(j, i) = (E_[i].array() * E_[j].array()).sum();
    for (int i = 0; i < n; ++i) B(i, n) = B(n, i) = -1;
    Vec rhs = Vec::Zero(n + 1);
    rhs(n) = -1;
    // scale for conditioning
    double s = B.topLeftCorner(n, n).diagonal().maxCoeff();
    if (s > 0) B.topLeftCorner(n, n) /= s;
    Vec c = B.colPivHouseholderQr().solve(rhs);
    Mat f = Mat::Zero(F_[0].rows(), F_[0].cols());
    for (int i = 0; i < n; ++i) f += c(i) * F_[i];
    return f;
  }
  void reset() { F_.clear(); E_.clear(); }
  bool empty() const { return F_.empty(); }
 private:
  int max_;
  std::vector<Mat> F_, E_;
};

Mat inv_sqrt(const Mat& S) {
  Eigen::SelfAdjointEigenSolver<Mat> es(S);
  if (es.eigenvalues()(0) < 1e-8)
    throw Error("overlap matrix is (nearly) singular: smallest eigenvalue " + std::to_string(es.eigenvalues()(0)));
  Vec d = es.eigenvalues().array().rsqrt();
  return es.eigenvectors() * d.asDiagonal() * es.eigenvectors().transpose();
}

void diagonalize(const Mat& F, const Mat& X, Vec& eps, Mat& C) {
  Eigen::SelfAdjointEigenSolver<Mat> es(X.transpose() * F * X);
  eps = es.eigenvalues();
  C = X * es.eigenvectors();
}

Mat density(const Mat& C, int nocc) { return C.leftCols(nocc) * C.leftCols(nocc).transpose(); }

// Spin-averaged fractional-occupation atomic HF density for the initial guess.
Mat atomic_density(int Z, BasisLibrary& lib, const std::string& name, bool sph) {
  Molecule am;
  Atom a;
  a.Z = Z;
  am.atoms.push_back(a);
  Basis b(am, lib, name, sph ? 1 : 0);
  OneElectron oe = one_electron(b);
  EriStore eri(b);
  const int n = b.nbf();
  Mat X = inv_sqrt(oe.S), H = oe.T + oe.V, D;
  Vec eps;
  Mat C;
  diagonalize(H, X, eps, C);
  auto occupy = [&](const Vec& e, const Mat& Cm) {
    Vec occ = Vec::Zero(n);
    double left = Z;
    int i = 0;
    while (i < n && left > 1e-12) {
      int j = i;
      while (j + 1 < n && std::fabs(e(j + 1) - e(i)) < 1e-3) ++j;
      double cap = 2.0 * (j - i + 1), put = std::min(cap, left);
      for (int k = i; k <= j; ++k) occ(k) = put / (j - i + 1);
      left -= put;
      i = j + 1;
    }
    Mat Dm = Mat::Zero(n, n);
    for (int k = 0; k < n; ++k)
      if (occ(k) > 0) Dm += occ(k) * Cm.col(k) * Cm.col(k).transpose();
    return Dm;
  };
  D = occupy(eps, C);
  Diis diis;
  double eold = 0;
  for (int it = 0; it < 100; ++it) {
    std::vector<Mat> J, K;
    eri.jk({D}, J, K, true);
    Mat F = H + J[0] - 0.5 * K[0];
    double e = 0.5 * (D.array() * (H + F).array()).sum();
    Mat err = X.transpose() * (F * D * oe.S - oe.S * D * F) * X;
    diis.push(F, err);
    diagonalize(diis.extrapolate(), X, eps, C);
    D = occupy(eps, C);
    if (it > 2 && std::fabs(e - eold) < 1e-8 && err.cwiseAbs().maxCoeff() < 1e-5) break;
    eold = e;
  }
  return D;
}

}  // namespace

ScfResult run_scf(Basis& basis, const Molecule& mol, const ScfOptions& opt) {
  mol.check();
  FunctionalSpec fs = functional_by_name(opt.method);
  const bool dft = !fs.is_hf();
  const int n = basis.nbf();
  const int na = mol.nalpha(), nb = mol.nbeta();
  Reference ref = opt.reference;
  if (ref == Reference::Restricted && na != nb) throw Error("restricted reference needs multiplicity 1; use uhf or rohf");
  if (ref == Reference::RestrictedOpen && na == nb) ref = Reference::Restricted;
  if (n < na) throw Error("fewer basis functions than occupied orbitals");
  const bool closed = ref == Reference::Restricted;
  const double exx = fs.exx;
  const bool want_k = exx != 0.0;

  ScfResult R;
  R.reference = ref;
  R.nalpha = na;
  R.nbeta = nb;
  R.e_nuc = mol.nuclear_repulsion();

  OneElectron oe = one_electron(basis);
  const Mat H = oe.T + oe.V, &S = oe.S;
  R.S = S;
  EriStore eri(basis);
  const Mat X = inv_sqrt(S);

  std::unique_ptr<XcIntegrator> xc;
  if (dft) xc.reset(new XcIntegrator(basis, fs, make_grid(mol, opt.grid_radial, opt.grid_angular)));

  // Initial guess: superposition of spin-averaged atomic densities.
  Mat Dsad = Mat::Zero(n, n);
  {
    std::map<int, Mat> atomd;
    for (const Shell& s : basis.shells()) (void)s;
    for (size_t a = 0; a < mol.atoms.size(); ++a) {
      int Z = mol.atoms[a].Z;
      if (!atomd.count(Z)) atomd[Z] = atomic_density(Z, basis.library(), basis.name(), basis.spherical());
      // place the atomic block on this atom's AOs (same shell order per element)
      std::vector<int> idx;
      for (const Shell& s : basis.shells())
        if (s.atom == static_cast<int>(a))
          for (int k = 0; k < s.nao; ++k) idx.push_back(s.ao0 + k);
      const Mat& Da0 = atomd[Z];
      if (static_cast<int>(idx.size()) != Da0.rows()) throw Error("internal: atomic guess size mismatch");
      for (size_t p = 0; p < idx.size(); ++p)
        for (size_t q = 0; q < idx.size(); ++q) Dsad(idx[p], idx[q]) = Da0(p, q);
    }
  }
  const double scale = static_cast<double>(na + nb) / std::max(1.0, Dsad.cwiseProduct(S).sum());
  Mat Da = 0.5 * scale * Dsad, Db = Da;

  Diis diis;
  Vec eps_a, eps_b;
  Mat Ca, Cb;
  double Eold = 0;
  bool first = true;
  Mat Fa, Fb;
  for (int it = 1; it <= opt.max_iter; ++it) {
    // ---- Fock matrices and energy ----
    std::vector<Mat> J, K;
    Mat Jt, Ka, Kb;
    if (closed) {
      eri.jk({Da}, J, K, want_k);
      Jt = 2.0 * J[0];
      if (want_k) Ka = Kb = K[0];
    } else {
      eri.jk({Da, Db}, J, K, want_k);
      Jt = J[0] + J[1];
      if (want_k) { Ka = K[0]; Kb = K[1]; }
    }
    Fa = H + Jt;
    Fb = Fa;
    double Exc = 0, EK = 0;
    if (want_k) {
      Fa -= exx * Ka;
      Fb -= exx * Kb;
      EK = -0.5 * exx * ((Da.array() * Ka.array()).sum() + (Db.array() * Kb.array()).sum());
    }
    if (dft) {
      Mat Va, Vb;
      Exc = xc->compute(Da, Db, closed, Va, Vb, &R.xc_electrons);
      Fa += Va;
      Fb += Vb;
    }
    const Mat Dt = Da + Db;
    const double E1 = (Dt.array() * H.array()).sum();
    const double EJ = 0.5 * (Dt.array() * Jt.array()).sum();
    const double E = E1 + EJ + EK + Exc + R.e_nuc;
    R.e_one = E1; R.e_coulomb = EJ; R.e_exchange_hf = EK; R.e_xc = Exc;

    // ---- error vector, effective Fock ----
    Mat F, err;
    if (closed) {
      F = Fa;
      err = X.transpose() * (F * Da * S - S * Da * F) * X;
    } else if (ref == Reference::Unrestricted) {
      Mat ea = X.transpose() * (Fa * Da * S - S * Da * Fa) * X, eb = X.transpose() * (Fb * Db * S - S * Db * Fb) * X;
      err.resize(n, 2 * n);
      err << ea, eb;
      F.resize(n, 2 * n);
      F << Fa, Fb;
    } else {
      // Restricted open shell: effective Fock matrix in AO form with the
      // blocks core-core = (Fa+Fb)/2, open-open = virtual-virtual = Fa,
      // core-open = Fb, open-virtual = Fa, core-virtual = (Fa+Fb)/2.
      // The off-diagonal blocks vanish at convergence; the diagonal blocks
      // only fix the canonical orbitals (docs/qm/README.md).
      const Mat Fc = 0.5 * (Fa + Fb);
      if (first) F = Fc;
      else {
        const Mat I = Mat::Identity(n, n);
        Mat Pc = Db * S, Po = (Da - Db) * S, Pv = I - Da * S;
        F = 0.5 * (Pc.transpose() * Fc * Pc + Po.transpose() * Fa * Po + Pv.transpose() * Fa * Pv);
        F += Po.transpose() * Fb * Pc + Po.transpose() * Fa * Pv + Pv.transpose() * Fc * Pc;
        F = F + F.transpose().eval();
      }
      Mat ea = X.transpose() * (F * Da * S - S * Da * F) * X, eb = X.transpose() * (F * Db * S - S * Db * F) * X;
      err.resize(n, 2 * n);
      err << ea, eb;
      if (first) err.setConstant(1.0);  // no meaningful gradient yet
    }
    const double gmax = err.cwiseAbs().maxCoeff();
    if (opt.verbose)
      std::cerr << "iter " << it << "  E = " << E << "  dE = " << E - Eold << "  |grad| = " << gmax << "\n";
    if (!first && std::fabs(E - Eold) < opt.conv_energy && gmax < opt.conv_grad) {
      R.converged = true;
      R.iterations = it;
      R.energy = E;
      // final orbitals from the (unextrapolated) effective Fock
      if (closed) { diagonalize(Fa, X, eps_a, Ca); eps_b = eps_a; Cb = Ca; }
      else if (ref == Reference::Unrestricted) { diagonalize(Fa, X, eps_a, Ca); diagonalize(Fb, X, eps_b, Cb); }
      else { diagonalize(F, X, eps_a, Ca); eps_b = eps_a; Cb = Ca; }
      break;
    }
    Eold = E;
    if (!first || ref != Reference::RestrictedOpen) diis.push(F, err);
    first = false;
    Mat Fx = diis.empty() ? F : diis.extrapolate();
    if (closed) {
      diagonalize(Fx, X, eps_a, Ca);
      Da = Db = density(Ca, na);
    } else if (ref == Reference::Unrestricted) {
      diagonalize(Fx.leftCols(n), X, eps_a, Ca);
      diagonalize(Fx.rightCols(n), X, eps_b, Cb);
      Da = density(Ca, na);
      Db = density(Cb, nb);
    } else {
      diagonalize(Fx, X, eps_a, Ca);
      eps_b = eps_a;
      Cb = Ca;
      Da = density(Ca, na);
      Db = density(Ca, nb);
    }
    R.iterations = it;
    R.energy = E;
  }
  if (!R.converged) {
    // leave the last orbitals in place
    if (closed) { diagonalize(Fa, X, eps_a, Ca); eps_b = eps_a; Cb = Ca; }
  }

  R.eps_a = eps_a; R.eps_b = eps_b; R.C_a = Ca; R.C_b = Cb;
  R.D_a = density(Ca, na);
  R.D_b = density(Cb, nb);
  R.occ_a = Vec::Zero(n); R.occ_b = Vec::Zero(n);
  for (int i = 0; i < na; ++i) R.occ_a(i) = 1;
  for (int i = 0; i < nb; ++i) R.occ_b(i) = 1;

  // Mulliken net charges, dipole, <S^2>
  const Mat PS = (R.D_a + R.D_b) * S;
  R.mulliken.assign(mol.atoms.size(), 0.0);
  for (size_t a = 0; a < mol.atoms.size(); ++a) R.mulliken[a] = mol.atoms[a].Z;
  for (const Shell& s : basis.shells())
    for (int k = 0; k < s.nao; ++k) R.mulliken[s.atom] -= PS(s.ao0 + k, s.ao0 + k);
  for (int c = 0; c < 3; ++c) {
    double nuc = 0;
    for (const auto& a : mol.atoms) nuc += a.Z * a.x[c];
    R.dipole[c] = nuc - ((R.D_a + R.D_b).array() * oe.dip[c].array()).sum();
  }
  const double sz = 0.5 * (na - nb);
  R.s2 = sz * (sz + 1);
  if (ref == Reference::Unrestricted && nb > 0 && na > 0) {
    Mat O = R.C_a.leftCols(na).transpose() * S * R.C_b.leftCols(nb);
    R.s2 = sz * (sz + 1) + nb - O.squaredNorm();
  }
  return R;
}

}  // namespace qm
