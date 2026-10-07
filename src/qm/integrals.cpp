#include "integrals.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#ifdef _OPENMP
#include <omp.h>
#endif

#include "cint_wrap.hpp"

namespace qm {

namespace {
using Fn1 = CINTIntegralFunction;

Mat one_e(Basis& b, Fn1* f, int comp, int which, double* origin = nullptr) {
  const int n = b.nbf();
  const int ns = b.nshl();
  if (origin) for (int k = 0; k < 3; ++k) b.env()[PTR_COMMON_ORIG + k] = origin[k];
  std::vector<Mat> out(comp, Mat::Zero(n, n));
  const auto& sh = b.shells();
  std::vector<double> buf, cache;
  for (int I = 0; I < ns; ++I)
    for (int J = 0; J <= I; ++J) {
      int shls[2] = {I, J};
      int ni = sh[I].nao, nj = sh[J].nao;
      buf.assign(static_cast<size_t>(ni) * nj * comp, 0.0);
      size_t cs = f(nullptr, nullptr, shls, b.atm().data(), b.natm(), b.bas().data(), ns, b.env().data(), nullptr, nullptr);
      cache.resize(cs + 16);
      f(buf.data(), nullptr, shls, b.atm().data(), b.natm(), b.bas().data(), ns, b.env().data(), nullptr, cache.data());
      for (int c = 0; c < comp; ++c)
        for (int j = 0; j < nj; ++j)
          for (int i = 0; i < ni; ++i) {
            double v = buf[c * ni * nj + j * ni + i];
            out[c](sh[I].ao0 + i, sh[J].ao0 + j) = v;
            out[c](sh[J].ao0 + j, sh[I].ao0 + i) = v;
          }
    }
  (void)which;
  return out[0].size() && comp == 1 ? out[0] : Mat();
}
}  // namespace

OneElectron one_electron(Basis& b) {
  OneElectron r;
  const bool sp = b.spherical();
  r.S = one_e(b, sp ? int1e_ovlp_sph : int1e_ovlp_cart, 1, 0);
  r.T = one_e(b, sp ? int1e_kin_sph : int1e_kin_cart, 1, 0);
  r.V = one_e(b, sp ? int1e_nuc_sph : int1e_nuc_cart, 1, 0);
  // dipole: three components in one call
  {
    const int n = b.nbf(), ns = b.nshl();
    double origin[3] = {0, 0, 0};
    for (int k = 0; k < 3; ++k) b.env()[PTR_COMMON_ORIG + k] = origin[k];
    for (auto& m : r.dip) m = Mat::Zero(n, n);
    Fn1* f = sp ? int1e_r_sph : int1e_r_cart;
    const auto& sh = b.shells();
    std::vector<double> buf, cache;
    for (int I = 0; I < ns; ++I)
      for (int J = 0; J <= I; ++J) {
        int shls[2] = {I, J};
        int ni = sh[I].nao, nj = sh[J].nao;
        buf.assign(static_cast<size_t>(ni) * nj * 3, 0.0);
        size_t cs = f(nullptr, nullptr, shls, b.atm().data(), b.natm(), b.bas().data(), ns, b.env().data(), nullptr, nullptr);
        cache.resize(cs + 16);
        f(buf.data(), nullptr, shls, b.atm().data(), b.natm(), b.bas().data(), ns, b.env().data(), nullptr, cache.data());
        for (int c = 0; c < 3; ++c)
          for (int j = 0; j < nj; ++j)
            for (int i = 0; i < ni; ++i) {
              double v = buf[c * ni * nj + j * ni + i];
              r.dip[c](sh[I].ao0 + i, sh[J].ao0 + j) = v;
              r.dip[c](sh[J].ao0 + j, sh[I].ao0 + i) = v;
            }
      }
  }
  return r;
}

double EriStore::bytes_needed(int n) {
  double np = n * (n + 1) / 2.0;
  return np * (np + 1) / 2.0 * 8.0;
}

static inline size_t pidx(size_t i, size_t j) { return i >= j ? i * (i + 1) / 2 + j : j * (j + 1) / 2 + i; }

double EriStore::get(int i, int j, int k, int l) const {
  size_t ij = pidx(i, j), kl = pidx(k, l);
  return v_[ij >= kl ? ij * (ij + 1) / 2 + kl : kl * (kl + 1) / 2 + ij];
}

EriStore::EriStore(Basis& b, double max_bytes) : n_(b.nbf()) {
  if (bytes_needed(n_) > max_bytes)
    throw Error("two-electron integrals need " + std::to_string(bytes_needed(n_) / 1e9) +
                " GB for " + std::to_string(n_) + " basis functions; molecule too large for the in-core engine");
  const size_t np = static_cast<size_t>(n_) * (n_ + 1) / 2;
  v_.assign(np * (np + 1) / 2, 0.0);
  const int ns = b.nshl();
  const auto& sh = b.shells();
  CINTOpt* opt = nullptr;
  int2e_optimizer(&opt, b.atm().data(), b.natm(), b.bas().data(), ns, b.env().data());
  Fn1* f = b.spherical() ? int2e_sph : int2e_cart;
  int maxn = 0;
  for (auto& s : sh) maxn = std::max(maxn, s.nao);
  const long nsp = static_cast<long>(ns) * (ns + 1) / 2;
  std::vector<int> spI(nsp), spJ(nsp);
  for (int I = 0, p = 0; I < ns; ++I)
    for (int J = 0; J <= I; ++J, ++p) { spI[p] = I; spJ[p] = J; }
#pragma omp parallel
  {
    std::vector<double> buf(static_cast<size_t>(maxn) * maxn * maxn * maxn), cache;
#pragma omp for schedule(dynamic, 4)
    for (long ij = 0; ij < nsp; ++ij) {
      int I = spI[ij], J = spJ[ij];
      for (long kl = 0; kl <= ij; ++kl) {
        int K = spI[kl], L = spJ[kl];
        int shls[4] = {I, J, K, L};
        size_t cs = f(nullptr, nullptr, shls, b.atm().data(), b.natm(), b.bas().data(), ns, b.env().data(), opt, nullptr);
        if (cache.size() < cs + 16) cache.resize(cs + 16);
        int ni = sh[I].nao, nj = sh[J].nao, nk = sh[K].nao, nl = sh[L].nao;
        int nz = f(buf.data(), nullptr, shls, b.atm().data(), b.natm(), b.bas().data(), ns, b.env().data(), opt, cache.data());
        if (!nz) continue;
        for (int l = 0; l < nl; ++l)
          for (int k = 0; k < nk; ++k)
            for (int j = 0; j < nj; ++j)
              for (int i = 0; i < ni; ++i) {
                size_t a = pidx(sh[I].ao0 + i, sh[J].ao0 + j), c = pidx(sh[K].ao0 + k, sh[L].ao0 + l);
                size_t idx = a >= c ? a * (a + 1) / 2 + c : c * (c + 1) / 2 + a;
                v_[idx] = buf[i + ni * (j + nj * (k + static_cast<size_t>(nk) * l))];
              }
      }
    }
  }
  CINTdel_optimizer(&opt);
}

void EriStore::jk(const std::vector<Mat>& D, std::vector<Mat>& J, std::vector<Mat>& K, bool want_k) const {
  const int nd = static_cast<int>(D.size());
  const size_t np = static_cast<size_t>(n_) * (n_ + 1) / 2;
  std::vector<int> pi(np), pj(np);
  for (int i = 0, p = 0; i < n_; ++i)
    for (int j = 0; j <= i; ++j, ++p) { pi[p] = i; pj[p] = j; }
  J.assign(nd, Mat::Zero(n_, n_));
  if (want_k) K.assign(nd, Mat::Zero(n_, n_)); else K.clear();
#pragma omp parallel
  {
    std::vector<Mat> Jt(nd, Mat::Zero(n_, n_)), Kt;
    if (want_k) Kt.assign(nd, Mat::Zero(n_, n_));
#pragma omp for schedule(dynamic, 8)
    for (long ij = 0; ij < static_cast<long>(np); ++ij) {
      const int i = pi[ij], j = pj[ij];
      const double* row = &v_[static_cast<size_t>(ij) * (ij + 1) / 2];
      for (long kl = 0; kl <= ij; ++kl) {
        double v = row[kl];
        if (v == 0.0) continue;
        const int k = pi[kl], l = pj[kl];
        if (i == j) v *= 0.5;
        if (k == l) v *= 0.5;
        if (ij == kl) v *= 0.5;
        for (int d = 0; d < nd; ++d) {
          const Mat& Dm = D[d];
          Jt[d](i, j) += 2.0 * Dm(k, l) * v;
          Jt[d](k, l) += 2.0 * Dm(i, j) * v;
          if (want_k) {
            Mat& Kd = Kt[d];
            Kd(i, k) += Dm(j, l) * v;
            Kd(j, k) += Dm(i, l) * v;
            Kd(i, l) += Dm(j, k) * v;
            Kd(j, l) += Dm(i, k) * v;
          }
        }
      }
    }
#pragma omp critical
    for (int d = 0; d < nd; ++d) {
      J[d] += Jt[d];
      if (want_k) K[d] += Kt[d];
    }
  }
  for (int d = 0; d < nd; ++d) {
    Mat Jd = J[d] + J[d].transpose();
    J[d] = Jd;
    if (want_k) {
      Mat Kd = K[d] + K[d].transpose();
      K[d] = Kd;
    }
  }
}

}  // namespace qm
