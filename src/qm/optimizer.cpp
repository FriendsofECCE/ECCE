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

// ---- constraints ----------------------------------------------------------

double con_value(const GeoConstraint& c, const Vec& x) {
  switch (c.kind) {
    case GeoConstraint::Bond: return distance(x, c.atoms);
    case GeoConstraint::Angle: return angle(x, c.atoms);
    default: return dihedral(x, c.atoms);
  }
}

double con_residual(const GeoConstraint& c, const Vec& x) {
  double r = con_value(c, x) - c.value;
  if (c.kind == GeoConstraint::Torsion) {
    while (r > kPi) r -= 2 * kPi;
    while (r < -kPi) r += 2 * kPi;
  }
  return r;
}

// Dummy atoms that no bond, angle or torsion names are fixed in space.
std::vector<bool> fixed_atoms(const Molecule& m, const std::vector<GeoConstraint>& cons) {
  std::vector<bool> fix(m.atoms.size(), false);
  std::vector<bool> named(m.atoms.size(), false);
  for (const auto& c : cons) {
    if (c.kind == GeoConstraint::Fix) fix[c.atoms[0]] = true;
    else
      for (int i = 0; i < (c.kind == GeoConstraint::Bond ? 2 : c.kind == GeoConstraint::Angle ? 3 : 4); ++i)
        named[c.atoms[i]] = true;
  }
  for (size_t a = 0; a < m.atoms.size(); ++a)
    if (m.atoms[a].Z == 0 && !named[a]) fix[a] = true;
  return fix;
}

// Rows: the gradient of every bond/angle/torsion constraint, and a unit row
// for each Cartesian component of a fixed atom.
Mat constraint_rows(const Vec& x, const std::vector<GeoConstraint>& cons, const std::vector<bool>& fix) {
  std::vector<Vec> rows;
  for (const auto& c : cons) {
    if (c.kind == GeoConstraint::Fix) continue;
    double (*f)(const Vec&, const int*) = c.kind == GeoConstraint::Bond ? distance : c.kind == GeoConstraint::Angle ? angle : dihedral;
    rows.push_back(b_row(x, f, c.atoms, c.kind == GeoConstraint::Bond ? 2 : c.kind == GeoConstraint::Angle ? 3 : 4));
  }
  for (size_t a = 0; a < fix.size(); ++a)
    if (fix[a])
      for (int k = 0; k < 3; ++k) {
        Vec e = Vec::Zero(x.size());
        e(3 * a + k) = 1;
        rows.push_back(e);
      }
  Mat B(rows.size(), x.size());
  for (size_t i = 0; i < rows.size(); ++i) B.row(i) = rows[i].transpose();
  return B;
}

// Orthonormal basis of the steps that keep every constraint (to first
// order); with no fixed atom also free of translation and rotation.
Mat constrained_space(const Vec& x, const Mat& B, bool with_rigid) {
  const int n = static_cast<int>(x.size());
  Mat M = Mat::Zero(n, n);
  for (int i = 0; i < B.rows(); ++i) {
    const double nr = B.row(i).norm();
    if (nr > 1e-12) M += B.row(i).transpose() * B.row(i) / (nr * nr);
  }
  if (with_rigid) {
    const Mat Qv = vibrational_space(x);
    M += Mat::Identity(n, n) - Qv * Qv.transpose();
  }
  Eigen::SelfAdjointEigenSolver<Mat> es(M);
  int nv = 0;
  for (int i = 0; i < n; ++i)
    if (es.eigenvalues()(i) < 1e-6) ++nv;
  return es.eigenvectors().leftCols(nv);
}

