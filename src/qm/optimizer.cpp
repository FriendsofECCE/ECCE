#include "optimizer.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace qm {

void OptOptions::set_convergence(const std::string& name) {
  if (name == "normal") { max_grad = 3e-4; rms_grad = 1e-4; max_step = 4e-3; rms_step = 2e-3; d_energy = 5e-6; }
  else if (name == "tight") { max_grad = 1e-4; rms_grad = 3e-5; max_step = 1e-3; rms_step = 6e-4; d_energy = 1e-6; }
  else throw Error("opt_conv must be normal or tight, not '" + name + "'");
}

namespace {

using Pos = std::vector<std::array<double, 3>>;

Vec coords(const Molecule& m) {
  Vec x(3 * m.atoms.size());
  for (size_t a = 0; a < m.atoms.size(); ++a)
    for (int k = 0; k < 3; ++k) x(3 * a + k) = m.atoms[a].x[k];
  return x;
}

void set_coords(Molecule& m, const Vec& x) {
  for (size_t a = 0; a < m.atoms.size(); ++a)
    for (int k = 0; k < 3; ++k) m.atoms[a].x[k] = x(3 * a + k);
}

// Orthonormal basis (columns) of the space orthogonal to rigid translation
// and rotation.
Mat vibrational_space(const Vec& x) {
  const int N = static_cast<int>(x.size() / 3), n = 3 * N;
  double c[3] = {0, 0, 0};
  for (int a = 0; a < N; ++a)
    for (int k = 0; k < 3; ++k) c[k] += x(3 * a + k) / N;
  std::vector<Vec> tr;
  for (int k = 0; k < 3; ++k) {
    Vec t = Vec::Zero(n);
    for (int a = 0; a < N; ++a) t(3 * a + k) = 1;
    tr.push_back(t / std::sqrt(static_cast<double>(N)));
  }
  for (int k = 0; k < 3; ++k) {
    Vec t = Vec::Zero(n);
    const int i = (k + 1) % 3, j = (k + 2) % 3;
    for (int a = 0; a < N; ++a) {
      t(3 * a + j) = x(3 * a + i) - c[i];   // e_k x r
      t(3 * a + i) = -(x(3 * a + j) - c[j]);
    }
    // a rotation that moves nothing (axis along a linear molecule) is not a mode
    if (t.squaredNorm() > 1e-4) tr.push_back(t / t.norm());
  }
  Mat M = Mat::Zero(n, n);
  for (const Vec& t : tr) M += t * t.transpose();
  Eigen::SelfAdjointEigenSolver<Mat> es(M);
  int nv = 0;
  for (int i = 0; i < n; ++i)
    if (es.eigenvalues()(i) < 1e-6) ++nv;
  // eigenvalues ascending: the null space is the first nv columns
  return es.eigenvectors().leftCols(nv);
}

// Lindh's parameters by period (1, 2, 3 and beyond).
int period(int Z) { return Z <= 2 ? 0 : (Z <= 10 ? 1 : 2); }

double rho(const Molecule& m, int A, int B) {
  static const double alpha[3][3] = {{1.0, 0.3949, 0.3949}, {0.3949, 0.28, 0.28}, {0.3949, 0.28, 0.28}};
  static const double rref[3][3] = {{1.35, 2.10, 2.53}, {2.10, 2.87, 3.40}, {2.53, 3.40, 3.40}};
  const int p = period(m.atoms[A].Z), q = period(m.atoms[B].Z);
  double r2 = 0;
  for (int k = 0; k < 3; ++k) {
    const double d = m.atoms[A].x[k] - m.atoms[B].x[k];
    r2 += d * d;
  }
  return std::exp(alpha[p][q] * (rref[p][q] * rref[p][q] - r2));
}

double distance(const Vec& x, const int* at) {
  return (x.segment<3>(3 * at[0]) - x.segment<3>(3 * at[1])).norm();
}
double angle(const Vec& x, const int* at) {
  const Eigen::Vector3d u = x.segment<3>(3 * at[0]) - x.segment<3>(3 * at[1]);
  const Eigen::Vector3d v = x.segment<3>(3 * at[2]) - x.segment<3>(3 * at[1]);
  return std::atan2(u.cross(v).norm(), u.dot(v));
}
double dihedral(const Vec& x, const int* at) {
  const Eigen::Vector3d b1 = x.segment<3>(3 * at[1]) - x.segment<3>(3 * at[0]);
  const Eigen::Vector3d b2 = x.segment<3>(3 * at[2]) - x.segment<3>(3 * at[1]);
  const Eigen::Vector3d b3 = x.segment<3>(3 * at[3]) - x.segment<3>(3 * at[2]);
  const Eigen::Vector3d n1 = b1.cross(b2), n2 = b2.cross(b3);
  return std::atan2(b2.norm() * b1.dot(n2), n1.dot(n2));
}

// d(coordinate)/dx by central differences over the atoms involved.
Vec b_row(const Vec& x0, double (*f)(const Vec&, const int*), const int* at, int nat) {
  Vec b = Vec::Zero(x0.size()), x = x0;
  const double h = 1e-5;
  for (int i = 0; i < nat; ++i)
    for (int k = 0; k < 3; ++k) {
      const int j = 3 * at[i] + k;
      x(j) = x0(j) + h;
      const double fp = f(x, at);
      x(j) = x0(j) - h;
      const double fm = f(x, at);
      x(j) = x0(j);
      double d = fp - fm;
      if (d > kPi) d -= 2 * kPi;       // dihedral wrap
      if (d < -kPi) d += 2 * kPi;
      b(j) = d / (2 * h);
    }
  return b;
}

}  // namespace

