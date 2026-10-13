#include "vibration.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#ifdef _OPENMP
#include <omp.h>
#endif

namespace qm {

namespace {
// CODATA 2018
constexpr double kEh = 4.3597447222071e-18;        // J
constexpr double kBohr = 0.529177210903e-10;       // m
constexpr double kAmu = 1.66053906660e-27;         // kg
constexpr double kC = 299792458.0;                 // m/s
constexpr double kNA = 6.02214076e23;
constexpr double kEps0 = 8.8541878128e-12;
constexpr double kElemCharge = 1.602176634e-19;
constexpr double kBoltz = 1.380649e-23;            // J/K
constexpr double kPlanck = 6.62607015e-34;         // J s
constexpr double kAtm = 101325.0;                  // Pa
constexpr double kBohrA = 0.529177210903;          // bohr in angstrom
}  // namespace

double isotope_mass(int Z) {
  static const double m[] = {0, 1.00782503207, 4.00260325415, 7.016004548, 9.012182201, 11.009305406, 12.0,
                             14.00307400478, 15.99491461956, 18.99840322, 19.9924401754, 22.9897692809,
                             23.985041700, 26.98153863, 27.9769265325, 30.97376163, 31.97207100, 34.96885268,
                             39.9623831225, 38.96370668, 39.96259098, 44.9559119, 47.9479463, 50.9439595,
                             51.9405075, 54.9380451, 55.9349375, 58.9331950, 57.9353429, 62.9295975,
                             63.9291422, 68.9255736, 73.9211778, 74.9215965, 79.9165213, 78.9183371, 83.911507};
  if (Z < 1 || Z >= static_cast<int>(sizeof(m) / sizeof(m[0]))) throw Error("no isotope mass for Z = " + std::to_string(Z));
  return m[Z];
}

Concurrency plan_concurrency(int npoints, double bytes_per_point) {
  Concurrency c;
#ifdef _OPENMP
  if (std::getenv("ECCE_QM_SERIAL")) return c;
  std::ifstream mi("/proc/meminfo");
  std::string key;
  double avail_kb = 0;
  while (mi >> key) {
    if (key == "MemAvailable:") { mi >> avail_kb; break; }
    mi.ignore(256, '\n');
  }
  if (avail_kb <= 0) return c;  // unknown memory: stay serial
  // Keep a quarter of the available memory free for the system and for the
  // grids and matrices that come on top of the integrals.
  const double fit = 0.75 * avail_kb * 1024.0 / std::max(bytes_per_point, 1.0);
  const int budget = std::max(1, omp_get_max_threads());
  const int p = static_cast<int>(std::min<double>({static_cast<double>(budget), static_cast<double>(npoints), fit}));
  if (p < 2) return c;
  c.workers = p;
  c.threads = std::max(1, budget / p);
#else
  (void)npoints; (void)bytes_per_point;
#endif
  return c;
}

Derivatives finite_difference_derivatives(const Molecule& mol, double step,
                                          const std::function<PointEval(const Molecule&)>& eval,
                                          const std::function<void(int, int)>& on_point, int workers,
                                          int threads_per_worker) {
  const int n = 3 * static_cast<int>(mol.atoms.size());
  Derivatives d;
  d.hessian = Mat::Zero(n, n);
  d.dmu = Mat::Zero(3, n);
  d.dalpha = Mat::Zero(9, n);
  bool have_alpha = false;

  // Point 2*i + s is coordinate i displaced by +step (s = 0) or -step.
  std::vector<PointEval> pts(2 * n);
  auto run_point = [&](int k) {
    Molecule m = mol;
    m.atoms[(k / 2) / 3].x[(k / 2) % 3] += (k % 2 == 0 ? step : -step);
    pts[k] = eval(m);
  };
  if (workers <= 1) {
    for (int k = 0; k < 2 * n; ++k) {
      run_point(k);
      if (on_point) on_point(k + 1, 2 * n);
    }
  } else {
    std::atomic<int> next(0);
    std::mutex mu;
    std::exception_ptr err;
    int done = 0;
    auto work = [&]() {
#ifdef _OPENMP
      // The thread count is per thread, so each worker's parallel regions get
      // their own team of this size and nothing nests.
      if (threads_per_worker > 0) omp_set_num_threads(threads_per_worker);
#else
      (void)threads_per_worker;
#endif
      for (;;) {
        const int k = next.fetch_add(1);
        if (k >= 2 * n) return;
        try {
          run_point(k);
        } catch (...) {
          std::lock_guard<std::mutex> lk(mu);
          if (!err) err = std::current_exception();
          next = 2 * n;
          return;
        }
        std::lock_guard<std::mutex> lk(mu);
        if (on_point) on_point(++done, 2 * n);
      }
    };
    std::vector<std::thread> pool;
    for (int w = 0; w < workers; ++w) pool.emplace_back(work);
    for (std::thread& t : pool) t.join();
    if (err) std::rethrow_exception(err);
  }

  for (int i = 0; i < n; ++i) {
    const PointEval* pe = &pts[2 * i];
    for (int j = 0; j < n; ++j) d.hessian(j, i) = (pe[0].grad[j] - pe[1].grad[j]) / (2 * step);
    for (int c = 0; c < 3; ++c) d.dmu(c, i) = (pe[0].dipole[c] - pe[1].dipole[c]) / (2 * step);
    for (int c = 0; c < 9; ++c) {
      d.dalpha(c, i) = (pe[0].alpha[c] - pe[1].alpha[c]) / (2 * step);
      if (pe[0].alpha[c] != 0.0 || pe[1].alpha[c] != 0.0) have_alpha = true;
    }
  }
  if (!have_alpha) d.dalpha.resize(0, 0);
  d.hessian = 0.5 * (d.hessian + d.hessian.transpose()).eval();
  return d;
}

namespace {

struct Centered {
  std::vector<std::array<double, 3>> r;  // relative to the centre of mass, bohr
  std::vector<double> m;
};

Centered centre_of_mass_frame(const Molecule& mol) {
  Centered c;
  double M = 0;
  std::array<double, 3> com = {0, 0, 0};
  for (const Atom& a : mol.atoms) {
    const double m = isotope_mass(a.Z);
    c.m.push_back(m);
    M += m;
    for (int k = 0; k < 3; ++k) com[k] += m * a.x[k];
  }
  for (double& v : com) v /= M;
  for (const Atom& a : mol.atoms) c.r.push_back({a.x[0] - com[0], a.x[1] - com[1], a.x[2] - com[2]});
  return c;
}

// Orthonormal basis of the translations and rotations in mass-weighted
// Cartesians; a rotation about the axis of a linear molecule drops out.
Mat transrot_space(const Centered& c) {
  const int na = static_cast<int>(c.m.size()), n = 3 * na;
  std::vector<Vec> raw;
  for (int k = 0; k < 3; ++k) {
    Vec v = Vec::Zero(n);
    for (int a = 0; a < na; ++a) v(3 * a + k) = std::sqrt(c.m[a]);
    raw.push_back(v);
  }
  for (int k = 0; k < 3; ++k) {
    Vec v = Vec::Zero(n);
    const int i = (k + 1) % 3, j = (k + 2) % 3;
    for (int a = 0; a < na; ++a) {
      // e_k x r
      v(3 * a + i) = -c.r[a][j] * std::sqrt(c.m[a]);
      v(3 * a + j) = c.r[a][i] * std::sqrt(c.m[a]);
    }
    raw.push_back(v);
  }
  std::vector<Vec> basis;
  for (Vec v : raw) {
    const double n0 = v.norm();
    if (n0 < 1e-10) continue;
    for (int pass = 0; pass < 2; ++pass)
      for (const Vec& b : basis) v -= b.dot(v) * b;
    if (v.norm() < 1e-4 * n0) continue;
    basis.push_back(v / v.norm());
  }
  Mat V(n, static_cast<int>(basis.size()));
  for (size_t k = 0; k < basis.size(); ++k) V.col(static_cast<int>(k)) = basis[k];
  return V;
}

double to_wavenumber(double lambda) {  // eigenvalue in hartree / (bohr^2 amu)
  static const double f = std::sqrt(kEh / (kBohr * kBohr * kAmu)) / (2 * 3.14159265358979323846 * kC) / 100.0;
  return (lambda < 0 ? -1.0 : 1.0) * std::sqrt(std::fabs(lambda)) * f;
}

}  // namespace

VibResult analyse_vibrations(const Molecule& mol, const Derivatives& d) {
  const int na = static_cast<int>(mol.atoms.size()), n = 3 * na;
  VibResult res;
  const Centered c = centre_of_mass_frame(mol);
  res.masses = c.m;
  Vec w(n);
  for (int i = 0; i < n; ++i) w(i) = std::sqrt(c.m[i / 3]);
  Mat Hmw = d.hessian;
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) Hmw(i, j) /= w(i) * w(j);

