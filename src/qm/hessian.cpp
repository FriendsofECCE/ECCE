#include "hessian.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#ifdef _OPENMP
#include <omp.h>
#endif
#include "cint_wrap.hpp"
#include "d3.hpp"
#include "grid.hpp"
#include "integrals.hpp"
#include "solvent.hpp"
#include "xc.hpp"

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
  // Kohn-Sham: LDA, GGA and global hybrids; IVO virtuals are not canonical,
  // which the CPKS denominators assume.  ROHF/ROKS orbitals are not
  // eigenvectors of one Fock matrix per spin, so the per-spin response
  // equations below do not apply to them.  A field has no derivative terms here.
  bool fn_ok = false;
  try { fn_ok = xc_analytic_hessian_supported(functional_by_name(opt.method)); } catch (const Error&) {}
  const bool closed = opt.reference == Reference::Restricted && r.reference == Reference::Restricted && r.nalpha == r.nbeta;
  const bool unrestricted = opt.reference == Reference::Unrestricted && r.reference == Reference::Unrestricted;
  return fn_ok && !opt.ivo && !r.ivo_applied && (closed || unrestricted) && r.nalpha > 0 && r.nbeta > 0 &&
         opt.field[0] == 0 && opt.field[1] == 0 && opt.field[2] == 0;
}