// Newton iterations (minimum-norm corrections) back onto the constraint values.
void restore_constraints(Vec& x, const std::vector<GeoConstraint>& cons, const std::vector<bool>& fix) {
  for (int it = 0; it < 200; ++it) {
    std::vector<double> res;
    for (const auto& c : cons)
      if (c.kind != GeoConstraint::Fix) res.push_back(con_residual(c, x));
    double worst = 0;
    for (double v : res) worst = std::max(worst, std::fabs(v));
    if (worst < 1e-11) return;
    const Mat B = constraint_rows(x, cons, fix);
    Vec rhs = Vec::Zero(B.rows());
    for (size_t i = 0; i < res.size(); ++i) rhs(i) = -res[i];
    Vec d = B.completeOrthogonalDecomposition().solve(rhs);
    if (d.norm() > 0.5) d *= 0.5 / d.norm();
    x += d;
  }
  throw Error("the geometry constraints cannot be satisfied together");
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
      if (a != b && mol.atoms[a].Z != 0 && mol.atoms[b].Z != 0) w[a][b] = rho(mol, a, b);
  for (int a = 0; a < N; ++a)
    for (int b = a + 1; b < N; ++b) {
      if (w[a][b] == 0.0) continue;   // a dummy atom has no bonds
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

  // Constraints without a value keep the starting one; the others are
  // imposed on the starting geometry before the first energy.
  std::vector<GeoConstraint> cons = opt.constraints;
  for (auto& c : cons)
    if (c.kind != GeoConstraint::Fix && !c.has_value) c.value = con_value(c, x);
  const std::vector<bool> fix = fixed_atoms(mol, cons);
  const bool constrained = !cons.empty() || std::count(fix.begin(), fix.end(), true) > 0;
  const bool any_fixed = std::count(fix.begin(), fix.end(), true) > 0;
  if (constrained) {
    restore_constraints(x, cons, fix);
    set_coords(mol, x);
  }
  // Space of allowed steps at x.
  auto step_space = [&](const Vec& xx) {
    return constrained ? constrained_space(xx, constraint_rows(xx, cons, fix), !any_fixed) : vibrational_space(xx);
  };

  // Model Hessian, made positive definite in the vibrational space.
  Mat H = Mat::Zero(n, n);
  if (step_space(x).cols() > 0) {   // a diatomic with its bond held has no free coordinate
    const Mat Q = step_space(x);
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
    const Mat Q = step_space(x);
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
    // With constraints the part of the gradient the constraints absorb is no
    // sign of a missed minimum.
    const Vec gc = constrained ? Vec(Q * (Q.transpose() * g)) : g;
    st.max_grad = gc.cwiseAbs().maxCoeff();
    st.rms_grad = std::sqrt(gc.squaredNorm() / n);
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
    Vec xt = x + s;
    if (constrained) {
      restore_constraints(xt, cons, fix);
      s = xt - x;
    }
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

OptResult optimize_ts(const Molecule& start, const OptOptions& opt,
                      const std::function<OptEval(const Molecule&)>& eval,
                      const std::function<Mat(const Molecule&)>& hessian,
                      const std::function<void(const OptStep&)>& on_step) {
  const int n = 3 * static_cast<int>(start.atoms.size());
  Molecule mol = start;
  Vec x = coords(mol);
  OptResult res;
  OptEval cur = eval(mol);
  res.evaluations = 1;
  Mat H = hessian(mol);
  H = 0.5 * (H + H.transpose());
  Vec g = Eigen::Map<const Vec>(cur.grad.data(), n);
  double trust = opt.trust, last_energy = 0;
  bool have_prev = false;
  Vec mode_prev;   // Cartesian vector of the mode followed at the previous step
  int since_hessian = 0;

  for (;;) {
    const Mat Q = vibrational_space(x);
    const int m = static_cast<int>(Q.cols());
    Vec s = Vec::Zero(n);
    Vec mode_now;
    if (m > 0) {
      Eigen::SelfAdjointEigenSolver<Mat> es(Q.transpose() * H * Q);
      const Vec e = es.eigenvalues();
      const Mat V = es.eigenvectors();
      const Vec F = V.transpose() * (Q.transpose() * g);
      // The walk must not hop between modes: follow the one that looks most
      // like the previous step's.
      int k = 0;
      if (mode_prev.size() > 0) {
        double best = -1;
        for (int i = 0; i < m; ++i) {
          const double ov = std::fabs(mode_prev.dot(Q * V.col(i)));
          if (ov > best) { best = ov; k = i; }
        }
      }
      mode_now = Q * V.col(k);
      Vec sq = Vec::Zero(m);
      const double lp = 0.5 * e(k) + 0.5 * std::sqrt(e(k) * e(k) + 4 * F(k) * F(k));
      if (std::fabs(e(k) - lp) > 1e-12) sq(k) = -F(k) / (e(k) - lp);
      // Downhill along the others: lowest root of their augmented Hessian.
      if (m > 1) {
        Mat A = Mat::Zero(m, m);
        Vec ev(m - 1), Fo(m - 1);
        for (int i = 0, j = 0; i < m; ++i)
          if (i != k) { ev(j) = e(i); Fo(j) = F(i); ++j; }
        A.topLeftCorner(m - 1, m - 1) = ev.asDiagonal();
        A.topRightCorner(m - 1, 1) = Fo;
        A.bottomLeftCorner(1, m - 1) = Fo.transpose();
        const double ln = Eigen::SelfAdjointEigenSolver<Mat>(A).eigenvalues()(0);
        for (int i = 0; i < m; ++i)
          if (i != k && std::fabs(e(i) - ln) > 1e-12) sq(i) = -F(i) / (e(i) - ln);
      }
      Vec sc = V * sq;
      if (sc.norm() > trust) sc *= trust / sc.norm();
      s = Q * sc;
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
    if (on_step) on_step(st);
    res.mol = mol;
    res.grad = cur.grad;
    res.energy = cur.energy;
    if (conv) { res.converged = true; break; }
    if (res.evaluations >= opt.max_steps) break;

    Molecule trial = mol;
    const Vec xt = x + s;
    set_coords(trial, xt);
    OptEval ev = eval(trial);
    ++res.evaluations;
    const Vec gt = Eigen::Map<const Vec>(ev.grad.data(), n);
    const double dE = ev.energy - cur.energy, pred = g.dot(s) + 0.5 * s.dot(H * s);

    if (opt.ts_hessian_every > 0 && ++since_hessian >= opt.ts_hessian_every) {
      H = hessian(trial);
      H = 0.5 * (H + H.transpose());
      since_hessian = 0;
    } else {
      // Bofill: Murtagh-Sargent where it is accurate, Powell-symmetric-Broyden
      // otherwise; neither forces positive curvature, which a saddle point needs.
      const Mat P = Q * Q.transpose();
      const Vec sp = P * s, xi = P * (gt - g) - H * sp;
      const double ss = sp.squaredNorm(), xs = xi.dot(sp), xx = xi.squaredNorm();
      if (ss > 1e-14 && xx > 1e-20) {
        const double phi = xs * xs / (xx * ss);
        if (std::fabs(xs) > 1e-14) H += phi * (xi * xi.transpose() / xs);
        H += (1 - phi) * ((xi * sp.transpose() + sp * xi.transpose()) / ss - xs * (sp * sp.transpose()) / (ss * ss));
      }
    }

    // The energy has no direction here, so the model is judged by how well it
    // predicted the change, and every step is taken.
    const double ratio = std::fabs(pred) > 1e-12 ? dE / pred : 1.0;
    if (ratio < 0.25 || ratio > 4.0) trust = std::max(0.5 * s.norm(), 1e-2);
    else if (ratio > 0.75 && ratio < 1.33 && s.norm() > 0.8 * trust) trust = std::min(2 * trust, opt.trust_max);
    last_energy = cur.energy;
    have_prev = true;
    mode_prev = mode_now;
    mol = trial;
    x = xt;
    cur = ev;
    g = gt;
  }
  return res;
}

}  // namespace qm
