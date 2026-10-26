#include "hessian.hpp"
#include <algorithm>
#include <cmath>
#ifdef _OPENMP
#include <omp.h>
#endif
#include "cint_wrap.hpp"
#include "integrals.hpp"

namespace qm {

namespace {

using Fn = CINTIntegralFunction;
using MatV = std::vector<Mat>;

int thread_count() {
#ifdef _OPENMP
  return omp_get_max_threads();
#else
  return 1;
#endif
}

// Every AO pair (i, j), no symmetry, ncomp components: out[c](i, j).
MatV pair_ints(Basis& b, Fn* f, int ncomp) {
  const int n = b.nbf(), ns = b.nshl();
  MatV out(ncomp, Mat::Zero(n, n));
  const auto& sh = b.shells();
  std::vector<double> buf, cache;
  for (int I = 0; I < ns; ++I)
    for (int J = 0; J < ns; ++J) {
      int shls[2] = {I, J};
      const int ni = sh[I].nao, nj = sh[J].nao;
      buf.assign(static_cast<size_t>(ni) * nj * ncomp, 0.0);
      size_t cs = f(nullptr, nullptr, shls, b.atm().data(), b.natm(), b.bas().data(), ns, b.env().data(), nullptr, nullptr);
      cache.resize(cs + 16);
      f(buf.data(), nullptr, shls, b.atm().data(), b.natm(), b.bas().data(), ns, b.env().data(), nullptr, cache.data());
      for (int c = 0; c < ncomp; ++c)
        for (int j = 0; j < nj; ++j)
          for (int i = 0; i < ni; ++i) out[c](sh[I].ao0 + i, sh[J].ao0 + j) = buf[c * ni * nj + j * ni + i];
    }
  return out;
}

// out[3A+c] += d/d(A,c) of coef * <i|O|j>, from d[c] = (grad_c i|O|j).  A function
// on A moves by -grad; with opatom >= 0 the operator sits on that atom and, by
// translation invariance, takes minus the sum of the function derivatives.
void add_first(MatV& out, const MatV& d, double coef, const std::vector<int>& aoatom, int opatom) {
  const int n = static_cast<int>(aoatom.size());
  for (int c = 0; c < 3; ++c)
    for (int j = 0; j < n; ++j)
      for (int i = 0; i < n; ++i) {
        out[3 * aoatom[i] + c](i, j) += coef * d[c](i, j);
        out[3 * aoatom[j] + c](i, j) += coef * d[c](j, i);
        if (opatom >= 0) out[3 * opatom + c](i, j) -= coef * (d[c](i, j) + d[c](j, i));
      }
}

// Second derivatives of sum_ij w_ij <i|O|j> over every ordered AO pair.  ii[3*c1+c2]
// = (grad_c1 grad_c2 i|O|j), ij = (grad_c1 i|O|grad_c2 j); the pair (j, i) of
// the same loop supplies the derivatives on j.  With opatom >= 0 the
// operator's own derivatives follow from translation invariance.
void add_second(Mat& H, const Mat& w, const MatV& ii, const MatV& ij, const std::vector<int>& aoatom, int opatom) {
  const int n = static_cast<int>(aoatom.size());
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) {
      const double wij = 2.0 * w(i, j);  // the pair (j, i) of E supplies the same terms again
      if (wij == 0.0) continue;
      const int a = aoatom[i], b = aoatom[j];
      for (int c1 = 0; c1 < 3; ++c1)
        for (int c2 = 0; c2 < 3; ++c2) {
          const double vii = wij * ii[3 * c1 + c2](i, j), vij = wij * ij[3 * c1 + c2](i, j);
          H(3 * a + c1, 3 * a + c2) += vii;
          H(3 * a + c1, 3 * b + c2) += vij;
          if (opatom >= 0) {
            const double vji = wij * ij[3 * c2 + c1](i, j);
            const int C = opatom;
            H(3 * a + c1, 3 * C + c2) -= vii + vij;
            H(3 * C + c1, 3 * a + c2) -= vii + vji;
            H(3 * C + c1, 3 * C + c2) += vii + vij;
          }
        }
    }
}

}  // namespace

