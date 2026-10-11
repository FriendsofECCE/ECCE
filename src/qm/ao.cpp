#include "ao.hpp"
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

void eval_ao(const Basis& b, const double* xyz, int n, int deriv, AoValues& out) {
  const int nb = b.nbf();
  out.phi = Mat::Zero(n, nb);
  if (deriv >= 1) {
    out.dx = Mat::Zero(n, nb);
    out.dy = Mat::Zero(n, nb);
    out.dz = Mat::Zero(n, nb);
  }
  if (deriv >= 2) {
    for (Mat* m : {&out.dxx, &out.dxy, &out.dxz, &out.dyy, &out.dyz, &out.dzz}) *m = Mat::Zero(n, nb);
  }
  const bool sph = b.spherical();
  for (const Shell& s : b.shells()) {
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
    for (int g = 0; g < n; ++g) {
      const double x = xyz[3 * g] - s.center[0], y = xyz[3 * g + 1] - s.center[1], z = xyz[3 * g + 2] - s.center[2];
      const double r2 = x * x + y * y + z * z;
      if (r2 > s.extent * s.extent) continue;
      double R = 0, Rp = 0, Rpp = 0;  // R, dR/d(r^2), d2R/d(r^2)^2
      for (int p = 0; p < np; ++p) {
        double e = s.coefs[p] * std::exp(-s.exps[p] * r2);
        R += e;
        Rp -= s.exps[p] * e;
        Rpp += s.exps[p] * s.exps[p] * e;
      }
      R *= fac;
      Rp *= fac;
      Rpp *= fac;
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
      }
      if (!T) {
        for (int c = 0; c < nc; ++c) {
          out.phi(g, s.ao0 + c) = cv[c];
          if (deriv >= 1) { out.dx(g, s.ao0 + c) = cgx[c]; out.dy(g, s.ao0 + c) = cgy[c]; out.dz(g, s.ao0 + c) = cgz[c]; }
          if (deriv >= 2) {
            Mat* hm[6] = {&out.dxx, &out.dxy, &out.dxz, &out.dyy, &out.dyz, &out.dzz};
            for (int q = 0; q < 6; ++q) (*hm[q])(g, s.ao0 + c) = ch[c][q];
          }
        }
      } else {
        const int ns = 2 * l + 1;
        for (int m = 0; m < ns; ++m) {
          double v = 0, vx = 0, vy = 0, vz = 0, vh[6] = {0, 0, 0, 0, 0, 0};
          for (int c = 0; c < nc; ++c) {
            double t = (*T)(c, m);
            v += t * cv[c];
            if (deriv >= 1) { vx += t * cgx[c]; vy += t * cgy[c]; vz += t * cgz[c]; }
            if (deriv >= 2) for (int q = 0; q < 6; ++q) vh[q] += t * ch[c][q];
          }
          if (deriv >= 2) {
            Mat* hm[6] = {&out.dxx, &out.dxy, &out.dxz, &out.dyy, &out.dyz, &out.dzz};
            for (int q = 0; q < 6; ++q) (*hm[q])(g, s.ao0 + m) = vh[q];
          }
          out.phi(g, s.ao0 + m) = v;
          if (deriv >= 1) { out.dx(g, s.ao0 + m) = vx; out.dy(g, s.ao0 + m) = vy; out.dz(g, s.ao0 + m) = vz; }
        }
      }
    }
  }
}

}  // namespace qm
