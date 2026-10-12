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
}  // namespace

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

}  // namespace qm