bool analytic_hessian_available(const ScfOptions& opt, const ScfResult& r) {
  // The analytic Hessian has no reaction-field term; with a solvent the
  // finite-difference one (of solvated gradients) is used.
  return opt.solvent_eps <= 0 && opt.method == "hf" && opt.reference == Reference::Restricted && r.reference == Reference::Restricted &&
         r.nalpha == r.nbeta && r.nalpha > 0 && opt.field[0] == 0 && opt.field[1] == 0 && opt.field[2] == 0;
}

Derivatives rhf_hessian(Basis& basis, const Molecule& mol, const ScfOptions& opt, const ScfResult& r) {
  if (!analytic_hessian_available(opt, r)) throw Error("analytic Hessian needs a closed-shell RHF reference");
  const int n = basis.nbf(), natm = static_cast<int>(mol.atoms.size()), P = 3 * natm, ns = basis.nshl();
  const int no = r.nalpha, nv = n - no;
  const bool sp = basis.spherical();
  const auto& sh = basis.shells();
  std::vector<int> aoatom(n);
  for (const Shell& s : sh)
    for (int k = 0; k < s.nao; ++k) aoatom[s.ao0 + k] = s.atom;

  const Mat Dt = r.D_a + r.D_b;
  const Mat W = 0.5 * Dt * r.F_a * Dt;  // energy-weighted density, = sum_i 2 eps_i c_i c_i^T
  const EriStore eri(basis);

  Mat H = Mat::Zero(P, P);

  // ---- nuclear repulsion ----
  for (int A = 0; A < natm; ++A)
    for (int B = 0; B < natm; ++B) {
      if (A == B) continue;
      double d[3], r2 = 0;
      for (int k = 0; k < 3; ++k) { d[k] = mol.atoms[A].x[k] - mol.atoms[B].x[k]; r2 += d[k] * d[k]; }
      const double rr = std::sqrt(r2), zz = mol.atoms[A].Z * mol.atoms[B].Z;
      for (int c1 = 0; c1 < 3; ++c1)
        for (int c2 = 0; c2 < 3; ++c2) {
          const double v = zz * (3 * d[c1] * d[c2] - (c1 == c2 ? r2 : 0.0)) / (r2 * r2 * rr);
          H(3 * A + c1, 3 * A + c2) += v;
          H(3 * A + c1, 3 * B + c2) -= v;
        }
    }

  // ---- one-electron derivative matrices (fixed orbitals) and skeleton second derivatives ----
  MatV Fx(P, Mat::Zero(n, n)), Sx(P, Mat::Zero(n, n));
  {
    add_first(Sx, pair_ints(basis, sp ? int1e_ipovlp_sph : int1e_ipovlp_cart, 3), -1.0, aoatom, -1);
    add_first(Fx, pair_ints(basis, sp ? int1e_ipkin_sph : int1e_ipkin_cart, 3), -1.0, aoatom, -1);
    add_second(H, -W, pair_ints(basis, sp ? int1e_ipipovlp_sph : int1e_ipipovlp_cart, 9),
               pair_ints(basis, sp ? int1e_ipovlpip_sph : int1e_ipovlpip_cart, 9), aoatom, -1);
    add_second(H, Dt, pair_ints(basis, sp ? int1e_ipipkin_sph : int1e_ipipkin_cart, 9),
               pair_ints(basis, sp ? int1e_ipkinip_sph : int1e_ipkinip_cart, 9), aoatom, -1);
    for (int C = 0; C < natm; ++C) {
      for (int k = 0; k < 3; ++k) basis.env()[PTR_RINV_ORIG + k] = mol.atoms[C].x[k];
      const double Z = mol.atoms[C].Z;
      add_first(Fx, pair_ints(basis, sp ? int1e_iprinv_sph : int1e_iprinv_cart, 3), Z, aoatom, C);
      add_second(H, -Z * Dt, pair_ints(basis, sp ? int1e_ipiprinv_sph : int1e_ipiprinv_cart, 9),
                 pair_ints(basis, sp ? int1e_iprinvip_sph : int1e_iprinvip_cart, 9), aoatom, C);
    }
  }

  // ---- two-electron: skeleton second derivatives and derivative Fock matrices ----
  // Over every ordered quartet, with the derivative on the first function and
  // Gamma = D_ij D_kl - (D_ik D_jl + D_il D_jk)/4 symmetric under the eight
  // permutations, the sum over centre pairs reduces to
  //   2 ipip1 -> (i,i), 2 ipvip1 -> (i,j), 4 ip1ip2 -> (i,k).
  {
    CINTOpt *o1 = nullptr, *o2 = nullptr, *o3 = nullptr, *o4 = nullptr;
    auto* at = basis.atm().data();
    auto* bs = basis.bas().data();
    auto* ev = basis.env().data();
    const int na = basis.natm();
    int2e_ip1_optimizer(&o1, at, na, bs, ns, ev);
    int2e_ipip1_optimizer(&o2, at, na, bs, ns, ev);
    int2e_ipvip1_optimizer(&o3, at, na, bs, ns, ev);
    int2e_ip1ip2_optimizer(&o4, at, na, bs, ns, ev);
    Fn* f1 = sp ? int2e_ip1_sph : int2e_ip1_cart;
    Fn* f2 = sp ? int2e_ipip1_sph : int2e_ipip1_cart;
    Fn* f3 = sp ? int2e_ipvip1_sph : int2e_ipvip1_cart;
    Fn* f4 = sp ? int2e_ip1ip2_sph : int2e_ip1ip2_cart;
    int maxn = 0;
    for (auto& s : sh) maxn = std::max(maxn, s.nao);
    const std::vector<double> Qd = schwarz_bounds(basis, true);
    std::vector<double> dm(static_cast<size_t>(ns) * ns, 0.0);
    for (int I = 0; I < ns; ++I)
      for (int J = 0; J < ns; ++J)
        dm[I * ns + J] = Dt.block(sh[I].ao0, sh[J].ao0, sh[I].nao, sh[J].nao).cwiseAbs().maxCoeff();
    const size_t maxblk = static_cast<size_t>(maxn) * maxn * maxn * maxn;
#pragma omp parallel
    {
      MatV g(P, Mat::Zero(n, n));
      Mat h = Mat::Zero(P, P);
      std::vector<double> b1(3 * maxblk), b2(9 * maxblk), b3(9 * maxblk), b4(9 * maxblk), cache;
#pragma omp for schedule(dynamic, 1)
      for (int IJ = 0; IJ < ns * ns; ++IJ) {
        const int I = IJ / ns, J = IJ % ns;
        for (int K = 0; K < ns; ++K)
          for (int L = 0; L < ns; ++L) {
            const double wmax = dm[I * ns + J] * dm[K * ns + L] +
                                0.5 * (dm[I * ns + K] * dm[J * ns + L] + dm[I * ns + L] * dm[J * ns + K]);
            // the second derivatives are not bounded by the first-derivative
            // bound, but sit within a small factor of it
            if (Qd[I * ns + J] * Qd[K * ns + L] * wmax < 0.01 * kSchwarzThreshold) continue;
            int shls[4] = {I, J, K, L};
            auto run = [&](Fn* f, CINTOpt* o, std::vector<double>& b) {
              size_t cs = f(nullptr, nullptr, shls, at, na, bs, ns, ev, o, nullptr);
              if (cache.size() < cs + 16) cache.resize(cs + 16);
              return f(b.data(), nullptr, shls, at, na, bs, ns, ev, o, cache.data()) != 0;
            };
            const bool k1 = run(f1, o1, b1), k2 = run(f2, o2, b2), k3 = run(f3, o3, b3), k4 = run(f4, o4, b4);
            if (!(k1 || k2 || k3 || k4)) continue;
            const int ni = sh[I].nao, nj = sh[J].nao, nk = sh[K].nao, nl = sh[L].nao;
            const int aI = sh[I].atom, aJ = sh[J].atom, aK = sh[K].atom;
            const size_t blk = static_cast<size_t>(ni) * nj * nk * nl;
            double acc2[9] = {0}, acc3[9] = {0}, acc4[9] = {0};
            for (int l = 0; l < nl; ++l)
              for (int k = 0; k < nk; ++k)
                for (int j = 0; j < nj; ++j)
                  for (int i = 0; i < ni; ++i) {
                    const int a = sh[I].ao0 + i, b = sh[J].ao0 + j, kk = sh[K].ao0 + k, ll = sh[L].ao0 + l;
                    const size_t idx = i + ni * (j + nj * (k + static_cast<size_t>(nk) * l));
                    const double dij = Dt(a, b), dkl = Dt(kk, ll);
                    const double gam = dij * dkl - 0.25 * (Dt(a, kk) * Dt(b, ll) + Dt(a, ll) * Dt(b, kk));
                    for (int q = 0; q < 9; ++q) {
                      if (k2) acc2[q] += gam * b2[q * blk + idx];
                      if (k3) acc3[q] += gam * b3[q * blk + idx];
                      if (k4) acc4[q] += gam * b4[q * blk + idx];
                    }
                    if (k1)
                      for (int c = 0; c < 3; ++c) {
                        const double v = -b1[c * blk + idx];  // centre derivative = -grad
                        Mat& G = g[3 * aI + c];
                        G(a, b) += dkl * v;
                        G(b, a) += dkl * v;
                        G(kk, ll) += dij * v;
                        G(ll, kk) += dij * v;
                        G(a, kk) -= 0.5 * Dt(b, ll) * v;
                        G(b, kk) -= 0.5 * Dt(a, ll) * v;
                        G(kk, a) -= 0.5 * Dt(ll, b) * v;
                        G(kk, b) -= 0.5 * Dt(ll, a) * v;
                      }
                  }
            for (int c1 = 0; c1 < 3; ++c1)
              for (int c2 = 0; c2 < 3; ++c2) {
                h(3 * aI + c1, 3 * aI + c2) += 2 * acc2[3 * c1 + c2];
                h(3 * aI + c1, 3 * aJ + c2) += 2 * acc3[3 * c1 + c2];
                h(3 * aI + c1, 3 * aK + c2) += 4 * acc4[3 * c1 + c2];
              }
          }
      }
#pragma omp critical
      {
        for (int p = 0; p < P; ++p) Fx[p] += g[p];
        H += h;
      }
    }
    CINTdel_optimizer(&o1);
    CINTdel_optimizer(&o2);
    CINTdel_optimizer(&o3);
    CINTdel_optimizer(&o4);
  }



  // ---- coupled-perturbed HF ----
  const Mat Co = r.C_a.leftCols(no), Cv = r.C_a.rightCols(nv);
  const Vec eo = r.eps_a.head(no), evir = r.eps_a.tail(nv);

  // G(D) = J(D) - K(D)/2 for symmetric AO densities, in chunks to bound the
  // per-thread J and K copies.
  const size_t chunk = std::max<size_t>(1, static_cast<size_t>(4e8 / (16.0 * n * n * thread_count())));
  auto gmat = [&](const MatV& Ds) {
    MatV out;
    out.reserve(Ds.size());
    for (size_t s = 0; s < Ds.size(); s += chunk) {
      const MatV part(Ds.begin() + s, Ds.begin() + std::min(Ds.size(), s + chunk));
      MatV J, K;
      eri.jk(part, J, K, true);
      for (size_t k = 0; k < part.size(); ++k) out.push_back(J[k] - 0.5 * K[k]);
    }
    return out;
  };
  auto vo_density = [&](const Mat& U) {
    const Mat t = Cv * U * Co.transpose();
    return Mat(2.0 * (t + t.transpose()));
  };

  MatV Dy(P);  // D^y in the AO basis
  {
    MatV Doo(P);
    for (int p = 0; p < P; ++p) Doo[p] = -2.0 * Co * (Co.transpose() * Sx[p] * Co) * Co.transpose();
    const MatV Goo = gmat(Doo);
    MatV R(P);  // -B, nv x no
    for (int p = 0; p < P; ++p)
      R[p] = -(Cv.transpose() * (Fx[p] + Goo[p]) * Co - Cv.transpose() * Sx[p] * Co * eo.asDiagonal());
    // Preconditioned conjugate gradients on (eps_a - eps_i) U + G_vo(U) = -B,
    // every perturbation in step so the integral passes are shared.
    Mat dE(nv, no);
    for (int a = 0; a < nv; ++a)
      for (int i = 0; i < no; ++i) dE(a, i) = evir(a) - eo(i);
    MatV X(P, Mat::Zero(nv, no)), Z(P), D(P);
    std::vector<double> rz(P, 0.0);
    std::vector<char> active(P, 1);
    for (int p = 0; p < P; ++p) {
      Z[p] = R[p].cwiseQuotient(dE);
      D[p] = Z[p];
      rz[p] = R[p].cwiseProduct(Z[p]).sum();
    }
    for (int it = 0;; ++it) {
      std::vector<int> act;
      for (int p = 0; p < P; ++p) {
        if (active[p] && R[p].cwiseAbs().maxCoeff() < 1e-10) active[p] = 0;
        if (active[p]) act.push_back(p);
      }
      if (act.empty()) break;
      if (it >= 100) throw Error("coupled-perturbed HF did not converge");
      MatV Ds;
      for (int p : act) Ds.push_back(vo_density(D[p]));
      const MatV Gd = gmat(Ds);
      for (size_t q = 0; q < act.size(); ++q) {
        const int p = act[q];
        const Mat AD = D[p].cwiseProduct(dE) + Cv.transpose() * Gd[q] * Co;
        const double alpha = rz[p] / D[p].cwiseProduct(AD).sum();
        X[p] += alpha * D[p];
        R[p] -= alpha * AD;
        Z[p] = R[p].cwiseQuotient(dE);
        const double rzn = R[p].cwiseProduct(Z[p]).sum();
        D[p] = Z[p] + (rzn / rz[p]) * D[p];
        rz[p] = rzn;
      }
    }
    for (int p = 0; p < P; ++p) Dy[p] = Doo[p] + vo_density(X[p]);
  }

  // ---- response terms: sum D^y F^x - sum W^y S^x, with W = D F D / 2 ----
  const MatV Gy = gmat(Dy);
  MatV Wy(P);
  for (int p = 0; p < P; ++p) {
    const Mat Fy = Fx[p] + Gy[p];
    Wy[p] = 0.5 * (Dy[p] * r.F_a * Dt + Dt * Fy * Dt + Dt * r.F_a * Dy[p]);
  }
  Mat Hr = Mat::Zero(P, P);
#pragma omp parallel for schedule(static)
  for (int y = 0; y < P; ++y)
    for (int x = 0; x < P; ++x) Hr(x, y) = Dy[y].cwiseProduct(Fx[x]).sum() - Wy[y].cwiseProduct(Sx[x]).sum();
  H += Hr;

  Derivatives d;
  d.hessian = 0.5 * (H + H.transpose());

  // ---- dipole derivatives: nuclear charge, density response, and the basis
  // functions moving with the nucleus (fixed density) ----
  {
    const OneElectron oe = one_electron(basis);
    for (int k = 0; k < 3; ++k) basis.env()[PTR_COMMON_ORIG + k] = 0.0;
    const MatV irp = pair_ints(basis, sp ? int1e_irp_sph : int1e_irp_cart, 9);
    d.dmu = Mat::Zero(3, P);
    for (int A = 0; A < natm; ++A)
      for (int k = 0; k < 3; ++k) {
        const int p = 3 * A + k;
        for (int c = 0; c < 3; ++c) {
          double fix = 0;
          for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j) {
              const double dij = Dt(i, j), t = irp[3 * c + k](i, j);
              if (aoatom[i] == A) fix += dij * ((c == k ? r.S(i, j) : 0.0) + t);
              if (aoatom[j] == A) fix -= dij * t;
            }
          d.dmu(c, p) = mol.atoms[A].Z * (c == k ? 1.0 : 0.0) - Dy[p].cwiseProduct(oe.dip[c]).sum() - fix;
        }
      }
  }
  return d;
}

}  // namespace qm