Mat lindh_hessian(const Molecule& mol) {
  const int N = static_cast<int>(mol.atoms.size());
  const Vec x = coords(mol);
  Mat H = Mat::Zero(3 * N, 3 * N);
  const double kr = 0.45, kphi = 0.15, ktau = 0.005, cut = 0.02;
  std::vector<std::vector<double>> w(N, std::vector<double>(N, 0.0));
  for (int a = 0; a < N; ++a)
    for (int b = 0; b < N; ++b)
      if (a != b) w[a][b] = rho(mol, a, b);
  for (int a = 0; a < N; ++a)
    for (int b = a + 1; b < N; ++b) {
      const int at[2] = {a, b};
      const Vec r = b_row(x, distance, at, 2);
      H += kr * w[a][b] * r * r.transpose();
    }
  for (int b = 0; b < N; ++b)
    for (int a = 0; a < N; ++a)
      for (int c = a + 1; c < N; ++c) {
        if (a == b || c == b) continue;
        const double wt = w[a][b] * w[b][c];
        if (wt < cut) continue;
        const int at[3] = {a, b, c};
        const double th = angle(x, at);
        if (std::sin(th) < 0.05) continue;  // linear: the angle has no gradient
        const Vec r = b_row(x, angle, at, 3);
        H += kphi * wt * r * r.transpose();
      }
  for (int b = 0; b < N; ++b)
    for (int c = 0; c < N; ++c) {
      if (b >= c) continue;
      for (int a = 0; a < N; ++a)
        for (int d = 0; d < N; ++d) {
          if (a == b || a == c || d == b || d == c || a == d) continue;
          const double wt = w[a][b] * w[b][c] * w[c][d];
          if (wt < cut) continue;
          const int at[4] = {a, b, c, d};
          if (std::sin(angle(x, at)) < 0.05 || std::sin(angle(x, at + 1)) < 0.05) continue;
          const Vec r = b_row(x, dihedral, at, 4);
          H += ktau * wt * r * r.transpose();
        }
    }
  return H;
}