  const Mat V = transrot_space(c);
  res.ntransrot = static_cast<int>(V.cols());
  {
    Eigen::SelfAdjointEigenSolver<Mat> es(Hmw);
    std::vector<double> f;
    for (int i = 0; i < n; ++i) f.push_back(to_wavenumber(es.eigenvalues()(i)));
    std::sort(f.begin(), f.end(), [](double a, double b) { return std::fabs(a) < std::fabs(b); });
    f.resize(res.ntransrot);
    res.residual = f;
  }
  const int nv = n - res.ntransrot;
  if (nv == 0) return res;

  // Complement of the translation/rotation space: the eigenvectors of the
  // projector with eigenvalue 1 come last.
  Mat P = Mat::Identity(n, n) - V * V.transpose();
  Eigen::SelfAdjointEigenSolver<Mat> ep(P);
  const Mat Q = ep.eigenvectors().rightCols(nv);
  Eigen::SelfAdjointEigenSolver<Mat> es(Q.transpose() * Hmw * Q);
  const Mat q = Q * es.eigenvectors();  // mass-weighted, orthonormal

  res.modes.resize(n, nv);
  const double conv = kElemCharge / std::sqrt(kAmu);  // e/sqrt(amu) -> C/sqrt(kg)
  for (int m = 0; m < nv; ++m) {
    res.freq.push_back(to_wavenumber(es.eigenvalues()(m)));
    if (res.freq.back() < 0) ++res.n_imaginary;
    Vec dx(n);
    for (int i = 0; i < n; ++i) dx(i) = q(i, m) / w(i);
    double a2 = 0;
    for (int comp = 0; comp < 3; ++comp) {
      double a = 0;
      for (int i = 0; i < n; ++i) a += d.dmu(comp, i) * dx(i);
      a2 += a * a;
    }
    // integrated absorption, the SI form of N_A pi / (3 c^2) |dmu/dQ|^2
    res.ir.push_back(kNA / (12.0 * kEps0 * kC * kC) * a2 * conv * conv / 1000.0);
    res.modes.col(m) = dx / dx.norm();
    if (d.dalpha.cols() == n) {
      // d(alpha)/dQ in bohr^2/sqrt(amu), then A^2/sqrt(amu)
      Eigen::Matrix3d da;
      for (int ij = 0; ij < 9; ++ij) da(ij / 3, ij % 3) = d.dalpha.row(ij).dot(dx.transpose()) * kBohrA * kBohrA;
      const double mean = da.trace() / 3.0;
      const double g2 = 0.5 * (std::pow(da(0, 0) - da(1, 1), 2) + std::pow(da(1, 1) - da(2, 2), 2) +
                               std::pow(da(2, 2) - da(0, 0), 2)) +
                        3.0 * (std::pow(0.5 * (da(0, 1) + da(1, 0)), 2) + std::pow(0.5 * (da(0, 2) + da(2, 0)), 2) +
                               std::pow(0.5 * (da(1, 2) + da(2, 1)), 2));
      res.raman.push_back(45.0 * mean * mean + 7.0 * g2);
      const double den = 45.0 * mean * mean + 4.0 * g2;
      res.depol.push_back(den > 0 ? 3.0 * g2 / den : 0.0);
    }
  }
  return res;
}