Derivatives rhf_hessian(Basis& basis, const Molecule& mol, const ScfOptions& opt, const ScfResult& r,
                        const HessianProgress& on_progress) {
  if (!analytic_hessian_available(opt, r))
    throw Error("analytic Hessian needs an RHF/UHF or RKS/UKS reference (LDA, GGA or global hybrid) and no field");
  const FunctionalSpec fs = functional_by_name(opt.method);
  const double exx = fs.exx;  // 1 for HF, the hybrid's fraction, 0 for a pure functional
  const int n = basis.nbf(), natm = static_cast<int>(mol.atoms.size()), P = 3 * natm, ns = basis.nshl();
  const bool sp = basis.spherical();
  const auto& sh = basis.shells();
  std::vector<int> aoatom(n);
  for (const Shell& s : sh)
    for (int k = 0; k < s.nao; ++k) aoatom[s.ao0 + k] = s.atom;

  // Spin channels: a closed shell has one (the alpha orbitals standing for
  // both spins, weight 2), an unrestricted reference alpha and beta (weight 1).
  // Every per-spin quantity below is that of one spin; totals sum wch times it.
  const int nch = r.reference == Reference::Unrestricted ? 2 : 1;
  const double wch = nch == 2 ? 1.0 : 2.0;
  const Mat* Dc[2] = {&r.D_a, &r.D_b};
  const Mat* Fc[2] = {&r.F_a, &r.F_b};
  const Mat* Cc[2] = {&r.C_a, &r.C_b};
  const Vec* Ec[2] = {&r.eps_a, &r.eps_b};
  const int noc[2] = {r.nalpha, r.nbeta};

  const Mat Dt = r.D_a + r.D_b;
  Mat W = Mat::Zero(n, n);  // energy-weighted density, sum over spins of sum_i eps_i c_i c_i^T
  for (int c = 0; c < nch; ++c) W += wch * (*Dc[c]) * (*Fc[c]) * (*Dc[c]);
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
  MatV hx(P, Mat::Zero(n, n)), Sx(P, Mat::Zero(n, n));
  {
    add_first(Sx, pair_ints(basis, sp ? int1e_ipovlp_sph : int1e_ipovlp_cart, 3), -1.0, aoatom, -1);
    add_first(hx, pair_ints(basis, sp ? int1e_ipkin_sph : int1e_ipkin_cart, 3), -1.0, aoatom, -1);
    add_second(H, -W, pair_ints(basis, sp ? int1e_ipipovlp_sph : int1e_ipipovlp_cart, 9),
               pair_ints(basis, sp ? int1e_ipovlpip_sph : int1e_ipovlpip_cart, 9), aoatom, -1);
    add_second(H, Dt, pair_ints(basis, sp ? int1e_ipipkin_sph : int1e_ipipkin_cart, 9),
               pair_ints(basis, sp ? int1e_ipkinip_sph : int1e_ipkinip_cart, 9), aoatom, -1);
    for (int C = 0; C < natm; ++C) {
      for (int k = 0; k < 3; ++k) basis.env()[PTR_RINV_ORIG + k] = mol.atoms[C].x[k];
      const double Z = mol.atoms[C].Z;
      add_first(hx, pair_ints(basis, sp ? int1e_iprinv_sph : int1e_iprinv_cart, 3), Z, aoatom, C);
      add_second(H, -Z * Dt, pair_ints(basis, sp ? int1e_ipiprinv_sph : int1e_ipiprinv_cart, 9),
                 pair_ints(basis, sp ? int1e_iprinvip_sph : int1e_iprinvip_cart, 9), aoatom, C);
    }
  }
  // Fx[c][p]: derivative Fock matrix of spin channel c at fixed orbitals
  std::vector<MatV> Fx(nch, hx);

  // ---- two-electron: skeleton second derivatives and derivative Fock matrices ----
  // Over every ordered quartet, with the derivative on the first function and
  // Gamma = D_ij D_kl - exx/2 sum_spin (D_ik D_jl + D_il D_jk) symmetric under
  // the eight permutations, the sum over centre pairs reduces to
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
    // dm: |total density| per shell pair; dk: the largest |wch D_spin| (exchange)
    std::vector<double> dm(static_cast<size_t>(ns) * ns, 0.0), dk(dm.size(), 0.0);
    for (int I = 0; I < ns; ++I)
      for (int J = 0; J < ns; ++J) {
        dm[I * ns + J] = Dt.block(sh[I].ao0, sh[J].ao0, sh[I].nao, sh[J].nao).cwiseAbs().maxCoeff();
        for (int c = 0; c < nch; ++c)
          dk[I * ns + J] = std::max(dk[I * ns + J], wch * Dc[c]->block(sh[I].ao0, sh[J].ao0, sh[I].nao, sh[J].nao).cwiseAbs().maxCoeff());
      }
    const size_t maxblk = static_cast<size_t>(maxn) * maxn * maxn * maxn;
#pragma omp parallel
    {
      MatV gj(P, Mat::Zero(n, n));
      std::vector<MatV> gk(exx != 0.0 ? nch : 0, MatV(P, Mat::Zero(n, n)));
      Mat h = Mat::Zero(P, P);
      std::vector<double> b1(3 * maxblk), b2(9 * maxblk), b3(9 * maxblk), b4(9 * maxblk), cache;
#pragma omp for schedule(dynamic, 1)
      for (int IJ = 0; IJ < ns * ns; ++IJ) {
        const int I = IJ / ns, J = IJ % ns;
        for (int K = 0; K < ns; ++K)
          for (int L = 0; L < ns; ++L) {
            const double wmax = dm[I * ns + J] * dm[K * ns + L] +
                                0.5 * exx * (dk[I * ns + K] * dk[J * ns + L] + dk[I * ns + L] * dk[J * ns + K]);
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
                    double xg = 0;
                    for (int c = 0; c < nch; ++c) {
                      const Mat& D = *Dc[c];
                      xg += wch * (D(a, kk) * D(b, ll) + D(a, ll) * D(b, kk));
                    }
                    const double gam = dij * dkl - 0.5 * exx * xg;
                    for (int q = 0; q < 9; ++q) {
                      if (k2) acc2[q] += gam * b2[q * blk + idx];
                      if (k3) acc3[q] += gam * b3[q * blk + idx];
                      if (k4) acc4[q] += gam * b4[q * blk + idx];
                    }
                    if (k1)
                      for (int c = 0; c < 3; ++c) {
                        const double v = -b1[c * blk + idx];  // centre derivative = -grad
                        Mat& G = gj[3 * aI + c];
                        G(a, b) += dkl * v;
                        G(b, a) += dkl * v;
                        G(kk, ll) += dij * v;
                        G(ll, kk) += dij * v;
                        if (exx == 0.0) continue;
                        const double xv = exx * v;
                        for (int s = 0; s < nch; ++s) {
                          const Mat& D = *Dc[s];
                          Mat& X = gk[s][3 * aI + c];
                          X(a, kk) -= D(b, ll) * xv;
                          X(b, kk) -= D(a, ll) * xv;
                          X(kk, a) -= D(ll, b) * xv;
                          X(kk, b) -= D(ll, a) * xv;
                        }
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
        for (int c = 0; c < nch; ++c)
          for (int p = 0; p < P; ++p) {
            Fx[c][p] += gj[p];
            if (exx != 0.0) Fx[c][p] += gk[c][p];
          }
        H += h;
      }
    }
    CINTdel_optimizer(&o1);
    CINTdel_optimizer(&o2);
    CINTdel_optimizer(&o3);
    CINTdel_optimizer(&o4);
  }

  // ---- exchange-correlation: skeleton second derivatives and Vxc derivatives
  // at fixed density; the grid is that of the SCF and does not move.  A closed
  // shell goes through libxc unpolarised with the total density. ----
  std::unique_ptr<XcIntegrator> xc;
  const std::vector<Mat> Pxc = nch == 2 ? std::vector<Mat>{r.D_a, r.D_b} : std::vector<Mat>{Dt};
  if (!fs.is_hf()) {
    xc.reset(new XcIntegrator(basis, fs, make_grid(mol, opt.grid_radial, opt.grid_angular, opt.grid_prune)));
    std::vector<MatV> Vx(nch, MatV(P, Mat::Zero(n, n)));
    xc->hessian_terms(Pxc, H, Vx);
    for (int c = 0; c < nch; ++c)
      for (int p = 0; p < P; ++p) Fx[c][p] += Vx[c][p];
  }


  // ---- CPCM reaction field (scf_gradient's model: points ride with their
  // atom, exponents fixed, the switching functions follow every atom).  With
  // E_pol = q.V/2, A q = -f V, the fixed-density second derivative is
  // q.V'' + q.A''.q/(2f) - u_x.A^-1.u_y/f, u = f V' + A' q, and the
  // derivative Fock matrix gains -sum_k (q'_k L_k + q_k L'_k), q' = -A^-1 u. ----
  std::unique_ptr<CpcmField> pcm;
  if (opt.solvent_eps > 0) {
    pcm.reset(new CpcmField(basis, mol, opt.solvent_eps, opt.cavity_scale));
    const Cpcm& cav = pcm->cavity();
    const int np = cav.size();
    const double f = cav.factor();
    const Vec q = cav.charges(pcm->potential(Dt));
    cav.add_hessian(mol, q, H);
    Mat U = cav.first_derivatives(mol, q);
    MatV Fsx(P, Mat::Zero(n, n));
    // Per atom: the q-weighted operator derivatives of its points, and every
    // point's electronic potential derivative (into U).
    std::vector<std::vector<int>> on(natm);
    for (int k = 0; k < np; ++k) on[cav.atom_of(k)].push_back(k);
    Fn* fi1 = sp ? int1e_iprinv_sph : int1e_iprinv_cart;
    Fn* fi2 = sp ? int1e_ipiprinv_sph : int1e_ipiprinv_cart;
    Fn* fi3 = sp ? int1e_iprinvip_sph : int1e_iprinvip_cart;
    for (int C = 0; C < natm; ++C) {
      if (on[C].empty()) continue;
      MatV L1(3, Mat::Zero(n, n)), Lii(9, Mat::Zero(n, n)), Lij(9, Mat::Zero(n, n));
#pragma omp parallel
      {
        std::vector<double> env = basis.env(), b1, b2, b3, cache;
        MatV l1(3, Mat::Zero(n, n)), lii(9, Mat::Zero(n, n)), lij(9, Mat::Zero(n, n));
        auto run = [&](Fn* fn, int* shls, std::vector<double>& b, size_t len) {
          b.assign(len, 0.0);
          const size_t cs = fn(nullptr, nullptr, shls, basis.atm().data(), basis.natm(), basis.bas().data(), ns, env.data(), nullptr, nullptr);
          if (cache.size() < cs + 16) cache.resize(cs + 16);
          fn(b.data(), nullptr, shls, basis.atm().data(), basis.natm(), basis.bas().data(), ns, env.data(), nullptr, cache.data());
        };
#pragma omp for schedule(dynamic, 4)
        for (size_t t = 0; t < on[C].size(); ++t) {
          const int k = on[C][t];
          const double qk = q(k);
          for (int c = 0; c < 3; ++c) env[PTR_RINV_ORIG + c] = cav.points()[k][c];
          double tot[3] = {0, 0, 0};
          for (int I = 0; I < ns; ++I)
            for (int J = 0; J < ns; ++J) {
              int shls[2] = {I, J};
              const int ni = sh[I].nao, nj = sh[J].nao, nn = ni * nj;
              run(fi1, shls, b1, 3 * nn);
              run(fi2, shls, b2, 9 * nn);
              run(fi3, shls, b3, 9 * nn);
              const int aI = sh[I].atom;
              for (int j = 0; j < nj; ++j)
                for (int i = 0; i < ni; ++i) {
                  const int a = sh[I].ao0 + i, b = sh[J].ao0 + j, x = j * ni + i;
                  for (int c = 0; c < 3; ++c) {
                    const double v = b1[c * nn + x];
                    l1[c](a, b) += qk * v;
                    const double dv = 2.0 * Dt(a, b) * v;  // dV_el,k / d(atom of a)
                    U(k, 3 * aI + c) += f * dv;
                    tot[c] += dv;
                  }
                  for (int c = 0; c < 9; ++c) {
                    lii[c](a, b) += qk * b2[c * nn + x];
                    lij[c](a, b) += qk * b3[c * nn + x];
                  }
                }
            }
          for (int c = 0; c < 3; ++c) U(k, 3 * C + c) -= f * tot[c];
        }
#pragma omp critical
        {
          for (int c = 0; c < 3; ++c) L1[c] += l1[c];
          for (int c = 0; c < 9; ++c) { Lii[c] += lii[c]; Lij[c] += lij[c]; }
        }
      }
      add_first(Fsx, L1, 1.0, aoatom, C);
      add_second(H, -Dt, Lii, Lij, aoatom, C);
    }
    const Mat Z = cav.solve(U);  // -q'
    H -= U.transpose() * Z / f;
    const MatV Fq = pcm->fock_many(Z);  // -sum_k Z_k L_k
    for (int p = 0; p < P; ++p) Fsx[p] -= Fq[p];
    for (int c = 0; c < nch; ++c)
      for (int p = 0; p < P; ++p) Fx[c][p] += Fsx[p];
  }

  // ---- coupled-perturbed HF / KS ----
  Mat Co[2], Cv[2], dE[2];
  Vec eo[2];
  for (int c = 0; c < nch; ++c) {
    const int no = noc[c], nv = n - no;
    Co[c] = Cc[c]->leftCols(no);
    Cv[c] = Cc[c]->rightCols(nv);
    eo[c] = Ec[c]->head(no);
    const Vec ev = Ec[c]->tail(nv);
    dE[c].resize(nv, no);
    for (int a = 0; a < nv; ++a)
      for (int i = 0; i < no; ++i) dE[c](a, i) = ev(a) - eo[c](i);
  }

  // G_c(D) = J(D_total) - exx K(D_c) + the XC kernel's response, for the
  // per-spin first-order densities Ds[c][k] (symmetric), in chunks to bound
  // the per-thread J and K copies.
  const size_t chunk = std::max<size_t>(1, static_cast<size_t>(4e8 / (16.0 * n * n * thread_count() * nch)));
  auto gmat = [&](const std::vector<MatV>& Ds) {
    const size_t nd = Ds[0].size();
    std::vector<MatV> out(nch);
    for (size_t s = 0; s < nd; s += chunk) {
      const size_t e = std::min(nd, s + chunk), m = e - s;
      MatV part;
      for (int c = 0; c < nch; ++c) part.insert(part.end(), Ds[c].begin() + s, Ds[c].begin() + e);
      MatV J, K;
      eri.jk(part, J, K, exx != 0.0);
      MatV Rs;  // reaction field of the total first-order density, alike for both spins
      if (pcm) {
        MatV Dt1(m, Mat::Zero(n, n));
        for (int c = 0; c < nch; ++c)
          for (size_t k = 0; k < m; ++k) Dt1[k] += wch * part[c * m + k];
        Rs = pcm->response(Dt1);
      }
      std::vector<MatV> X;
      if (xc) {
        std::vector<MatV> sub(nch);
        for (int c = 0; c < nch; ++c)
          for (size_t k = 0; k < m; ++k) sub[c].push_back(nch == 2 ? part[c * m + k] : Mat(wch * part[k]));
        X = xc->kernel(Pxc, sub);
      }
      for (size_t k = 0; k < m; ++k) {
        Mat jt = Mat::Zero(n, n);
        for (int c = 0; c < nch; ++c) jt += wch * J[c * m + k];
        for (int c = 0; c < nch; ++c) {
          Mat g = jt;
          if (exx != 0.0) g -= exx * K[c * m + k];
          if (xc) g += X[c][k];
          if (pcm) g += Rs[k];
          out[c].push_back(g);
        }
      }
    }
    return out;
  };
  auto vo_density = [&](int c, const Mat& U) {
    const Mat t = Cv[c] * U * Co[c].transpose();
    return Mat(t + t.transpose());
  };

  std::vector<MatV> Dy(nch, MatV(P));  // per-spin D^y in the AO basis
  {
    std::vector<MatV> Doo(nch, MatV(P));
    for (int c = 0; c < nch; ++c)
      for (int p = 0; p < P; ++p) Doo[c][p] = -Co[c] * (Co[c].transpose() * Sx[p] * Co[c]) * Co[c].transpose();
    const std::vector<MatV> Goo = gmat(Doo);
    // Preconditioned conjugate gradients on (eps_a - eps_i) U + G_vo(U) = -B,
    // both spins of a perturbation as one vector, every perturbation in step
    // so the integral passes are shared.
    std::vector<MatV> R(nch, MatV(P)), X(nch, MatV(P)), Z(nch, MatV(P)), D(nch, MatV(P));
    std::vector<double> rz(P, 0.0);
    std::vector<char> active(P, 1);
    for (int c = 0; c < nch; ++c)
      for (int p = 0; p < P; ++p) {
        R[c][p] = -(Cv[c].transpose() * (Fx[c][p] + Goo[c][p]) * Co[c] - Cv[c].transpose() * Sx[p] * Co[c] * eo[c].asDiagonal());
        X[c][p] = Mat::Zero(dE[c].rows(), dE[c].cols());
        Z[c][p] = R[c][p].cwiseQuotient(dE[c]);
        D[c][p] = Z[c][p];
        rz[p] += R[c][p].cwiseProduct(Z[c][p]).sum();
      }
    for (int it = 0;; ++it) {
      std::vector<int> act;
      for (int p = 0; p < P; ++p) {
        double rmax = 0;
        for (int c = 0; c < nch; ++c) rmax = std::max(rmax, R[c][p].size() ? R[c][p].cwiseAbs().maxCoeff() : 0.0);
        if (active[p] && rmax < 1e-10) active[p] = 0;
        if (active[p]) act.push_back(p);
      }
      if (on_progress) on_progress(P - static_cast<int>(act.size()), P, it, false);
      if (act.empty()) break;
      if (it >= 100) throw Error("coupled-perturbed SCF did not converge");
      std::vector<MatV> Ds(nch);
      for (int c = 0; c < nch; ++c)
        for (int p : act) Ds[c].push_back(vo_density(c, D[c][p]));
      const std::vector<MatV> Gd = gmat(Ds);
      for (size_t q = 0; q < act.size(); ++q) {
        const int p = act[q];
        Mat AD[2];
        double dad = 0;
        for (int c = 0; c < nch; ++c) {
          AD[c] = D[c][p].cwiseProduct(dE[c]) + Cv[c].transpose() * Gd[c][q] * Co[c];
          dad += D[c][p].cwiseProduct(AD[c]).sum();
        }
        const double alpha = rz[p] / dad;
        double rzn = 0;
        for (int c = 0; c < nch; ++c) {
          X[c][p] += alpha * D[c][p];
          R[c][p] -= alpha * AD[c];
          Z[c][p] = R[c][p].cwiseQuotient(dE[c]);
          rzn += R[c][p].cwiseProduct(Z[c][p]).sum();
        }
        for (int c = 0; c < nch; ++c) D[c][p] = Z[c][p] + (rzn / rz[p]) * D[c][p];
        rz[p] = rzn;
      }
    }
    for (int c = 0; c < nch; ++c)
      for (int p = 0; p < P; ++p) Dy[c][p] = Doo[c][p] + vo_density(c, X[c][p]);
  }

  if (on_progress) on_progress(P, P, 0, true);
  // ---- response terms: sum over spins of D^y F^x - W^y S^x, with W = D F D ----
  const std::vector<MatV> Gy = gmat(Dy);
  std::vector<MatV> Wy(nch, MatV(P));
  for (int c = 0; c < nch; ++c) {
    const Mat &Ds = *Dc[c], &F = *Fc[c];
    for (int p = 0; p < P; ++p) {
      const Mat Fy = Fx[c][p] + Gy[c][p];
      Wy[c][p] = Dy[c][p] * F * Ds + Ds * Fy * Ds + Ds * F * Dy[c][p];
    }
  }
  Mat Hr = Mat::Zero(P, P);
#pragma omp parallel for schedule(static)
  for (int y = 0; y < P; ++y)
    for (int x = 0; x < P; ++x) {
      double v = 0;
      for (int c = 0; c < nch; ++c) v += wch * (Dy[c][y].cwiseProduct(Fx[c][x]).sum() - Wy[c][y].cwiseProduct(Sx[x]).sum());
      Hr(x, y) = v;
    }
  H += Hr;

  // ---- D3(BJ): central differences of its analytic gradient (no SCF, cheap
  // and accurate to ~1e-9 at this step). ----
  D3Params dp;
  if (!opt.dispersion.empty() && d3bj_params(opt.method, dp)) {
    const double h = 1e-4;
    Molecule mm = mol;
    for (int p = 0; p < P; ++p) {
      std::vector<double> gp(P, 0.0), gm(P, 0.0);
      mm.atoms[p / 3].x[p % 3] = mol.atoms[p / 3].x[p % 3] + h;
      d3bj_energy(mm, dp, &gp);
      mm.atoms[p / 3].x[p % 3] = mol.atoms[p / 3].x[p % 3] - h;
      d3bj_energy(mm, dp, &gm);
      mm.atoms[p / 3].x[p % 3] = mol.atoms[p / 3].x[p % 3];
      for (int x = 0; x < P; ++x) H(x, p) += (gp[x] - gm[x]) / (2 * h);
    }
  }

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
        Mat Dyt = Mat::Zero(n, n);
        for (int c = 0; c < nch; ++c) Dyt += wch * Dy[c][p];
        for (int c = 0; c < 3; ++c) {
          double fix = 0;
          for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j) {
              const double dij = Dt(i, j), t = irp[3 * c + k](i, j);
              if (aoatom[i] == A) fix += dij * ((c == k ? r.S(i, j) : 0.0) + t);
              if (aoatom[j] == A) fix -= dij * t;
            }
          d.dmu(c, p) = mol.atoms[A].Z * (c == k ? 1.0 : 0.0) - Dyt.cwiseProduct(oe.dip[c]).sum() - fix;
        }
      }
  }
  return d;
}

}  // namespace qm
