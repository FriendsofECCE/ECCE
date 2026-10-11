#include "gradient.hpp"
#include <array>
#include <cmath>
#include <memory>
#include "cint_wrap.hpp"
#include "xc.hpp"

namespace qm {

namespace {

using Fn1 = CINTIntegralFunction;
using Mat3 = std::array<Mat, 3>;

// <grad_x i | O | j> for all AO pairs (no symmetry: the derivative acts on i
// only), one matrix per Cartesian component.
Mat3 one_e_deriv(Basis& b, Fn1* f) {
  const int n = b.nbf(), ns = b.nshl();
  Mat3 out = {Mat::Zero(n, n), Mat::Zero(n, n), Mat::Zero(n, n)};
  const auto& sh = b.shells();
  std::vector<double> buf, cache;
  for (int I = 0; I < ns; ++I)
    for (int J = 0; J < ns; ++J) {
      int shls[2] = {I, J};
      const int ni = sh[I].nao, nj = sh[J].nao;
      buf.assign(static_cast<size_t>(ni) * nj * 3, 0.0);
      size_t cs = f(nullptr, nullptr, shls, b.atm().data(), b.natm(), b.bas().data(), ns, b.env().data(), nullptr, nullptr);
      cache.resize(cs + 16);
      f(buf.data(), nullptr, shls, b.atm().data(), b.natm(), b.bas().data(), ns, b.env().data(), nullptr, cache.data());
      for (int c = 0; c < 3; ++c)
        for (int j = 0; j < nj; ++j)
          for (int i = 0; i < ni; ++i) out[c](sh[I].ao0 + i, sh[J].ao0 + j) = buf[c * ni * nj + j * ni + i];
    }
  return out;
}

}  // namespace

std::vector<double> scf_gradient(Basis& basis, const Molecule& mol, const ScfOptions& opt, const ScfResult& r) {
  const int n = basis.nbf(), natm = static_cast<int>(mol.atoms.size());
  const bool sp = basis.spherical();
  const bool closed = r.reference == Reference::Restricted;
  const FunctionalSpec fs = functional_by_name(opt.method);
  const auto& sh = basis.shells();
  std::vector<int> aoatom(n);
  for (const Shell& s : sh)
    for (int k = 0; k < s.nao; ++k) aoatom[s.ao0 + k] = s.atom;

  std::vector<double> g(3 * natm, 0.0);
  const Mat Dt = r.D_a + r.D_b;

  // Nuclear repulsion.
  for (int A = 0; A < natm; ++A)
    for (int B = 0; B < natm; ++B) {
      if (A == B) continue;
      double d[3], r2 = 0;
      for (int k = 0; k < 3; ++k) { d[k] = mol.atoms[A].x[k] - mol.atoms[B].x[k]; r2 += d[k] * d[k]; }
      const double f = -mol.atoms[A].Z * mol.atoms[B].Z / (r2 * std::sqrt(r2));
      for (int k = 0; k < 3; ++k) g[3 * A + k] += f * d[k];
    }

  // Energy-weighted density.  S^-1 F D + D F S^-1 is also right for ROHF,
  // where the orbitals are stationary only against the rotations that
  // change the energy.
  const Mat Si = r.S.inverse();
  Mat W = Mat::Zero(n, n);
  {
    const Mat Wa = Si * r.F_a * r.D_a, Wb = Si * r.F_b * r.D_b;
    W = 0.5 * (Wa + Wa.transpose() + Wb + Wb.transpose());
  }

  // Overlap, kinetic: with i on atom A, dS_ij/dA = -(grad i|j), and the same
  // for j, which doubles the symmetric sum.
  {
    Mat3 ds = one_e_deriv(basis, sp ? int1e_ipovlp_sph : int1e_ipovlp_cart);
    Mat3 dk = one_e_deriv(basis, sp ? int1e_ipkin_sph : int1e_ipkin_cart);
    for (int c = 0; c < 3; ++c)
      for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
          g[3 * aoatom[i] + c] += 2 * W(i, j) * ds[c](i, j) - 2 * Dt(i, j) * dk[c](i, j);
  }

  // Nuclear attraction, nucleus by nucleus: functions on A contribute
  // 2 Z_C D_ij (grad i|1/r_C|j); the operator centre C gets minus the sum of
  // all of them (translation invariance).
  for (int C = 0; C < natm; ++C) {
    for (int k = 0; k < 3; ++k) basis.env()[PTR_RINV_ORIG + k] = mol.atoms[C].x[k];
    Mat3 dv = one_e_deriv(basis, sp ? int1e_iprinv_sph : int1e_iprinv_cart);
    const double Z = mol.atoms[C].Z;
    for (int c = 0; c < 3; ++c) {
      double tot = 0;
      for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
          const double v = 2 * Z * Dt(i, j) * dv[c](i, j);
          g[3 * aoatom[i] + c] += v;
          tot += v;
        }
      g[3 * C + c] -= tot;
    }
  }

