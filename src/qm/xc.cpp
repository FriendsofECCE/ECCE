#include "xc.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <xc.h>
#include <xc_funcs.h>
#include "ao.hpp"
#ifdef _OPENMP
#include <omp.h>
#endif

namespace qm {

FunctionalSpec functional_by_name(const std::string& name) {
  std::string n;
  for (char c : name) n += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  FunctionalSpec f;
  f.name = n;
  if (n == "hf") return f;
  if (n == "svwn" || n == "lda") { f.libxc_ids = {XC_LDA_X, XC_LDA_C_VWN}; f.exx = 0; }
  else if (n == "pbe") { f.libxc_ids = {XC_GGA_X_PBE, XC_GGA_C_PBE}; f.exx = 0; f.gga = true; }
  else if (n == "b3lyp") { f.libxc_ids = {XC_HYB_GGA_XC_B3LYP5}; f.exx = 0.2; f.gga = true; }
  else if (n == "pbe0") { f.libxc_ids = {XC_HYB_GGA_XC_PBEH}; f.exx = 0.25; f.gga = true; }
  else throw Error("unknown method '" + name + "' (hf, svwn, pbe, b3lyp, pbe0)");
  return f;
}

// Batches of nearby grid points, each with the shells that reach it.
struct XcIntegrator::Impl {
  struct Batch {
    size_t g0 = 0;
    int n = 0;
    std::vector<int> shells;  // shells with any value above the cutoff here
    std::vector<int> ao;      // their AOs, in basis order
    std::vector<int> aoatom;  // atom of each of those AOs
  };
  std::vector<Batch> batches;
};

namespace {
const int kBatch = 128;  // points per batch

// Recursive bisection along the longest side so that each leaf is compact.
void bisect(const Grid& g, std::vector<size_t>& idx, size_t lo, size_t hi, std::vector<std::pair<size_t, size_t>>& leaves) {
  if (hi - lo <= static_cast<size_t>(kBatch)) { leaves.emplace_back(lo, hi); return; }
  double mn[3] = {1e300, 1e300, 1e300}, mx[3] = {-1e300, -1e300, -1e300};
  for (size_t k = lo; k < hi; ++k) {
    const double p[3] = {g.x[idx[k]], g.y[idx[k]], g.z[idx[k]]};
    for (int c = 0; c < 3; ++c) { mn[c] = std::min(mn[c], p[c]); mx[c] = std::max(mx[c], p[c]); }
  }
  int ax = 0;
  for (int c = 1; c < 3; ++c) if (mx[c] - mn[c] > mx[ax] - mn[ax]) ax = c;
  const std::vector<double>& v = ax == 0 ? g.x : (ax == 1 ? g.y : g.z);
  const size_t mid = (lo + hi) / 2;
  std::nth_element(idx.begin() + lo, idx.begin() + mid, idx.begin() + hi, [&](size_t a, size_t b) { return v[a] < v[b]; });
  bisect(g, idx, lo, mid, leaves);
  bisect(g, idx, mid, hi, leaves);
}
}  // namespace

XcIntegrator::XcIntegrator(const Basis& b, const FunctionalSpec& f, Grid g)
    : basis_(b), spec_(f), impl_(new Impl) {
  std::vector<size_t> idx(g.size());
  for (size_t k = 0; k < idx.size(); ++k) idx[k] = k;
  std::vector<std::pair<size_t, size_t>> leaves;
  bisect(g, idx, 0, idx.size(), leaves);
  for (size_t k = 0; k < idx.size(); ++k) {
    grid_.x.push_back(g.x[idx[k]]); grid_.y.push_back(g.y[idx[k]]);
    grid_.z.push_back(g.z[idx[k]]); grid_.w.push_back(g.w[idx[k]]);
  }
  for (const auto& lf : leaves) {
    Impl::Batch B;
    B.g0 = lf.first;
    B.n = static_cast<int>(lf.second - lf.first);
    double mn[3] = {1e300, 1e300, 1e300}, mx[3] = {-1e300, -1e300, -1e300};
    for (size_t k = lf.first; k < lf.second; ++k) {
      const double p[3] = {grid_.x[k], grid_.y[k], grid_.z[k]};
      for (int c = 0; c < 3; ++c) { mn[c] = std::min(mn[c], p[c]); mx[c] = std::max(mx[c], p[c]); }
    }
    for (int is = 0; is < basis_.nshl(); ++is) {
      const Shell& s = basis_.shells()[is];
      double d2 = 0;  // squared distance from the shell centre to the batch's bounding box
      for (int c = 0; c < 3; ++c) {
        const double d = std::max({mn[c] - s.center[c], s.center[c] - mx[c], 0.0});
        d2 += d * d;
      }
      if (d2 > s.extent * s.extent) continue;
      B.shells.push_back(is);
      for (int k = 0; k < s.nao; ++k) { B.ao.push_back(s.ao0 + k); B.aoatom.push_back(s.atom); }
    }
    if (!B.shells.empty()) impl_->batches.push_back(std::move(B));
  }
}
XcIntegrator::~XcIntegrator() = default;

namespace {
struct Func {
  xc_func_type f;
  Func(int id, int pol) {
    if (xc_func_init(&f, id, pol ? XC_POLARIZED : XC_UNPOLARIZED) != 0) throw Error("libxc: cannot init functional");
  }
  ~Func() { xc_func_end(&f); }
};

// Closed-shell libxc derivatives of the energy density with respect to the
// total rho and sigma = |grad rho|^2: first (vr, vs) and second (frr, frs, fss).
struct Kern {
  std::vector<double> vr, vs, frr, frs, fss;
};
void eval_kernel(std::vector<std::unique_ptr<Func>>& fs, bool gga, int n, const double* rho, const double* sig, Kern& k) {
  k.vr.assign(n, 0.0); k.frr.assign(n, 0.0);
  k.vs.assign(gga ? n : 0, 0.0); k.frs.assign(gga ? n : 0, 0.0); k.fss.assign(gga ? n : 0, 0.0);
  std::vector<double> a(n), b(n), c(n), d(n), e(n);
  for (auto& fp : fs) {
    const xc_func_type* p = &fp->f;
    if (p->info->family == XC_FAMILY_LDA) {
      xc_lda_vxc_fxc(p, n, rho, a.data(), c.data());
      for (int g = 0; g < n; ++g) { k.vr[g] += a[g]; k.frr[g] += c[g]; }
    } else {
      xc_gga_vxc_fxc(p, n, rho, sig, a.data(), b.data(), c.data(), d.data(), e.data());
      for (int g = 0; g < n; ++g) {
        k.vr[g] += a[g]; k.vs[g] += b[g]; k.frr[g] += c[g]; k.frs[g] += d[g]; k.fss[g] += e[g];
      }
    }
  }
}
}  // namespace

bool xc_analytic_hessian_supported(const FunctionalSpec& f) {
  for (int id : f.libxc_ids) {
    Func fn(id, 0);
    const int fam = fn.f.info->family;
    if (fam != XC_FAMILY_LDA && fam != XC_FAMILY_GGA && fam != XC_FAMILY_HYB_GGA) return false;
    if (!(fn.f.info->flags & XC_FLAGS_HAVE_FXC)) return false;
    double om = 0, al = 0, be = 0;
    xc_hyb_cam_coef(&fn.f, &om, &al, &be);
    if (om != 0.0) return false;
  }
  return true;
}

double XcIntegrator::compute(const Mat& Da, const Mat& Db, bool closed, Mat& Va, Mat& Vb, double* nel) {
  const int nb = basis_.nbf();
  const bool gga = spec_.gga;
  const int deriv = gga ? 1 : 0;
  const int blk = kBatch;
  const int nblk = static_cast<int>(impl_->batches.size());
  Va = Mat::Zero(nb, nb);
  Vb = Mat::Zero(nb, nb);
  double Exc = 0, ne = 0;
  const int pol = closed ? 0 : 1;
#pragma omp parallel
  {
    std::vector<std::unique_ptr<Func>> fs;
    for (int id : spec_.libxc_ids) fs.emplace_back(new Func(id, pol));
    Mat Vat = Mat::Zero(nb, nb), Vbt = Mat::Zero(nb, nb);
    double Et = 0, net = 0;
    AoValues ao;
    std::vector<double> xyz(3 * blk);
#pragma omp for schedule(dynamic)
    for (int ib = 0; ib < nblk; ++ib) {
      const Impl::Batch& B = impl_->batches[ib];
      const size_t g0 = B.g0;
      const int n = B.n, nl = static_cast<int>(B.ao.size());
      for (int g = 0; g < n; ++g) {
        xyz[3 * g] = grid_.x[g0 + g]; xyz[3 * g + 1] = grid_.y[g0 + g]; xyz[3 * g + 2] = grid_.z[g0 + g];
      }
      eval_ao(basis_, xyz.data(), n, deriv, ao, &B.shells);
      const Mat Das = Da(B.ao, B.ao), Dbs = closed ? Das : Db(B.ao, B.ao);
      // densities
      Mat ta = ao.phi * Das;
      Vec ra = (ta.array() * ao.phi.array()).rowwise().sum();
      Mat tb;
      Vec rb;
      if (!closed) { tb = ao.phi * Dbs; rb = (tb.array() * ao.phi.array()).rowwise().sum(); }
      else rb = ra;
      Vec gax, gay, gaz, gbx, gby, gbz;
      if (gga) {
        gax = 2 * (ta.array() * ao.dx.array()).rowwise().sum();
        gay = 2 * (ta.array() * ao.dy.array()).rowwise().sum();
        gaz = 2 * (ta.array() * ao.dz.array()).rowwise().sum();
        if (!closed) {
          gbx = 2 * (tb.array() * ao.dx.array()).rowwise().sum();
          gby = 2 * (tb.array() * ao.dy.array()).rowwise().sum();
          gbz = 2 * (tb.array() * ao.dz.array()).rowwise().sum();
        } else { gbx = gax; gby = gay; gbz = gaz; }
      }
      // libxc inputs
      std::vector<double> rho(closed ? n : 2 * n), sig(gga ? (closed ? n : 3 * n) : 0);
      std::vector<double> zk(n), vrho(closed ? n : 2 * n), vsig(gga ? (closed ? n : 3 * n) : 0);
      std::vector<double> e_acc(n, 0.0), vr_acc(rho.size(), 0.0), vs_acc(sig.size(), 0.0);
      for (int g = 0; g < n; ++g) {
        if (closed) {
          rho[g] = 2 * ra[g];
          if (gga) { double X = gax[g] * 2, Y = gay[g] * 2, Z = gaz[g] * 2; sig[g] = X * X + Y * Y + Z * Z; }
        } else {
          rho[2 * g] = ra[g]; rho[2 * g + 1] = rb[g];
          if (gga) {
            sig[3 * g] = gax[g] * gax[g] + gay[g] * gay[g] + gaz[g] * gaz[g];
            sig[3 * g + 1] = gax[g] * gbx[g] + gay[g] * gby[g] + gaz[g] * gbz[g];
            sig[3 * g + 2] = gbx[g] * gbx[g] + gby[g] * gby[g] + gbz[g] * gbz[g];
          }
        }
        net += grid_.w[g0 + g] * (ra[g] + rb[g]);
      }
      for (auto& fp : fs) {
        std::fill(zk.begin(), zk.end(), 0.0);
        std::fill(vrho.begin(), vrho.end(), 0.0);
        std::fill(vsig.begin(), vsig.end(), 0.0);
        const xc_func_type* p = &fp->f;
        if (p->info->family == XC_FAMILY_LDA)
          xc_lda_exc_vxc(p, n, rho.data(), zk.data(), vrho.data());
        else
          xc_gga_exc_vxc(p, n, rho.data(), sig.data(), zk.data(), vrho.data(), vsig.data());
        for (int g = 0; g < n; ++g) e_acc[g] += zk[g];
        for (size_t k = 0; k < vr_acc.size(); ++k) vr_acc[k] += vrho[k];
        for (size_t k = 0; k < vs_acc.size(); ++k) vs_acc[k] += vsig[k];
      }
      // energy and potential pieces
      Mat Aa = Mat::Zero(n, nl), Ab;
      if (!closed) Ab = Mat::Zero(n, nl);
      Vec wv(n);
      for (int g = 0; g < n; ++g) {
        const double w = grid_.w[g0 + g];
        Et += w * (rho.size() == static_cast<size_t>(n) ? rho[g] : rho[2 * g] + rho[2 * g + 1]) * e_acc[g];
      }
      if (closed) {
        for (int g = 0; g < n; ++g) {
          const double w = grid_.w[g0 + g];
          double c0 = 0.5 * w * vr_acc[g];
          double cx = 0, cy = 0, cz = 0;
          if (gga) {
            double f = 2 * w * vs_acc[g] * 2;  // 2*vsigma*grad(rho_total) with grad(rho_total) = 2*ga
            cx = f * gax[g]; cy = f * gay[g]; cz = f * gaz[g];
          }
          Aa.row(g) = c0 * ao.phi.row(g);
          if (gga) Aa.row(g) += cx * ao.dx.row(g) + cy * ao.dy.row(g) + cz * ao.dz.row(g);
        }
        Mat Vt = ao.phi.transpose() * Aa;
        Vat(B.ao, B.ao) += Vt + Vt.transpose();
      } else {
        for (int g = 0; g < n; ++g) {
          const double w = grid_.w[g0 + g];
          double ca = 0.5 * w * vr_acc[2 * g], cb = 0.5 * w * vr_acc[2 * g + 1];
          Aa.row(g) = ca * ao.phi.row(g);
          Ab.row(g) = cb * ao.phi.row(g);
          if (gga) {
            double vaa = vs_acc[3 * g], vab = vs_acc[3 * g + 1], vbb = vs_acc[3 * g + 2];
            double ax = w * (2 * vaa * gax[g] + vab * gbx[g]), ay = w * (2 * vaa * gay[g] + vab * gby[g]),
                   az = w * (2 * vaa * gaz[g] + vab * gbz[g]);
            double bx = w * (2 * vbb * gbx[g] + vab * gax[g]), by = w * (2 * vbb * gby[g] + vab * gay[g]),
                   bz = w * (2 * vbb * gbz[g] + vab * gaz[g]);
            Aa.row(g) += ax * ao.dx.row(g) + ay * ao.dy.row(g) + az * ao.dz.row(g);
            Ab.row(g) += bx * ao.dx.row(g) + by * ao.dy.row(g) + bz * ao.dz.row(g);
          }
        }
        Mat Vta = ao.phi.transpose() * Aa, Vtb = ao.phi.transpose() * Ab;
        Vat(B.ao, B.ao) += Vta + Vta.transpose();
        Vbt(B.ao, B.ao) += Vtb + Vtb.transpose();
      }
    }
#pragma omp critical
    {
      Va += Vat; Vb += Vbt; Exc += Et; ne += net;
    }
  }
  if (closed) Vb = Va;
  if (nel) *nel = ne;
  return Exc;
}

void XcIntegrator::gradient(const Mat& Da, const Mat& Db, bool closed, std::vector<double>& grad) {
  const int nb = basis_.nbf();
  const bool gga = spec_.gga;
  const int blk = kBatch;
  const int nblk = static_cast<int>(impl_->batches.size());
  const int natm = basis_.natm();
  grad.assign(3 * natm, 0.0);
  const int pol = closed ? 0 : 1;
#pragma omp parallel
  {
    std::vector<std::unique_ptr<Func>> fs;
    for (int id : spec_.libxc_ids) fs.emplace_back(new Func(id, pol));
    std::vector<double> gt(3 * natm, 0.0);
    AoValues ao;
    std::vector<double> xyz(3 * blk);
#pragma omp for schedule(dynamic)
    for (int ib = 0; ib < nblk; ++ib) {
      const Impl::Batch& B = impl_->batches[ib];
      const size_t g0 = B.g0;
      const int n = B.n, nl = static_cast<int>(B.ao.size());
      for (int g = 0; g < n; ++g) {
        xyz[3 * g] = grid_.x[g0 + g]; xyz[3 * g + 1] = grid_.y[g0 + g]; xyz[3 * g + 2] = grid_.z[g0 + g];
      }
      eval_ao(basis_, xyz.data(), n, gga ? 2 : 1, ao, &B.shells);
      const Mat Das = Da(B.ao, B.ao), Dbs = closed ? Das : Db(B.ao, B.ao);
      const Mat* dphi[3] = {&ao.dx, &ao.dy, &ao.dz};
      const Mat* hess[3][3] = {{&ao.dxx, &ao.dxy, &ao.dxz}, {&ao.dxy, &ao.dyy, &ao.dyz}, {&ao.dxz, &ao.dyz, &ao.dzz}};
      const Mat ta = ao.phi * Das;
      const Vec ra = (ta.array() * ao.phi.array()).rowwise().sum();
      Mat tb;
      Vec rb = ra;
      if (!closed) { tb = ao.phi * Dbs; rb = (tb.array() * ao.phi.array()).rowwise().sum(); }
      Mat tda[3], tdb[3];  // grad(phi) * D
      Vec ga[3], gb[3];    // grad(rho_spin)
      if (gga) {
        for (int d = 0; d < 3; ++d) {
          tda[d] = *dphi[d] * Das;
          ga[d] = 2 * (ta.array() * dphi[d]->array()).rowwise().sum();
          if (!closed) {
            tdb[d] = *dphi[d] * Dbs;
            gb[d] = 2 * (tb.array() * dphi[d]->array()).rowwise().sum();
          } else gb[d] = ga[d];
        }
      }
      std::vector<double> rho(closed ? n : 2 * n), sig(gga ? (closed ? n : 3 * n) : 0);
      std::vector<double> zk(n), vrho(rho.size()), vsig(sig.size());
      std::vector<double> vr_acc(rho.size(), 0.0), vs_acc(sig.size(), 0.0);
      for (int g = 0; g < n; ++g) {
        if (closed) {
          rho[g] = 2 * ra[g];
          if (gga) { double X = 2 * ga[0][g], Y = 2 * ga[1][g], Z = 2 * ga[2][g]; sig[g] = X * X + Y * Y + Z * Z; }
        } else {
          rho[2 * g] = ra[g]; rho[2 * g + 1] = rb[g];
          if (gga) {
            sig[3 * g] = ga[0][g] * ga[0][g] + ga[1][g] * ga[1][g] + ga[2][g] * ga[2][g];
            sig[3 * g + 1] = ga[0][g] * gb[0][g] + ga[1][g] * gb[1][g] + ga[2][g] * gb[2][g];
            sig[3 * g + 2] = gb[0][g] * gb[0][g] + gb[1][g] * gb[1][g] + gb[2][g] * gb[2][g];
          }
        }
      }
      for (auto& fp : fs) {
        std::fill(zk.begin(), zk.end(), 0.0);
        std::fill(vrho.begin(), vrho.end(), 0.0);
        std::fill(vsig.begin(), vsig.end(), 0.0);
        const xc_func_type* p = &fp->f;
        if (p->info->family == XC_FAMILY_LDA) xc_lda_exc_vxc(p, n, rho.data(), zk.data(), vrho.data());
        else xc_gga_exc_vxc(p, n, rho.data(), sig.data(), zk.data(), vrho.data(), vsig.data());
        for (size_t k = 0; k < vr_acc.size(); ++k) vr_acc[k] += vrho[k];
        for (size_t k = 0; k < vs_acc.size(); ++k) vs_acc[k] += vsig[k];
      }
      // One spin's part: -2 sum_{mu on A, nu} D_mu,nu int [ v_rho d_x(phi_mu) phi_nu
      // + g . grad(d_x(phi_mu) phi_nu) ], with the g vector compute() puts in the potential.
      auto spin_term = [&](const Mat& t, const Mat* td, const Vec& vrw, const Vec* gv, double factor) {
        for (int d = 0; d < 3; ++d) {
          Mat A = dphi[d]->array() * (t.array().colwise() * vrw.array());
          if (gga)
            for (int e = 0; e < 3; ++e)
              A.array() += (hess[d][e]->array() * t.array() + dphi[d]->array() * td[e].array()).colwise() * gv[e].array();
          const Vec col = A.colwise().sum();
          for (int m = 0; m < nl; ++m) gt[3 * B.aoatom[m] + d] -= 2 * factor * col(m);
        }
      };
      Vec vra(n), vrb(n), gva[3], gvb[3];
      for (int d = 0; d < 3; ++d) { gva[d] = Vec::Zero(n); gvb[d] = Vec::Zero(n); }
      for (int g = 0; g < n; ++g) {
        const double w = grid_.w[g0 + g];
        if (closed) {
          vra[g] = w * vr_acc[g];
          if (gga) for (int d = 0; d < 3; ++d) gva[d][g] = 4 * w * vs_acc[g] * ga[d][g];
        } else {
          vra[g] = w * vr_acc[2 * g];
          vrb[g] = w * vr_acc[2 * g + 1];
          if (gga) {
            const double vaa = vs_acc[3 * g], vab = vs_acc[3 * g + 1], vbb = vs_acc[3 * g + 2];
            for (int d = 0; d < 3; ++d) {
              gva[d][g] = w * (2 * vaa * ga[d][g] + vab * gb[d][g]);
              gvb[d][g] = w * (2 * vbb * gb[d][g] + vab * ga[d][g]);
            }
          }
        }
      }
      spin_term(ta, tda, vra, gva, closed ? 2.0 : 1.0);
      if (!closed) spin_term(tb, tdb, vrb, gvb, 1.0);
    }
#pragma omp critical
    for (int k = 0; k < 3 * natm; ++k) grad[k] += gt[k];
  }
}

// The Vxc matrix is V_mn = sum_g w [vr phi_m phi_n + 2 vs grad(rho) . grad(phi_m phi_n)].
// A first-order density (drho, dgrad) changes it through vr, vs (kernel) and
// through grad(rho) itself; a0 and ad below are the coefficients of phi_m phi_n
// and of grad_d(phi_m phi_n) in that change.
std::vector<Mat> XcIntegrator::kernel(const Mat& P, const std::vector<Mat>& D1) {
  const int nb = basis_.nbf(), nd = static_cast<int>(D1.size());
  const bool gga = spec_.gga;
  const int nblk = static_cast<int>(impl_->batches.size());
  std::vector<Mat> out(nd, Mat::Zero(nb, nb));
#pragma omp parallel
  {
    std::vector<std::unique_ptr<Func>> fs;
    for (int id : spec_.libxc_ids) fs.emplace_back(new Func(id, 0));
    std::vector<Mat> ot(nd, Mat::Zero(nb, nb));
    AoValues ao;
    std::vector<double> xyz(3 * kBatch);
    Kern K;
#pragma omp for schedule(dynamic)
    for (int ib = 0; ib < nblk; ++ib) {
      const Impl::Batch& B = impl_->batches[ib];
      const size_t g0 = B.g0;
      const int n = B.n;
      for (int g = 0; g < n; ++g) {
        xyz[3 * g] = grid_.x[g0 + g]; xyz[3 * g + 1] = grid_.y[g0 + g]; xyz[3 * g + 2] = grid_.z[g0 + g];
      }
      eval_ao(basis_, xyz.data(), n, gga ? 1 : 0, ao, &B.shells);
      const Mat* d1[3] = {&ao.dx, &ao.dy, &ao.dz};
      const Mat t = ao.phi * P(B.ao, B.ao);
      const Vec rho = (t.array() * ao.phi.array()).rowwise().sum();
      Vec gr[3], w(n);
      std::vector<double> sig(gga ? n : 0);
      if (gga) {
        for (int e = 0; e < 3; ++e) gr[e] = 2 * (t.array() * d1[e]->array()).rowwise().sum();
        for (int g = 0; g < n; ++g) sig[g] = gr[0][g] * gr[0][g] + gr[1][g] * gr[1][g] + gr[2][g] * gr[2][g];
      }
      for (int g = 0; g < n; ++g) w[g] = grid_.w[g0 + g];
      eval_kernel(fs, gga, n, rho.data(), sig.data(), K);
      for (int k = 0; k < nd; ++k) {
        const Mat u = ao.phi * D1[k](B.ao, B.ao);
        const Vec dr = (u.array() * ao.phi.array()).rowwise().sum();
        Vec a0(n), ad[3];
        if (!gga) {
          for (int g = 0; g < n; ++g) a0[g] = w[g] * K.frr[g] * dr[g];
        } else {
          Vec dg[3];
          for (int e = 0; e < 3; ++e) { dg[e] = 2 * (u.array() * d1[e]->array()).rowwise().sum(); ad[e].resize(n); }
          for (int g = 0; g < n; ++g) {
            const double ds = 2 * (gr[0][g] * dg[0][g] + gr[1][g] * dg[1][g] + gr[2][g] * dg[2][g]);
            const double dvr = K.frr[g] * dr[g] + K.frs[g] * ds, dvs = K.frs[g] * dr[g] + K.fss[g] * ds;
            a0[g] = w[g] * dvr;
            for (int e = 0; e < 3; ++e) ad[e][g] = 2 * w[g] * (dvs * gr[e][g] + K.vs[g] * dg[e][g]);
          }
        }
        Mat A = ao.phi.array().colwise() * (0.5 * a0).array();
        if (gga)
          for (int e = 0; e < 3; ++e) A.array() += d1[e]->array().colwise() * ad[e].array();
        const Mat M = ao.phi.transpose() * A;
        ot[k](B.ao, B.ao) += M + M.transpose();
      }
    }
#pragma omp critical
    for (int k = 0; k < nd; ++k) out[k] += ot[k];
  }
  return out;
}

// Perturbation x = (atom A, direction c) moves the functions on A, so at fixed
// P: drho^x = -2 sum_{m on A} d_c(phi_m) (phi P)_m and dgrad^x its gradient;
// d2Exc/dxdy = kernel(drho^x, drho^y) + sum_g w [vr rho^xy + 2 vs grad(rho).grad(rho^xy)],
// rho^xy = 2 sum P [d_A,B d_cd(phi_m) phi_n + d_c(phi_m) d_d(phi_n), m on A, n on B].
// GGAs need third AO derivatives for grad(rho^xy).
void XcIntegrator::hessian_terms(const Mat& P, Mat& H, std::vector<Mat>& Vx) {
  const int nb = basis_.nbf(), NP = 3 * basis_.natm();
  const bool gga = spec_.gga;
  const int nblk = static_cast<int>(impl_->batches.size());
#pragma omp parallel
  {
    std::vector<std::unique_ptr<Func>> fs;
    for (int id : spec_.libxc_ids) fs.emplace_back(new Func(id, 0));
    Mat Ht = Mat::Zero(NP, NP);
    std::vector<Mat> Vt(NP, Mat::Zero(nb, nb));
    AoValues ao;
    std::vector<double> xyz(3 * kBatch);
    Kern K;
#pragma omp for schedule(dynamic)
    for (int ib = 0; ib < nblk; ++ib) {
      const Impl::Batch& B = impl_->batches[ib];
      const size_t g0 = B.g0;
      const int n = B.n, nl = static_cast<int>(B.ao.size());
      for (int g = 0; g < n; ++g) {
        xyz[3 * g] = grid_.x[g0 + g]; xyz[3 * g + 1] = grid_.y[g0 + g]; xyz[3 * g + 2] = grid_.z[g0 + g];
      }
      eval_ao(basis_, xyz.data(), n, gga ? 3 : 2, ao, &B.shells);
      const Mat* d1[3] = {&ao.dx, &ao.dy, &ao.dz};
      const Mat* d2[3][3] = {{&ao.dxx, &ao.dxy, &ao.dxz}, {&ao.dxy, &ao.dyy, &ao.dyz}, {&ao.dxz, &ao.dyz, &ao.dzz}};
      const Mat Ps = P(B.ao, B.ao);
      const Mat t = ao.phi * Ps;
      const Vec rho = (t.array() * ao.phi.array()).rowwise().sum();
      Mat td[3];
      Vec gr[3], w(n), wvr(n), s[3];  // s_e = 2 w vs grad_e(rho)
      std::vector<double> sig(gga ? n : 0);
      if (gga) {
        for (int e = 0; e < 3; ++e) {
          td[e] = *d1[e] * Ps;
          gr[e] = 2 * (t.array() * d1[e]->array()).rowwise().sum();
        }
        for (int g = 0; g < n; ++g) sig[g] = gr[0][g] * gr[0][g] + gr[1][g] * gr[1][g] + gr[2][g] * gr[2][g];
      }
      eval_kernel(fs, gga, n, rho.data(), sig.data(), K);
      for (int g = 0; g < n; ++g) { w[g] = grid_.w[g0 + g]; wvr[g] = w[g] * K.vr[g]; }
      if (gga)
        for (int e = 0; e < 3; ++e) {
          s[e].resize(n);
          for (int g = 0; g < n; ++g) s[e][g] = 2 * w[g] * K.vs[g] * gr[e][g];
        }

      // atoms with functions in this batch; M(m, a) = 1 when AO m sits on local atom a
      std::vector<int> atoms, la(nl);
      for (int m = 0; m < nl; ++m) {
        auto it = std::find(atoms.begin(), atoms.end(), B.aoatom[m]);
        la[m] = static_cast<int>(it - atoms.begin());
        if (it == atoms.end()) atoms.push_back(B.aoatom[m]);
      }
      const int nal = static_cast<int>(atoms.size()), nx = 3 * nal;
      Mat M = Mat::Zero(nl, nal);
      for (int m = 0; m < nl; ++m) M(m, la[m]) = 1.0;

      // first-order densities of the nx local perturbations, column 3a+c
      Mat dr(n, nx), dg[3];
      if (gga) for (int e = 0; e < 3; ++e) dg[e].resize(n, nx);
      for (int c = 0; c < 3; ++c) {
        const Mat r = -2.0 * (d1[c]->array() * t.array()).matrix() * M;
        for (int a = 0; a < nal; ++a) dr.col(3 * a + c) = r.col(a);
        if (gga)
          for (int e = 0; e < 3; ++e) {
            const Mat q = -2.0 * (d2[c][e]->array() * t.array() + d1[c]->array() * td[e].array()).matrix() * M;
            for (int a = 0; a < nal; ++a) dg[e].col(3 * a + c) = q.col(a);
          }
      }
      Mat a0(n, nx), ad[3];
      if (!gga) {
        a0 = dr.array().colwise() * (w.array() * Eigen::Map<const Vec>(K.frr.data(), n).array());
      } else {
        for (int e = 0; e < 3; ++e) ad[e].resize(n, nx);
        for (int x = 0; x < nx; ++x)
          for (int g = 0; g < n; ++g) {
            const double ds = 2 * (gr[0][g] * dg[0](g, x) + gr[1][g] * dg[1](g, x) + gr[2][g] * dg[2](g, x));
            const double dvr = K.frr[g] * dr(g, x) + K.frs[g] * ds, dvs = K.frs[g] * dr(g, x) + K.fss[g] * ds;
            a0(g, x) = w[g] * dvr;
            for (int e = 0; e < 3; ++e) ad[e](g, x) = 2 * w[g] * (dvs * gr[e][g] + K.vs[g] * dg[e](g, x));
          }
      }

      // second derivatives: kernel part, then rho^xy
      Mat hl = a0.transpose() * dr;
      if (gga) for (int e = 0; e < 3; ++e) hl += ad[e].transpose() * dg[e];
      for (int c = 0; c < 3; ++c)
        for (int d = 0; d < 3; ++d) {
          Mat Q = (d2[c][d]->array() * t.array()).colwise() * wvr.array();
          if (gga)
            for (int e = 0; e < 3; ++e)
              Q.array() += (ao.d3[ao_d3_index(c, d, e)].array() * t.array() + d2[c][d]->array() * td[e].array()).colwise() * s[e].array();
          const Vec same = 2.0 * M.transpose() * Q.colwise().sum().transpose();
          for (int a = 0; a < nal; ++a) hl(3 * a + c, 3 * a + d) += same[a];
          Mat Kcd = d1[c]->transpose() * (d1[d]->array().colwise() * wvr.array()).matrix();
          if (gga)
            for (int e = 0; e < 3; ++e)
              Kcd += d2[c][e]->transpose() * (d1[d]->array().colwise() * s[e].array()).matrix() +
                     d1[c]->transpose() * (d2[d][e]->array().colwise() * s[e].array()).matrix();
          const Mat pair = 2.0 * M.transpose() * Ps.cwiseProduct(Kcd) * M;
          for (int a = 0; a < nal; ++a)
            for (int b = 0; b < nal; ++b) hl(3 * a + c, 3 * b + d) += pair(a, b);
        }
      for (int a = 0; a < nal; ++a)
        for (int b = 0; b < nal; ++b)
          Ht.block(3 * atoms[a], 3 * atoms[b], 3, 3) += hl.block(3 * a, 3 * b, 3, 3);

      // Fock derivatives: the functions' own motion, then the kernel part
      Mat base = ao.phi.array().colwise() * wvr.array();
      if (gga) for (int e = 0; e < 3; ++e) base.array() += d1[e]->array().colwise() * s[e].array();
      for (int c = 0; c < 3; ++c) {
        Mat Mc = d1[c]->transpose() * base;
        if (gga)
          for (int e = 0; e < 3; ++e) Mc += d2[c][e]->transpose() * (ao.phi.array().colwise() * s[e].array()).matrix();
        for (int m = 0; m < nl; ++m) {
          Mat& V = Vt[3 * B.aoatom[m] + c];
          for (int k = 0; k < nl; ++k) {
            V(B.ao[m], B.ao[k]) -= Mc(m, k);
            V(B.ao[k], B.ao[m]) -= Mc(m, k);
          }
        }
      }
      for (int x = 0; x < nx; ++x) {
        Mat A = ao.phi.array().colwise() * (0.5 * a0.col(x)).array();
        if (gga) for (int e = 0; e < 3; ++e) A.array() += d1[e]->array().colwise() * ad[e].col(x).array();
        const Mat Mm = ao.phi.transpose() * A;
        Vt[3 * atoms[x / 3] + x % 3](B.ao, B.ao) += Mm + Mm.transpose();
      }
    }
#pragma omp critical
    {
      H += Ht;
      for (int p = 0; p < NP; ++p) Vx[p] += Vt[p];
    }
  }
}

}  // namespace qm
