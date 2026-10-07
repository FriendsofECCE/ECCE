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

struct XcIntegrator::Impl {};

XcIntegrator::XcIntegrator(const Basis& b, const FunctionalSpec& f, Grid g)
    : basis_(b), spec_(f), grid_(std::move(g)), impl_(new Impl) {}
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
  const size_t ng = grid_.size();
  const int blk = 128;
  const int nblk = static_cast<int>((ng + blk - 1) / blk);
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
      const size_t g0 = static_cast<size_t>(ib) * blk;
      const int n = static_cast<int>(std::min<size_t>(blk, ng - g0));
      for (int g = 0; g < n; ++g) {
        xyz[3 * g] = grid_.x[g0 + g]; xyz[3 * g + 1] = grid_.y[g0 + g]; xyz[3 * g + 2] = grid_.z[g0 + g];
      }
      eval_ao(basis_, xyz.data(), n, deriv, ao);
      // densities
      Mat ta = ao.phi * Da;
      Vec ra = (ta.array() * ao.phi.array()).rowwise().sum();
      Mat tb;
      Vec rb;
      if (!closed) { tb = ao.phi * Db; rb = (tb.array() * ao.phi.array()).rowwise().sum(); }
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
      Mat Aa = Mat::Zero(n, nb), Ab;
      if (!closed) Ab = Mat::Zero(n, nb);
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
        Vat += Vt + Vt.transpose();
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
        Vat += Vta + Vta.transpose();
        Vbt += Vtb + Vtb.transpose();
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

}  // namespace qm
