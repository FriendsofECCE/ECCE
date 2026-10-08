#include "basis.hpp"
#include <algorithm>
#include <cmath>
#include <cctype>

#include "cint_wrap.hpp"

namespace qm {

// integral of r^n exp(-a r^2) dr from 0 to infinity
static double gaussian_int(int n, double a) { return std::tgamma((n + 1) * 0.5) / (2.0 * std::pow(a, (n + 1) * 0.5)); }

bool Basis::default_spherical(const std::string&) {
  // Spherical d/f for every basis set, as ORCA does; Cartesian is opt-in
  // (input keyword "spherical false") because the energy differs slightly.
  return true;
}

Basis::Basis(const Molecule& mol, BasisLibrary& lib, const std::string& name, int spherical)
    : mol_(mol), name_(name), lib_(&lib) {
  sph_ = spherical < 0 ? default_spherical(name) : spherical != 0;
  const int natm_ = static_cast<int>(mol.atoms.size());
  env_.assign(PTR_ENV_START, 0.0);
  atm_.assign(natm_ * ATM_SLOTS, 0);
  for (int a = 0; a < natm_; ++a) {
    int off = static_cast<int>(env_.size());
    for (int k = 0; k < 3; ++k) env_.push_back(mol.atoms[a].x[k]);
    atm_[a * ATM_SLOTS + CHARGE_OF] = mol.atoms[a].Z;
    atm_[a * ATM_SLOTS + PTR_COORD] = off;
  }
  int ao = 0;
  for (int a = 0; a < natm_; ++a) {
    std::vector<ShellDef> defs = lib.shells(name, mol.atoms[a].Z);
    std::stable_sort(defs.begin(), defs.end(), [](const ShellDef& x, const ShellDef& y) { return x.l < y.l; });
    for (const auto& d : defs) {
      Shell s;
      s.atom = a;
      s.l = d.l;
      s.exps = d.exps;
      s.raw_coefs = d.coefs;
      const int np = static_cast<int>(d.exps.size());
      // Primitive normalisation, then renormalise the contraction (libcint
      // leaves both to the caller).
      std::vector<double> c(np);
      for (int p = 0; p < np; ++p)
        c[p] = d.coefs[p] / std::sqrt(gaussian_int(2 * d.l + 2, 2 * d.exps[p]));
      double ss = 0;
      for (int p = 0; p < np; ++p)
        for (int q = 0; q < np; ++q) ss += c[p] * c[q] * gaussian_int(2 * d.l + 2, d.exps[p] + d.exps[q]);
      for (auto& x : c) x /= std::sqrt(ss);
      s.coefs = c;
      for (int k = 0; k < 3; ++k) s.center[k] = mol.atoms[a].x[k];
      s.nao = nao_of(s.l, sph_);
      s.ao0 = ao;
      ao += s.nao;
      // extent: where the most diffuse primitive's value drops below 1e-10
      double ext = 0;
      for (int p = 0; p < np; ++p) {
        double a0 = d.exps[p], amp = std::fabs(c[p]);
        double r = 1.0;
        for (int it = 0; it < 200; ++it) {
          double v = amp * std::pow(r, s.l) * std::exp(-a0 * r * r);
          if (v < 1e-10 && r > 1.0 / std::sqrt(a0)) break;
          r += 0.1;
        }
        ext = std::max(ext, r);
      }
      s.extent = ext;
      // libcint basis entry
      int pe = static_cast<int>(env_.size());
      for (double e : s.exps) env_.push_back(e);
      int pc = static_cast<int>(env_.size());
      for (double x : s.coefs) env_.push_back(x);
      int b[BAS_SLOTS] = {0};
      b[ATOM_OF] = a;
      b[ANG_OF] = s.l;
      b[NPRIM_OF] = np;
      b[NCTR_OF] = 1;
      b[PTR_EXP] = pe;
      b[PTR_COEFF] = pc;
      bas_.insert(bas_.end(), b, b + BAS_SLOTS);
      shells_.push_back(std::move(s));
    }
  }
  nbf_ = ao;
}

std::string Basis::ao_label(int ao) const {
  static const char* sphnames[][9] = {
      {"s"}, {"px", "py", "pz"},
      {"dxy", "dyz", "dz2", "dxz", "dx2y2"},
      {"fy3x2", "fxyz", "fyz2", "fz3", "fxz2", "fzx2y2", "fx3"},
  };
  for (const auto& s : shells_) {
    if (ao < s.ao0 + s.nao) {
      int k = ao - s.ao0;
      std::string t;
      if (sph_ && s.l <= 3) t = sphnames[s.l][k];
      else if (!sph_ && s.l <= 1) t = s.l == 0 ? "s" : std::string("p") + "xyz"[k];
      else {
        // Cartesian: enumerate lx >= ly >= ... in libcint order
        int idx = 0;
        for (int lx = s.l; lx >= 0 && t.empty(); --lx)
          for (int ly = s.l - lx; ly >= 0; --ly, ++idx)
            if (idx == k) {
              int lz = s.l - lx - ly;
              t = std::string(1, "spdfghik"[s.l]);
              t += std::string(lx, 'x') + std::string(ly, 'y') + std::string(lz, 'z');
              break;
            }
        if (sph_) t = std::string(1, "spdfghik"[s.l]) + "_m" + std::to_string(k - s.l);
      }
      return std::to_string(s.atom) + element_symbol(mol_.atoms[s.atom].Z) + " " + std::to_string(s.l) + t;
    }
  }
  return "?";
}

}  // namespace qm