  // Two-electron terms.  The derivative acts on the first function of
  // (ij|kl); the four centres of a symmetric contraction are equivalent, so
  // dE/dA = -2 sum_{i on A} [ Dt_ij Dt_kl - exx sum_s D_ik D_jl ] (grad i j|kl).
  {
    const int ns = basis.nshl();
    const double exx = fs.exx;
    const std::array<const Mat*, 2> D = {&r.D_a, &r.D_b};
    CINTOpt* opt2 = nullptr;
    int2e_ip1_optimizer(&opt2, basis.atm().data(), basis.natm(), basis.bas().data(), ns, basis.env().data());
    Fn1* f = sp ? int2e_ip1_sph : int2e_ip1_cart;
    int maxn = 0;
    for (auto& s : sh) maxn = std::max(maxn, s.nao);
#pragma omp parallel
    {
      std::vector<double> gt(3 * natm, 0.0), buf(static_cast<size_t>(maxn) * maxn * maxn * maxn * 3), cache;
#pragma omp for schedule(dynamic, 2)
      for (int IJ = 0; IJ < ns * ns; ++IJ) {
        const int I = IJ / ns, J = IJ % ns;
        for (int K = 0; K < ns; ++K)
          for (int L = 0; L <= K; ++L) {
            int shls[4] = {I, J, K, L};
            size_t cs = f(nullptr, nullptr, shls, basis.atm().data(), basis.natm(), basis.bas().data(), ns, basis.env().data(), opt2, nullptr);
            if (cache.size() < cs + 16) cache.resize(cs + 16);
            const int ni = sh[I].nao, nj = sh[J].nao, nk = sh[K].nao, nl = sh[L].nao;
            if (!f(buf.data(), nullptr, shls, basis.atm().data(), basis.natm(), basis.bas().data(), ns, basis.env().data(), opt2, cache.data())) continue;
            const double wkl = K == L ? 1.0 : 2.0;
            const size_t blk = static_cast<size_t>(ni) * nj * nk * nl;
            for (int c = 0; c < 3; ++c) {
              double acc = 0;
              for (int l = 0; l < nl; ++l)
                for (int k = 0; k < nk; ++k)
                  for (int j = 0; j < nj; ++j)
                    for (int i = 0; i < ni; ++i) {
                      const int a = sh[I].ao0 + i, b = sh[J].ao0 + j, kk = sh[K].ao0 + k, ll = sh[L].ao0 + l;
                      double w = -2 * wkl * Dt(a, b) * Dt(kk, ll);
                      if (exx != 0.0) {
                        double ex = 0;
                        for (int s = 0; s < 2; ++s) {
                          const Mat& Ds = *D[s];
                          ex += Ds(a, kk) * Ds(b, ll);
                          if (K != L) ex += Ds(a, ll) * Ds(b, kk);
                        }
                        w += 2 * exx * ex;
                      }
                      acc += w * buf[c * blk + i + ni * (j + nj * (k + static_cast<size_t>(nk) * l))];
                    }
              gt[3 * sh[I].atom + c] += acc;
            }
          }
      }
#pragma omp critical
      for (int k = 0; k < 3 * natm; ++k) g[k] += gt[k];
    }
    CINTdel_optimizer(&opt2);
  }

  if (!fs.is_hf()) {
    XcIntegrator xc(basis, fs, make_grid(mol, opt.grid_radial, opt.grid_angular));
    std::vector<double> gx;
    xc.gradient(r.D_a, r.D_b, closed, gx);
    for (int k = 0; k < 3 * natm; ++k) g[k] += gx[k];
  }
  return g;
}

}  // namespace qm
