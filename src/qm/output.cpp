#include "output.hpp"
#include <cmath>
#include <iomanip>
#include <map>
#include <ostream>

namespace qm {

std::vector<std::string> ao_labels(const Basis& b) {
  static const char* sph[][9] = {
      {"s"}, {"px", "py", "pz"},
      {"dxy", "dyz", "dz2", "dxz", "dx2y2"},
      {"fy3x2", "fxyz", "fyz2", "fz3", "fxz2", "fzx2y2", "fx3"},
      {"g-4", "g-3", "g-2", "g-1", "g0", "g+1", "g+2", "g+3", "g+4"}};
  std::vector<std::string> out(b.nbf());
  std::map<std::pair<int, int>, int> count;  // (atom, l) -> shells so far
  for (const Shell& s : b.shells()) {
    int n = ++count[{s.atom, s.l}];
    for (int k = 0; k < s.nao; ++k) {
      std::string t;
      if (b.spherical() && s.l <= 4) t = sph[s.l][k];
      else if (!b.spherical() && s.l <= 1) t = s.l == 0 ? "s" : std::string("p") + "xyz"[k];
      else if (!b.spherical()) {
        int idx = 0;
        for (int lx = s.l; lx >= 0 && t.empty(); --lx)
          for (int ly = s.l - lx; ly >= 0; --ly, ++idx)
            if (idx == k) {
              t = std::string(1, "spdfghik"[s.l]) + std::string(lx, 'x') + std::string(ly, 'y') + std::string(s.l - lx - ly, 'z');
              break;
            }
      } else t = std::string(1, "spdfghik"[s.l]) + std::to_string(k - s.l);
      out[s.ao0 + k] = std::to_string(s.atom) + element_symbol(b.molecule().atoms[s.atom].Z) + " " + std::to_string(n) + t;
    }
  }
  return out;
}

static const char* ref_name(Reference r, bool dft) {
  switch (r) {
    case Reference::Restricted: return dft ? "rks" : "rhf";
    case Reference::Unrestricted: return dft ? "uks" : "uhf";
    default: return dft ? "roks" : "rohf";
  }
}

void write_text_output(std::ostream& os, const Input& in, const Basis& b, const ScfResult& r, double seconds) {
  const bool dft = in.scf.method != "hf";
  const bool restricted = r.reference != Reference::Unrestricted;
  const Molecule& mol = b.molecule();
  os << std::setprecision(12) << std::fixed;
  os << "# ecce-qm output, format version 1\n";
  os << "format_version 1\n";
  os << "program ecce-qm 0.1\n";
  os << "title " << in.title << "\n";
  os << "method " << in.scf.method << "\n";
  os << "reference " << ref_name(r.reference, dft) << "\n";
  os << "basis " << in.basis << "\n";
  os << "spherical " << (b.spherical() ? "true" : "false") << "\n";
  if (dft) os << "grid " << in.scf.grid_radial << " " << in.scf.grid_angular << "\n";
  os << "charge " << mol.charge << "\n";
  os << "multiplicity " << mol.multiplicity << "\n";
  os << "nbf " << b.nbf() << "\n";
  os << "nalpha " << r.nalpha << "\nnbeta " << r.nbeta << "\n";
  os << "converged " << (r.converged ? "yes" : "no") << "\n";
  os << "iterations " << r.iterations << "\n";
  os << "energy_total " << r.energy << "\n";
  os << "energy_nuclear " << r.e_nuc << "\n";
  os << "energy_one_electron " << r.e_one << "\n";
  os << "energy_coulomb " << r.e_coulomb << "\n";
  os << "energy_exchange_hf " << r.e_exchange_hf << "\n";
  os << "energy_xc " << r.e_xc << "\n";
  if (dft) os << "grid_electrons " << r.xc_electrons << "\n";
  os << "s_squared " << r.s2 << "\n";
  os << "dipole_au " << r.dipole[0] << " " << r.dipole[1] << " " << r.dipole[2] << "\n";
  double dn = std::sqrt(r.dipole[0] * r.dipole[0] + r.dipole[1] * r.dipole[1] + r.dipole[2] * r.dipole[2]);
  os << "dipole_debye " << r.dipole[0] * kDebyePerAu << " " << r.dipole[1] * kDebyePerAu << " "
     << r.dipole[2] * kDebyePerAu << " " << dn * kDebyePerAu << "\n";
  os << "wall_seconds " << std::setprecision(2) << seconds << std::setprecision(12) << "\n";
  os << "begin atoms  # index symbol Z x y z (angstrom) mulliken_net_charge\n";
  for (size_t a = 0; a < mol.atoms.size(); ++a)
    os << a << " " << element_symbol(mol.atoms[a].Z) << " " << mol.atoms[a].Z << " "
       << mol.atoms[a].x[0] * kBohrToAngstrom << " " << mol.atoms[a].x[1] * kBohrToAngstrom << " "
       << mol.atoms[a].x[2] * kBohrToAngstrom << " " << r.mulliken[a] << "\n";
  os << "end atoms\n";
  auto labels = ao_labels(b);
  os << "begin ao_labels  # index label, order of the MO coefficients\n";
  for (int i = 0; i < b.nbf(); ++i) os << i + 1 << " " << labels[i] << "\n";
  os << "end ao_labels\n";
  auto spin_block = [&](const char* spin, const Vec& eps, const Vec& occa, const Vec& occb, const Mat& C, bool total_occ) {
    os << "begin orbitals " << spin << "  # index energy(hartree) occupation\n";
    for (int i = 0; i < b.nbf(); ++i)
      os << i + 1 << " " << eps(i) << " " << (total_occ ? occa(i) + occb(i) : occa(i)) << "\n";
    os << "end orbitals\n";
    os << "begin mo_coefficients " << spin << "  # one line per MO: index then nbf coefficients\n";
    os << std::setprecision(10);
    for (int i = 0; i < b.nbf(); ++i) {
      os << i + 1;
      for (int k = 0; k < b.nbf(); ++k) os << " " << C(k, i);
      os << "\n";
    }
    os << std::setprecision(12);
    os << "end mo_coefficients\n";
  };
  if (restricted) spin_block("restricted", r.eps_a, r.occ_a, r.occ_b, r.C_a, true);
  else {
    spin_block("alpha", r.eps_a, r.occ_a, r.occ_a, r.C_a, false);
    spin_block("beta", r.eps_b, r.occ_b, r.occ_b, r.C_b, false);
  }
  os << "end_of_output\n";
}

void write_molden(std::ostream& os, const Input& in, const Basis& b, const ScfResult& r) {
  if (!b.spherical()) throw Error("Molden export needs spherical d/f functions");
  const Molecule& mol = b.molecule();
  os << "[Molden Format]\n[Title]\n " << in.title << " (ecce-qm)\n[Atoms] Angs\n";
  os << std::setprecision(10) << std::fixed;
  for (size_t a = 0; a < mol.atoms.size(); ++a)
    os << element_symbol(mol.atoms[a].Z) << " " << a + 1 << " " << mol.atoms[a].Z << " "
       << mol.atoms[a].x[0] * kBohrToAngstrom << " " << mol.atoms[a].x[1] * kBohrToAngstrom << " "
       << mol.atoms[a].x[2] * kBohrToAngstrom << "\n";
  os << "[5D]\n[7F]\n[9G]\n[GTO]\n";
  os << std::scientific << std::setprecision(10);
  const char* L = "spdfghik";
  for (size_t a = 0; a < mol.atoms.size(); ++a) {
    os << a + 1 << " 0\n";
    for (const Shell& s : b.shells()) {
      if (s.atom != static_cast<int>(a)) continue;
      os << L[s.l] << " " << s.exps.size() << " 1.00\n";
      for (size_t p = 0; p < s.exps.size(); ++p) os << s.exps[p] << " " << s.raw_coefs[p] << "\n";
    }
    os << "\n";
  }
  // Molden orders spherical functions m = 0,+1,-1,+2,-2,...; p as x,y,z.
  std::vector<int> perm(b.nbf());  // molden position -> our AO index
  for (const Shell& s : b.shells()) {
    if (s.l <= 1) { for (int k = 0; k < s.nao; ++k) perm[s.ao0 + k] = s.ao0 + k; continue; }
    int pos = 0;
    for (int m = 0; m <= s.l; ++m) {
      perm[s.ao0 + pos++] = s.ao0 + s.l + m;
      if (m > 0) perm[s.ao0 + pos++] = s.ao0 + s.l - m;
    }
  }
  os << "[MO]\n";
  os << std::fixed << std::setprecision(8);
  auto spin = [&](const char* name, const Vec& eps, const Vec& occ, const Mat& C, bool two) {
    for (int i = 0; i < b.nbf(); ++i) {
      os << "Sym= " << i + 1 << "a\nEne= " << eps(i) << "\nSpin= " << name << "\nOccup= "
         << (two ? 2.0 * occ(i) : occ(i)) << "\n";
      for (int k = 0; k < b.nbf(); ++k) os << " " << k + 1 << " " << C(perm[k], i) << "\n";
    }
  };
  if (r.reference == Reference::Unrestricted) {
    spin("Alpha", r.eps_a, r.occ_a, r.C_a, false);
    spin("Beta", r.eps_b, r.occ_b, r.C_b, false);
  } else {
    // one set of orbitals: occupation 2, 1 or 0 (ROHF open shell = 1)
    Vec occ = r.occ_a + r.occ_b;
    for (int i = 0; i < b.nbf(); ++i) {
      os << "Sym= " << i + 1 << "a\nEne= " << r.eps_a(i) << "\nSpin= Alpha\nOccup= " << occ(i) << "\n";
      for (int k = 0; k < b.nbf(); ++k) os << " " << k + 1 << " " << r.C_a(perm[k], i) << "\n";
    }
  }
}

}  // namespace qm