int rotational_symmetry_number(const Molecule& mol) {
  const Centered c = centre_of_mass_frame(mol);
  const int na = static_cast<int>(c.m.size());
  if (na < 2) return 1;
  const double tol = 0.02;  // bohr
  using V3 = std::array<double, 3>;
  auto norm = [](const V3& v) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); };
  auto dot = [](const V3& a, const V3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; };
  auto cross = [](const V3& a, const V3& b) {
    return V3{a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
  };
  // an atom off the centre, then the atom furthest off the line through it
  int ia = -1, ib = -1;
  double best = tol;
  for (int i = 0; i < na; ++i)
    if (norm(c.r[i]) > best) { best = norm(c.r[i]); ia = i; }
  if (ia < 0) return 1;
  best = tol;
  for (int i = 0; i < na; ++i) {
    const double off = norm(cross(c.r[ia], c.r[i])) / norm(c.r[ia]);
    if (off > best) { best = off; ib = i; }
  }
  auto has_image = [&](int i, const V3& p) {
    for (int j = 0; j < na; ++j)
      if (mol.atoms[j].Z == mol.atoms[i].Z && norm({c.r[j][0] - p[0], c.r[j][1] - p[1], c.r[j][2] - p[2]}) < tol) return true;
    return false;
  };
  if (ib < 0) {  // linear: 2 with an inversion centre
    for (int i = 0; i < na; ++i)
      if (!has_image(i, {-c.r[i][0], -c.r[i][1], -c.r[i][2]})) return 1;
    return 2;
  }
  // Every proper rotation is fixed by where it sends atoms ia and ib.
  const V3 fa = c.r[ia], fb = c.r[ib], fc = cross(fa, fb);
  Eigen::Matrix3d F;
  for (int k = 0; k < 3; ++k) { F(k, 0) = fa[k]; F(k, 1) = fb[k]; F(k, 2) = fc[k]; }
  const Eigen::Matrix3d Finv = F.inverse();
  int count = 0;
  for (int ja = 0; ja < na; ++ja) {
    if (mol.atoms[ja].Z != mol.atoms[ia].Z || std::fabs(norm(c.r[ja]) - norm(fa)) > tol) continue;
    for (int jb = 0; jb < na; ++jb) {
      if (mol.atoms[jb].Z != mol.atoms[ib].Z || std::fabs(norm(c.r[jb]) - norm(fb)) > tol) continue;
      if (std::fabs(dot(c.r[ja], c.r[jb]) - dot(fa, fb)) > tol * (norm(fa) + norm(fb))) continue;
      const V3 tc = cross(c.r[ja], c.r[jb]);
      Eigen::Matrix3d T;
      for (int k = 0; k < 3; ++k) { T(k, 0) = c.r[ja][k]; T(k, 1) = c.r[jb][k]; T(k, 2) = tc[k]; }
      const Eigen::Matrix3d R = T * Finv;
      if (std::fabs(R.determinant() - 1.0) > 1e-2 || (R * R.transpose() - Eigen::Matrix3d::Identity()).norm() > 5e-2) continue;
      bool ok = true;
      for (int i = 0; i < na && ok; ++i) {
        const Eigen::Vector3d p = R * Eigen::Vector3d(c.r[i][0], c.r[i][1], c.r[i][2]);
        ok = has_image(i, {p(0), p(1), p(2)});
      }
      if (ok) ++count;
    }
  }
  return std::max(count, 1);
}