OptResult optimize_geometry(const Molecule& start, const OptOptions& opt,
                            const std::function<OptEval(const Molecule&)>& eval,
                            const std::function<void(const OptStep&)>& on_step) {
  const int n = 3 * static_cast<int>(start.atoms.size());
  Molecule mol = start;
  Vec x = coords(mol);
  OptResult res;

  // Model Hessian, made positive definite in the vibrational space.
  Mat H;
  {
    const Mat Q = vibrational_space(x);
    Eigen::SelfAdjointEigenSolver<Mat> es(Q.transpose() * lindh_hessian(mol) * Q);
    Vec ev = es.eigenvalues().cwiseMax(0.02).cwiseMin(1.5);
    H = Q * es.eigenvectors() * ev.asDiagonal() * es.eigenvectors().transpose() * Q.transpose();
  }

  OptEval cur = eval(mol);
  res.evaluations = 1;
  double trust = opt.trust, last_energy = 0;
  bool have_prev = false, reported = false;
  Vec g = Eigen::Map<const Vec>(cur.grad.data(), n);

  for (;;) {
    // Step from the current point: RFO in the vibrational space.
    const Mat Q = vibrational_space(x);
    const Mat Hq = Q.transpose() * H * Q;
    const Vec gq = Q.transpose() * g;
    const int m = static_cast<int>(gq.size());
    Vec s = Vec::Zero(n);
    if (m > 0) {
      Mat A = Mat::Zero(m + 1, m + 1);
      A.topLeftCorner(m, m) = Hq;
      A.topRightCorner(m, 1) = gq;
      A.bottomLeftCorner(1, m) = gq.transpose();
      Eigen::SelfAdjointEigenSolver<Mat> es(A);
      const Vec v = es.eigenvectors().col(0);
      Vec sq = (std::fabs(v(m)) > 1e-10) ? Vec(v.head(m) / v(m)) : Vec(-Hq.inverse() * gq);
      if (sq.norm() > trust) sq *= trust / sq.norm();
      s = Q * sq;
    }

    OptStep st;
    st.index = res.evaluations;
    st.mol = mol;
    st.energy = cur.energy;
    st.grad = cur.grad;
    st.max_grad = g.cwiseAbs().maxCoeff();
    st.rms_grad = std::sqrt(g.squaredNorm() / n);
    st.max_step = s.cwiseAbs().maxCoeff();
    st.rms_step = std::sqrt(s.squaredNorm() / n);
    st.d_energy = have_prev ? cur.energy - last_energy : 0.0;
    const bool conv = st.max_grad <= opt.max_grad && st.rms_grad <= opt.rms_grad && st.max_step <= opt.max_step &&
                      st.rms_step <= opt.rms_step && (!have_prev || std::fabs(st.d_energy) <= opt.d_energy);
    if (!reported) { if (on_step) on_step(st); reported = true; }
    res.mol = mol;
    res.grad = cur.grad;
    res.energy = cur.energy;
    if (conv) { res.converged = true; break; }
    if (res.evaluations >= opt.max_steps) break;

    // Try the step.
    Molecule trial = mol;
    const Vec xt = x + s;
    set_coords(trial, xt);
    OptEval ev = eval(trial);
    ++res.evaluations;
    const Vec gt = Eigen::Map<const Vec>(ev.grad.data(), n);
    const double dE = ev.energy - cur.energy, pred = g.dot(s) + 0.5 * s.dot(H * s);

    // BFGS update with the rigid-body part removed.
    {
      const Mat P = Q * Q.transpose();
      const Vec sp = P * s, yp = P * (gt - g);
      const double ys = yp.dot(sp);
      const Vec Hs = H * sp;
      const double sHs = sp.dot(Hs);
      if (ys > 1e-8 * sp.norm() * yp.norm() && sHs > 0) H += yp * yp.transpose() / ys - Hs * Hs.transpose() / sHs;
    }

    if (dE > 1e-8 && trust > 1e-3) {   // the energy went up: retry from the same point, shorter
      trust = std::max(0.5 * s.norm(), 1e-3);
      continue;
    }
    const double ratio = pred < -1e-12 ? dE / pred : 1.0;
    if (ratio > 0.75 && s.norm() > 0.8 * trust) trust = std::min(2 * trust, opt.trust_max);
    else if (ratio < 0.25) trust = std::max(0.5 * s.norm(), 1e-3);
    last_energy = cur.energy;
    have_prev = true;
    reported = false;
    mol = trial;
    x = xt;
    cur = ev;
    g = gt;
  }
  return res;
}

}  // namespace qm
