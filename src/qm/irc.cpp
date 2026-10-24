#include "irc.hpp"
#include <Eigen/Eigenvalues>
#include <algorithm>
#include <cmath>
#include "vibration.hpp"

namespace qm {
namespace {

Vec weighted_coords(const Molecule& m, const Vec& sm) {
  Vec y(3 * m.atoms.size());
  for (size_t a = 0; a < m.atoms.size(); ++a)
    for (int k = 0; k < 3; ++k) y(3 * a + k) = sm(3 * a + k) * m.atoms[a].x[k];
  return y;
}

// Orthonormal basis of the mass-weighted space without translation and rotation.
Mat internal_space(const Molecule& m, const Vec& sm) {
  const int N = static_cast<int>(m.atoms.size()), n = 3 * N;
  double tot = 0, c[3] = {0, 0, 0};
  for (int a = 0; a < N; ++a) {
    const double w = sm(3 * a) * sm(3 * a);
    tot += w;
    for (int k = 0; k < 3; ++k) c[k] += w * m.atoms[a].x[k];
  }
  for (double& v : c) v /= tot;
  std::vector<Vec> ext;
  for (int k = 0; k < 3; ++k) {
    Vec t = Vec::Zero(n);
    for (int a = 0; a < N; ++a) t(3 * a + k) = sm(3 * a + k);
    ext.push_back(t / t.norm());
  }
  for (int k = 0; k < 3; ++k) {
    Vec t = Vec::Zero(n);
    const int i = (k + 1) % 3, j = (k + 2) % 3;
    for (int a = 0; a < N; ++a) {
      t(3 * a + j) = sm(3 * a + j) * (m.atoms[a].x[i] - c[i]);
      t(3 * a + i) = -sm(3 * a + i) * (m.atoms[a].x[j] - c[j]);
    }
    if (t.squaredNorm() > 1e-4) ext.push_back(t / t.norm());
  }
  Mat M = Mat::Zero(n, n);
  for (const Vec& t : ext) M += t * t.transpose();
  Eigen::SelfAdjointEigenSolver<Mat> es(M);
  int nv = 0;
  for (int i = 0; i < n; ++i)
    if (es.eigenvalues()(i) < 1e-6) ++nv;
  return es.eigenvectors().leftCols(nv);
}

Mat weight_hessian(const Mat& H, const Vec& sm) {
  Mat Hy = H;
  for (int i = 0; i < H.rows(); ++i)
    for (int j = 0; j < H.cols(); ++j) Hy(i, j) /= sm(i) * sm(j);
  return 0.5 * (Hy + Hy.transpose());
}

// Step to the minimum on the sphere of the given radius about the pivot (D is
// the current offset from it): the multiplier lies below the lowest eigenvalue.
Vec sphere_step(const Mat& Hr, const Vec& gr, const Vec& D, double radius) {
  Eigen::SelfAdjointEigenSolver<Mat> es(Hr);
  const Vec e = es.eigenvalues();
  const Vec gt = es.eigenvectors().transpose() * gr, Dt = es.eigenvectors().transpose() * D;
  auto len2 = [&](double lam) {
    double s = 0;
    for (int i = 0; i < e.size(); ++i) {
      const double v = (Dt(i) * e(i) - gt(i)) / (e(i) - lam);
      s += v * v;
    }
    return s;
  };
  double hi = e(0) - 1e-10, lo = e(0) - 1.0;
  for (int i = 0; i < 200 && len2(lo) > radius * radius; ++i) lo = e(0) - 2 * (e(0) - lo);
  for (int i = 0; i < 200; ++i) {
    const double mid = 0.5 * (lo + hi);
    (len2(mid) > radius * radius ? hi : lo) = mid;
  }
  const double lam = 0.5 * (lo + hi);
  Vec d(e.size());
  for (int i = 0; i < e.size(); ++i) d(i) = (-gt(i) + lam * Dt(i)) / (e(i) - lam);
  Vec step = es.eigenvectors() * d;
  if (step.norm() > radius) step *= radius / step.norm();
  return step;
}

void bofill(Mat& H, const Vec& s, const Vec& dg) {
  const Vec xi = dg - H * s;
  const double ss = s.squaredNorm(), xs = xi.dot(s), xx = xi.squaredNorm();
  if (ss < 1e-14 || xx < 1e-20) return;
  const double phi = xs * xs / (xx * ss);
  if (std::fabs(xs) > 1e-14) H += phi * (xi * xi.transpose() / xs);
  H += (1 - phi) * ((xi * s.transpose() + s * xi.transpose()) / ss - xs * (s * s.transpose()) / (ss * ss));
}

}  // namespace

std::vector<IrcPoint> irc_path(const Molecule& ts, const Mat& Hcart, int sign, const IrcOptions& opt,
                               const std::function<OptEval(const Molecule&)>& eval,
                               const std::function<Mat(const Molecule&)>& hessian,
                               const std::function<void(const IrcPoint&)>& on_point) {
  const int N = static_cast<int>(ts.atoms.size()), n = 3 * N;
  Vec sm(n);
  for (int a = 0; a < N; ++a)
    for (int k = 0; k < 3; ++k) sm(3 * a + k) = std::sqrt(isotope_mass(ts.atoms[a].Z));
  const Mat B = internal_space(ts, sm);
  const Mat P = B * B.transpose();

  Mat H = P * weight_hessian(Hcart, sm) * P;
  Eigen::SelfAdjointEigenSolver<Mat> es(B.transpose() * H * B);
  if (es.eigenvalues()(0) > -1e-5) throw Error("irc: the Hessian at the start geometry has no imaginary mode");
  Vec mode = B * es.eigenvectors().col(0);
  // The sign of an eigenvector is arbitrary: "forward" is the way the atom
  // that moves most goes in the positive direction.
  int big = 0;
  for (int i = 1; i < n; ++i)
    if (std::fabs(mode(i)) > std::fabs(mode(big))) big = i;
  if (mode(big) < 0) mode = -mode;
  Vec dir = sign * mode;

  auto to_molecule = [&](const Vec& y) {
    Molecule m = ts;
    for (int a = 0; a < N; ++a)
      for (int k = 0; k < 3; ++k) m.atoms[a].x[k] = y(3 * a + k) / sm(3 * a + k);
    return m;
  };
  auto mw_grad = [&](const OptEval& e) {
    Vec g(n);
    for (int i = 0; i < n; ++i) g(i) = e.grad[i] / sm(i);
    const Vec out = P * g;
    return out;
  };
  auto cart_norms = [&](const OptEval& e, double& rms, double& mx) {
    mx = 0;
    double s2 = 0;
    for (double v : e.grad) { mx = std::max(mx, std::fabs(v)); s2 += v * v; }
    rms = std::sqrt(s2 / n);
  };

  std::vector<IrcPoint> path;
  Vec y = weighted_coords(ts, sm);
  OptEval cur = eval(ts);
  Vec g = mw_grad(cur);
  double s_total = 0;
  const double half = 0.5 * opt.step;

  for (int k = 0; k < opt.max_points; ++k) {
    if (k > 0 && opt.hessian_every > 0 && k % opt.hessian_every == 0)
      H = P * weight_hessian(hessian(to_molecule(y)), sm) * P;
    if (k > 0) dir = -g / g.norm();
    const Vec pivot = y + half * dir;
    Vec q = y + opt.step * dir;
    OptEval eq = eval(to_molecule(q));
    Vec gq = mw_grad(eq);
    for (int it = 0;; ++it) {
      const Vec D = q - pivot;
      const Vec u = D / D.norm();
      const Vec gperp = gq - gq.dot(u) * u;
      double rms = 0, mx = 0;
      {
        // Cartesian size of the part of the gradient across the sphere.
        Vec gc(n);
        for (int i = 0; i < n; ++i) gc(i) = gperp(i) * sm(i);
        mx = gc.cwiseAbs().maxCoeff();
        rms = std::sqrt(gc.squaredNorm() / n);
      }
      if ((rms <= opt.tol_rms && mx <= opt.tol_max) || it >= opt.max_micro) break;
      const Vec dq = B * sphere_step(B.transpose() * H * B, B.transpose() * gq, B.transpose() * D, half);
      const Vec qn = q + dq;
      OptEval en = eval(to_molecule(qn));
      const Vec gn = mw_grad(en);
      bofill(H, dq, gn - gq);
      H = P * H * P;
      q = qn; eq = en; gq = gn;
    }
    if (!path.empty() && eq.energy > path.back().energy + 1e-9) break;  // overshot the valley
    if (path.empty() && eq.energy > cur.energy + 1e-9) throw Error("irc: the first step goes uphill; is the start a transition state?");
    s_total += sign * (q - y).norm();
    IrcPoint pt;
    pt.mol = to_molecule(q);
    pt.energy = eq.energy;
    pt.s = s_total;
    cart_norms(eq, pt.rms_grad, pt.max_grad);
    path.push_back(pt);
    if (on_point) on_point(pt);
    y = q; cur = eq; g = gq;
    if (pt.rms_grad <= opt.rms_grad && pt.max_grad <= opt.max_grad) break;
  }
  return path;
}

}  // namespace qm