Thermo thermochemistry(const Molecule& mol, const VibResult& v, double T, double p_atm, int sigma) {
  Thermo t;
  t.temperature = T;
  t.pressure = p_atm;
  const Centered c = centre_of_mass_frame(mol);
  const int na = static_cast<int>(c.m.size());
  t.symmetry_number = sigma > 0 ? sigma : rotational_symmetry_number(mol);
  const double pi = 3.14159265358979323846;
  const double kT = kBoltz * T;
  const double kEhK = kBoltz / kEh;  // hartree/K
  double M = 0;
  for (double m : c.m) M += m;

  const double qt = std::pow(2 * pi * M * kAmu * kT / (kPlanck * kPlanck), 1.5) * kT / (p_atm * kAtm);
  t.s_trans = kEhK * (std::log(qt) + 2.5);
  const double e_trans = 1.5 * kT / kEh;

  double e_rot = 0;
  Eigen::Matrix3d I = Eigen::Matrix3d::Zero();
  for (int a = 0; a < na; ++a) {
    const auto& r = c.r[a];
    const double r2 = r[0] * r[0] + r[1] * r[1] + r[2] * r[2];
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) I(i, j) += c.m[a] * ((i == j ? r2 : 0.0) - r[i] * r[j]);
  }
  Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> ei(I);
  auto theta = [&](double inertia) {  // rotational temperature, inertia in amu bohr^2
    return kPlanck * kPlanck / (8 * pi * pi * inertia * kAmu * kBohr * kBohr * kBoltz);
  };
  t.linear = (na > 1 && v.ntransrot == 5);
  if (t.linear) {
    const double q = T / (t.symmetry_number * theta(ei.eigenvalues()(2)));
    t.s_rot = kEhK * (std::log(q) + 1.0);
    e_rot = kT / kEh;
  } else if (na > 1) {
    const double q = std::sqrt(pi) / t.symmetry_number * std::pow(T, 1.5) /
                     std::sqrt(theta(ei.eigenvalues()(0)) * theta(ei.eigenvalues()(1)) * theta(ei.eigenvalues()(2)));
    t.s_rot = kEhK * (std::log(q) + 1.5);
    e_rot = 1.5 * kT / kEh;
  }

  // Imaginary modes are left out of every vibrational term.
  double e_vib = 0;
  const double hc = kPlanck * kC * 100.0;  // J per cm-1
  for (double nu : v.freq) {
    if (nu <= 0) continue;
    const double x = hc * nu / kT;
    t.zpe += 0.5 * hc * nu / kEh;
    e_vib += hc * nu / kEh / std::expm1(x);
    t.s_vib += kEhK * (x / std::expm1(x) - std::log1p(-std::exp(-x)));
  }
  t.s_elec = kEhK * std::log(static_cast<double>(mol.multiplicity));

  t.e_thermal = t.zpe + e_trans + e_rot + e_vib;
  t.enthalpy_corr = t.e_thermal + kT / kEh;
  t.ts = T * (t.s_trans + t.s_rot + t.s_vib + t.s_elec);
  t.gibbs_corr = t.enthalpy_corr - t.ts;
  return t;
}

}  // namespace qm
