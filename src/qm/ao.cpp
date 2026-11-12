#include "ao.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include <map>
#include <mutex>

#include "cint_wrap.hpp"

namespace qm {

namespace {
// Cartesian -> spherical matrix (ncart x nsph, column-major as libcint
// returns it) taken from libcint so both agree on order, sign and scale.
const Mat& c2s(int l) {
  static std::map<int, Mat> cache;
  static std::mutex mu;
  std::lock_guard<std::mutex> lk(mu);
  auto it = cache.find(l);
  if (it != cache.end()) return it->second;
  const int nc = (l + 1) * (l + 2) / 2, ns = 2 * l + 1;
  Mat id = Mat::Identity(nc, nc);
  Mat out = Mat::Zero(nc, ns);
  CINTc2s_ket_sph1(out.data(), id.data(), nc, nc, l);
  return cache.emplace(l, out).first->second;
}
}  // namespace

int ao_d3_index(int i, int j, int k) {
  if (i > j) std::swap(i, j);
  if (j > k) std::swap(j, k);
  if (i > j) std::swap(i, j);
  static const int idx[3][3][3] = {{{0, 1, 2}, {0, 3, 4}, {0, 0, 5}}, {{0, 0, 0}, {0, 6, 7}, {0, 0, 8}}, {{0, 0, 0}, {0, 0, 0}, {0, 0, 9}}};
  return idx[i][j][k];
}

void eval_ao(const Basis& b, const double* xyz, int n, int deriv, AoValues& out, const std::vector<int>* shells) {
  int nb = b.nbf();
  if (shells) {
    nb = 0;
    for (int k : *shells) nb += b.shells()[k].nao;
  }
  out.phi = Mat::Zero(n, nb);
  if (deriv >= 1) {
    out.dx = Mat::Zero(n, nb);
    out.dy = Mat::Zero(n, nb);
    out.dz = Mat::Zero(n, nb);
  }
  if (deriv >= 2) {
    for (Mat* m : {&out.dxx, &out.dxy, &out.dxz, &out.dyy, &out.dyz, &out.dzz}) *m = Mat::Zero(n, nb);
  }
  if (deriv >= 3)
    for (Mat& m : out.d3) m = Mat::Zero(n, nb);
  static const int trip[10][3] = {{0, 0, 0}, {0, 0, 1}, {0, 0, 2}, {0, 1, 1}, {0, 1, 2},
                                  {0, 2, 2}, {1, 1, 1}, {1, 1, 2}, {1, 2, 2}, {2, 2, 2}};
  const bool sph = b.spherical();
  const int nsel = shells ? static_cast<int>(shells->size()) : b.nshl();
  int col = 0;
  for (int is = 0; is < nsel; ++is) {
    const Shell& s = b.shells()[shells ? (*shells)[is] : is];
    const int col0 = col;
    col += s.nao;
    const int l = s.l, nc = (l + 1) * (l + 2) / 2;
    const double fac = l == 0 ? 0.282094791773878143 : (l == 1 ? 0.488602511902919921 : 1.0);
    // list of (lx,ly,lz) in libcint order
    std::vector<std::array<int, 3>> pw;
    for (int lx = l; lx >= 0; --lx)
      for (int ly = l - lx; ly >= 0; --ly) pw.push_back({lx, ly, l - lx - ly});
    const int np = static_cast<int>(s.exps.size());
    const Mat* T = (sph && l >= 2) ? &c2s(l) : nullptr;
    std::vector<double> cv(nc), cgx(nc), cgy(nc), cgz(nc);
    std::vector<std::array<double, 6>> ch(nc);  // xx xy xz yy yz zz
    std::vector<std::array<double, 10>> c3(nc);
    for (int g = 0; g < n; ++g) {
      const double x = xyz[3 * g] - s.center[0], y = xyz[3 * g + 1] - s.center[1], z = xyz[3 * g + 2] - s.center[2];
      const double r2 = x * x + y * y + z * z;
      if (r2 > s.extent * s.extent) continue;
      double R = 0, Rp = 0, Rpp = 0, Rppp = 0;  // R and its derivatives with respect to r^2
      for (int p = 0; p < np; ++p) {
        double e = s.coefs[p] * std::exp(-s.exps[p] * r2);
        R += e;
        Rp -= s.exps[p] * e;
        Rpp += s.exps[p] * s.exps[p] * e;
        Rppp -= s.exps[p] * s.exps[p] * s.exps[p] * e;
      }
      R *= fac;
      Rp *= fac;
      Rpp *= fac;
      Rppp *= fac;
      const double xv[3] = {x, y, z};
      auto ipow = [](double v, int k) { double r = 1; for (int i = 0; i < k; ++i) r *= v; return r; };
      for (int c = 0; c < nc; ++c) {
        const int a = pw[c][0], bb = pw[c][1], cc = pw[c][2];
        const double xa = ipow(x, a), yb = ipow(y, bb), zc = ipow(z, cc);
        const double mono = xa * yb * zc;
        cv[c] = mono * R;
        if (deriv >= 1) {
          cgx[c] = (a ? a * ipow(x, a - 1) * yb * zc : 0.0) * R + mono * 2 * x * Rp;
          cgy[c] = (bb ? bb * xa * ipow(y, bb - 1) * zc : 0.0) * R + mono * 2 * y * Rp;
          cgz[c] = (cc ? cc * xa * yb * ipow(z, cc - 1) : 0.0) * R + mono * 2 * z * Rp;
        }
        if (deriv >= 2) {
          // monomial derivatives m_i, m_ij; f = m R(r^2)
          const double mx = a ? a * ipow(x, a - 1) * yb * zc : 0.0, my = bb ? bb * xa * ipow(y, bb - 1) * zc : 0.0,
                       mz = cc ? cc * xa * yb * ipow(z, cc - 1) : 0.0;
          const double mxx = a > 1 ? a * (a - 1) * ipow(x, a - 2) * yb * zc : 0.0,
                       myy = bb > 1 ? bb * (bb - 1) * xa * ipow(y, bb - 2) * zc : 0.0,
                       mzz = cc > 1 ? cc * (cc - 1) * xa * yb * ipow(z, cc - 2) : 0.0,
                       mxy = (a && bb) ? a * bb * ipow(x, a - 1) * ipow(y, bb - 1) * zc : 0.0,
                       mxz = (a && cc) ? a * cc * ipow(x, a - 1) * yb * ipow(z, cc - 1) : 0.0,
                       myz = (bb && cc) ? bb * cc * xa * ipow(y, bb - 1) * ipow(z, cc - 1) : 0.0;
          ch[c] = {mxx * R + 4 * x * mx * Rp + mono * (2 * Rp + 4 * x * x * Rpp),
                   mxy * R + 2 * (x * my + y * mx) * Rp + mono * 4 * x * y * Rpp,
                   mxz * R + 2 * (x * mz + z * mx) * Rp + mono * 4 * x * z * Rpp,
                   myy * R + 4 * y * my * Rp + mono * (2 * Rp + 4 * y * y * Rpp),
                   myz * R + 2 * (y * mz + z * my) * Rp + mono * 4 * y * z * Rpp,
                   mzz * R + 4 * z * mz * Rp + mono * (2 * Rp + 4 * z * z * Rpp)};
        }
        if (deriv >= 3) {
          // Leibniz over the three axes: f_ijk = sum over subsets S of m_S R_(rest).
          const int pe[3] = {a, bb, cc};
          auto mder = [&](const int* cnt) {
            double v = 1;
            for (int q = 0; q < 3; ++q) {
              if (cnt[q] > pe[q]) return 0.0;
              for (int t = 0; t < cnt[q]; ++t) v *= pe[q] - t;
              v *= ipow(xv[q], pe[q] - cnt[q]);
            }
            return v;
          };
          auto rder = [&](const int* ax, int k) {
            if (k == 0) return R;
            if (k == 1) return 2 * xv[ax[0]] * Rp;
            if (k == 2) return (ax[0] == ax[1] ? 2 * Rp : 0.0) + 4 * xv[ax[0]] * xv[ax[1]] * Rpp;
            return 4 * ((ax[0] == ax[1] ? xv[ax[2]] : 0.0) + (ax[0] == ax[2] ? xv[ax[1]] : 0.0) + (ax[1] == ax[2] ? xv[ax[0]] : 0.0)) * Rpp +
                   8 * xv[ax[0]] * xv[ax[1]] * xv[ax[2]] * Rppp;
          };
          for (int q = 0; q < 10; ++q) {
            double v = 0;
            for (int S = 0; S < 8; ++S) {
              int cnt[3] = {0, 0, 0}, rest[3], nr = 0;
              for (int t = 0; t < 3; ++t) {
                if (S & (1 << t)) ++cnt[trip[q][t]];
                else rest[nr++] = trip[q][t];
              }
              v += mder(cnt) * rder(rest, nr);
            }
            c3[c][q] = v;
          }
        }
      }
      if (!T) {
        for (int c = 0; c < nc; ++c) {
          out.phi(g, col0 + c) = cv[c];
          if (deriv >= 1) { out.dx(g, col0 + c) = cgx[c]; out.dy(g, col0 + c) = cgy[c]; out.dz(g, col0 + c) = cgz[c]; }
          if (deriv >= 2) {
            Mat* hm[6] = {&out.dxx, &out.dxy, &out.dxz, &out.dyy, &out.dyz, &out.dzz};
            for (int q = 0; q < 6; ++q) (*hm[q])(g, col0 + c) = ch[c][q];
          }
          if (deriv >= 3)
            for (int q = 0; q < 10; ++q) out.d3[q](g, col0 + c) = c3[c][q];
        }
      } else {
        const int ns = 2 * l + 1;
        for (int m = 0; m < ns; ++m) {
          double v = 0, vx = 0, vy = 0, vz = 0, vh[6] = {0, 0, 0, 0, 0, 0}, v3[10] = {0};
          for (int c = 0; c < nc; ++c) {
            double t = (*T)(c, m);
            if (deriv >= 3) for (int q = 0; q < 10; ++q) v3[q] += t * c3[c][q];
            v += t * cv[c];
            if (deriv >= 1) { vx += t * cgx[c]; vy += t * cgy[c]; vz += t * cgz[c]; }
            if (deriv >= 2) for (int q = 0; q < 6; ++q) vh[q] += t * ch[c][q];
          }
          if (deriv >= 2) {
            Mat* hm[6] = {&out.dxx, &out.dxy, &out.dxz, &out.dyy, &out.dyz, &out.dzz};
            for (int q = 0; q < 6; ++q) (*hm[q])(g, col0 + m) = vh[q];
          }
          if (deriv >= 3)
            for (int q = 0; q < 10; ++q) out.d3[q](g, col0 + m) = v3[q];
          out.phi(g, col0 + m) = v;
          if (deriv >= 1) { out.dx(g, col0 + m) = vx; out.dy(g, col0 + m) = vy; out.dz(g, col0 + m) = vz; }
        }
      }
    }
  }
}

}  // namespace qm
