#include "output.hpp"
#include "localise.hpp"
#include "polar.hpp"
#include <cctype>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <map>
#include <ostream>

namespace qm {

std::vector<std::string> ao_labels(const Basis& b, const Molecule* full) {
  const std::vector<int> idx = full ? full->real_atoms() : std::vector<int>();
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
      out[s.ao0 + k] = std::to_string(full ? idx[s.atom] : s.atom) + element_symbol(b.molecule().atoms[s.atom].Z) + " " + std::to_string(n) + t;
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

void write_opt_step(std::ostream& os, const OptStep& st) {
  os << std::setprecision(12) << std::fixed;
  os << "opt_step " << st.index << "\n";
  os << "energy_step " << st.energy << "\n";
  os << std::scientific << std::setprecision(4);
  os << "opt_criteria max_gradient " << st.max_grad << " rms_gradient " << st.rms_grad << " max_step " << st.max_step
     << " rms_step " << st.rms_step << " energy_change " << st.d_energy << "\n";
  os << std::fixed << std::setprecision(10);
  os << "begin geometry  # index symbol x y z (angstrom)\n";
  for (size_t a = 0; a < st.mol.atoms.size(); ++a)
    os << a + 1 << " " << element_symbol(st.mol.atoms[a].Z) << " " << st.mol.atoms[a].x[0] * kBohrToAngstrom << " "
       << st.mol.atoms[a].x[1] * kBohrToAngstrom << " " << st.mol.atoms[a].x[2] * kBohrToAngstrom << "\n";
  os << "end geometry\n";
  os << "begin gradient  # index symbol dE/dx dE/dy dE/dz (hartree/bohr)\n";
  for (size_t a = 0; a < st.mol.atoms.size(); ++a)
    os << a + 1 << " " << element_symbol(st.mol.atoms[a].Z) << " " << st.grad[3 * a] << " " << st.grad[3 * a + 1] << " "
       << st.grad[3 * a + 2] << "\n";
  os << "end gradient\n";
  os.flush();
}

void write_scan_table(std::ostream& os, const std::vector<ScanPointOut>& pts, bool bond) {
  char b[160];
  os << "begin scan_table  # point value energy(hartree) converged; unit " << (bond ? "angstrom" : "degree") << "\n";
  for (size_t i = 0; i < pts.size(); ++i) {
    std::snprintf(b, sizeof b, "%zu %.6f %.12f %s\n", i + 1, pts[i].value, pts[i].energy, pts[i].converged ? "yes" : "no");
    os << b;
  }
  os << "end scan_table\n";
  os.flush();
}

void write_scan_geometry(std::ostream& os, int point, double value, const Molecule& mol) {
  char b[160];
  std::snprintf(b, sizeof b, "begin scan_geometry  # point %d value %.6f; index symbol x y z (angstrom)\n", point, value);
  os << b;
  for (size_t a = 0; a < mol.atoms.size(); ++a) {
    std::snprintf(b, sizeof b, "%zu %s %.10f %.10f %.10f\n", a + 1, element_symbol(mol.atoms[a].Z),
                  mol.atoms[a].x[0] * kBohrToAngstrom, mol.atoms[a].x[1] * kBohrToAngstrom, mol.atoms[a].x[2] * kBohrToAngstrom);
    os << b;
  }
  os << "end scan_geometry\n";
  os.flush();
}

void write_progress_scan(std::ostream& os, int point, int points) {
  os << "progress scan point=" << point << "/" << points << "\n";
  os.flush();
}

// Fixed formats, so the monitor's lines do not depend on the stream's state.
static std::string sci(double v, int digits = 2) {
  char b[32];
  std::snprintf(b, sizeof b, "%.*e", digits, v);
  return b;
}

static std::string fix(double v) {
  char b[32];
  std::snprintf(b, sizeof b, "%.10f", v);
  return b;
}

void write_progress_scf(std::ostream& os, const ScfCycle& c, const ScfOptions& opt) {
  os << "progress scf cycle=" << c.cycle << " energy=" << fix(c.energy);
  if (!std::isnan(c.dE)) os << " dE=" << sci(c.dE) << "/" << sci(opt.conv_energy, 0);
  os << " grad=" << sci(c.grad) << "/" << sci(opt.conv_grad, 0);
  if (c.converged) os << " converged=yes";
  os << "\n";
  os.flush();
}

void write_progress_scf_failed(std::ostream& os, int cycles) {
  os << "progress scf cycle=" << cycles << " converged=no\n";
  os.flush();
}

void write_progress_opt(std::ostream& os, const OptStep& st, const OptOptions& opt) {
  os << "progress opt step=" << st.index << " energy=" << fix(st.energy) << " maxgrad=" << sci(st.max_grad) << "/"
     << sci(opt.max_grad, 0) << " rmsgrad=" << sci(st.rms_grad) << "/" << sci(opt.rms_grad, 0)
     << " maxstep=" << sci(st.max_step) << "/" << sci(opt.max_step, 0) << " rmsstep=" << sci(st.rms_step) << "/"
     << sci(opt.rms_step, 0);
  if (st.index > 1) os << " dE=" << sci(st.d_energy) << "/" << sci(opt.d_energy, 0);
  os << "\n";
  os.flush();
}

void write_progress_opt_end(std::ostream& os, int step, bool converged) {
  os << "progress opt step=" << step << " converged=" << (converged ? "yes" : "no") << "\n";
  os.flush();
}

void write_progress_freq(std::ostream& os, int point, int points) {
  os << "progress freq point=" << point << "/" << points << "\n";
  os.flush();
}

void write_progress_freq_end(std::ostream& os, int points, const FrequencyOutput& f) {
  const int imag = f.vib.n_imaginary;
  os << "progress freq point=" << points << "/" << points << " done=yes real="
     << static_cast<int>(f.vib.freq.size()) - imag << " imaginary=" << imag << " zpe=" << fix(f.thermo.zpe) << "\n";
  os.flush();
}

// Every mode of the 3N is listed, the translations and rotations first with
// frequency and intensity 0 (as ORCA does), so the lists line up row by row.
static void write_frequency_blocks(std::ostream& os, const FrequencyOutput& f, const Molecule& mol, double e_el) {
  const VibResult& v = f.vib;
  const int n = 3 * static_cast<int>(mol.atoms.size()), ntr = v.ntransrot;
  os << std::fixed << std::setprecision(12);
  os << "hessian_step " << f.step << "\n";
  os << "max_gradient " << std::scientific << std::setprecision(4) << f.max_gradient << std::fixed << std::setprecision(12) << "\n";
  os << "vib_transrot " << ntr << "\n";
  os << "vib_imaginary " << v.n_imaginary << "\n";
  os << "begin transrot_residual  # smallest frequencies (cm-1) before projection\n";
  os << std::setprecision(4);
  for (double x : v.residual) os << x << "\n";
  os << "end transrot_residual\n";
  os << "begin vibrations  # index frequency(cm-1) ir_intensity(km/mol); imaginary frequencies are negative\n";
  for (int i = 0; i < n; ++i) {
    const bool real = i >= ntr;
    os << i + 1 << " " << (real ? v.freq[i - ntr] : 0.0) << " " << (real ? v.ir[i - ntr] : 0.0) << "\n";
  }
  os << "end vibrations\n";
  os << "begin normal_modes  # index then x y z of every atom, Cartesian displacements of unit length\n";
  os << std::setprecision(8);
  for (int i = 0; i < n; ++i) {
    os << i + 1;
    for (int k = 0; k < n; ++k) os << " " << (i >= ntr ? v.modes(k, i - ntr) : 0.0);
    os << "\n";
  }
  os << "end normal_modes\n";
  if (!v.raman.empty()) {
    os << "begin raman  # index activity(A^4/amu) depolarization_ratio, rows as in vibrations\n";
    os << std::setprecision(6);
    for (int i = 0; i < n; ++i) {
      const bool real = i >= ntr;
      os << i + 1 << " " << (real ? v.raman[i - ntr] : 0.0) << " " << (real ? v.depol[i - ntr] : 0.0) << "\n";
    }
    os << "end raman\n";
    os << std::setprecision(12);
  }
  const Thermo& t = f.thermo;
  os << std::setprecision(12);
  os << "thermo_temperature " << t.temperature << "\nthermo_pressure_atm " << t.pressure << "\n";
  os << "symmetry_number " << t.symmetry_number << "\n";
  os << "zero_point_energy " << t.zpe << "\n";
  os << "thermal_energy " << e_el + t.e_thermal << "\n";
  os << "enthalpy " << e_el + t.enthalpy_corr << "\n";
  os << "entropy_ts " << t.ts << "\n";
  os << "gibbs_energy " << e_el + t.gibbs_corr << "\n";
  os << std::setprecision(10);
  os << "entropy_translation " << t.s_trans << "\nentropy_rotation " << t.s_rot << "\nentropy_vibration " << t.s_vib
     << "\nentropy_electronic " << t.s_elec << "\n";
}

void write_text_output(std::ostream& os, const Input& in, const Basis& b, const ScfResult& r, double seconds,
                       const OptResult* opt, const FrequencyOutput* freq, const ChargeSets* charges,
                       const Molecule* full_mol, const PolarOutput* polar) {
  const bool dft = in.scf.method != "hf";
  const bool restricted = r.reference != Reference::Unrestricted;
  const Molecule& mol = b.molecule();
  const Molecule& allat = full_mol ? *full_mol : mol;   // with dummy atoms
  const std::vector<int> real = allat.real_atoms();
  // per-real-atom values to one per atom of allat (dummy atoms: 0)
  auto spread = [&](const std::vector<double>& q) {
    std::vector<double> o(allat.atoms.size(), 0.0);
    for (size_t i = 0; i < real.size() && i < q.size(); ++i) o[real[i]] = q[i];
    return o;
  };
  os << std::setprecision(12) << std::fixed;
  os << "# ecce-qm output, format version 1\n";
  os << "format_version 1\n";
  os << "program ecce-qm 0.1\n";
  os << "title " << in.title << "\n";
  static const char* kRunNames[] = {"energy", "gradient", "optimize", "frequency", "optfreq", "ts"};
  os << "runtype " << kRunNames[static_cast<int>(in.runtype)] << "\n";
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
  if (opt) {
    os << "opt_converged " << (opt->converged ? "yes" : "no") << "\n";
    os << "opt_evaluations " << opt->evaluations << "\n";
  }
  os << "energy_total " << r.energy << "\n";
  os << "energy_nuclear " << r.e_nuc << "\n";
  os << "energy_one_electron " << r.e_one << "\n";
  os << "energy_coulomb " << r.e_coulomb << "\n";
  os << "energy_exchange_hf " << r.e_exchange_hf << "\n";
  os << "energy_xc " << r.e_xc << "\n";
  if (dft) os << "grid_electrons " << r.xc_electrons << "\n";
  // A closed-shell restricted run has <S^2> = 0 by construction, so it is
  // not listed (#390); open-shell runs (ROHF/ROKS and UHF/UKS) keep it.
  if (r.reference != Reference::Restricted)
    os << "s_squared " << r.s2 << "\n";
  os << "dipole_au " << r.dipole[0] << " " << r.dipole[1] << " " << r.dipole[2] << "\n";
  double dn = std::sqrt(r.dipole[0] * r.dipole[0] + r.dipole[1] * r.dipole[1] + r.dipole[2] * r.dipole[2]);
  os << "dipole_debye " << r.dipole[0] * kDebyePerAu << " " << r.dipole[1] * kDebyePerAu << " "
     << r.dipole[2] * kDebyePerAu << " " << dn * kDebyePerAu << "\n";
  os << "wall_seconds " << std::setprecision(2) << seconds << std::setprecision(12) << "\n";
  os << "begin atoms  # index symbol Z x y z (angstrom) mulliken_net_charge\n";
  const std::vector<double> mull = spread(r.mulliken);
  for (size_t a = 0; a < allat.atoms.size(); ++a)
    os << a << " " << element_symbol(allat.atoms[a].Z) << " " << allat.atoms[a].Z << " "
       << allat.atoms[a].x[0] * kBohrToAngstrom << " " << allat.atoms[a].x[1] * kBohrToAngstrom << " "
       << allat.atoms[a].x[2] * kBohrToAngstrom << " " << mull[a] << "\n";
  os << "end atoms\n";
  if (charges) {
    auto block = [&](const char* name, const std::vector<double>& qreal, int npts) {
      if (qreal.empty()) return;
      const std::vector<double> q = spread(qreal);
      os << "begin charges " << name << "  # index symbol net_charge";
      if (npts) os << "  (" << npts << " fit points)";
      os << "\n";
      for (size_t a = 0; a < q.size(); ++a) os << a << " " << element_symbol(allat.atoms[a].Z) << " " << q[a] << "\n";
      os << "end charges\n";
    };
    block("loewdin", charges->loewdin, 0);
    block("chelpg", charges->chelpg, charges->chelpg_points);
    block("mk", charges->mk, charges->mk_points);
  }
  auto labels = ao_labels(b, full_mol);
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
  if (in.localise && r.reference == Reference::Restricted && r.converged && r.nalpha > 0) {
    // Same density as the canonical set, so the pictures are of one wavefunction.
    const Localised loc = localise_occupied(mol, b, r, &real);
    os << "begin localised_orbitals restricted  # index energy(hartree) occupation label\n";
    for (size_t i = 0; i < loc.orb.size(); ++i) os << i + 1 << " " << loc.orb[i].energy << " 2 " << loc.orb[i].label << "\n";
    os << "end localised_orbitals\n";
    os << "begin localised_mo_coefficients restricted  # one line per localised MO: index then nbf coefficients\n";
    os << std::setprecision(10);
    for (size_t i = 0; i < loc.orb.size(); ++i) {
      os << i + 1;
      for (int k = 0; k < b.nbf(); ++k) os << " " << loc.C(k, i);
      os << "\n";
    }
    os << std::setprecision(12);
    os << "end localised_mo_coefficients\n";
  }
  if (polar && polar->alpha.rows() == 3) {
    const PolarInvariants pi = polarizability_invariants(polar->alpha);
    os << "polarizability_field_step " << polar->step << "\n";
    os << "polarizability_isotropic " << pi.iso << "\n";
    os << "polarizability_anisotropy " << pi.aniso << "\n";
    os << "begin polarizability  # rows xx xy xz / yx yy yz / zx zy zz (a.u., bohr^3)\n";
    for (int i = 0; i < 3; ++i)
      os << polar->alpha(i, 0) << " " << polar->alpha(i, 1) << " " << polar->alpha(i, 2) << "\n";
    os << "end polarizability\n";
  }
  if (freq) write_frequency_blocks(os, *freq, mol, r.energy);
  os << "end_of_output\n";
}

void write_molden(std::ostream& os, const Input& in, const Basis& b, const ScfResult& r, const FrequencyOutput* freq) {
  if (!b.spherical()) throw Error("Molden export needs spherical d/f functions");
  const Molecule& mol = b.molecule();
  os << "[Molden Format]\n[Title]\n " << in.title << " (ecce-qm)\n[Atoms] Angs\n";
  os << std::setprecision(10) << std::fixed;
  for (size_t a = 0; a < mol.atoms.size(); ++a)
    os << element_symbol(mol.atoms[a].Z) << " " << a + 1 << " " << mol.atoms[a].Z << " "
       << mol.atoms[a].x[0] * kBohrToAngstrom << " " << mol.atoms[a].x[1] * kBohrToAngstrom << " "
       << mol.atoms[a].x[2] * kBohrToAngstrom << "\n";
  if (freq && !freq->vib.freq.empty()) {
    const VibResult& v = freq->vib;
    const int nat = static_cast<int>(mol.atoms.size());
    os << std::fixed << std::setprecision(4) << "[FREQ]\n";
    for (double x : v.freq) os << x << "\n";
    os << std::setprecision(10) << "[FR-COORD]\n";
    for (int a = 0; a < nat; ++a)
      os << element_symbol(mol.atoms[a].Z) << " " << mol.atoms[a].x[0] << " " << mol.atoms[a].x[1] << " " << mol.atoms[a].x[2] << "\n";
    os << "[FR-NORM-COORD]\n";
    for (size_t m = 0; m < v.freq.size(); ++m) {
      os << "vibration " << m + 1 << "\n";
      for (int a = 0; a < nat; ++a)
        os << v.modes(3 * a, m) << " " << v.modes(3 * a + 1, m) << " " << v.modes(3 * a + 2, m) << "\n";
    }
    os << std::setprecision(4) << "[INT]\n";
    for (double x : v.ir) os << x << "\n";
  }
  os << std::fixed << std::setprecision(10);
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

// Format: "ecce-qm-orbitals 1", basis name, natoms and their Z, nbf, nalpha,
// nbeta, restricted flag, then C_a (and C_b unless restricted) row by row.
void write_orbitals(std::ostream& os, const Input& in, const Molecule& mol, const ScfResult& r) {
  const bool same_spin = r.reference != Reference::Unrestricted;
  os << "ecce-qm-orbitals 1\nbasis " << in.basis << "\natoms " << mol.atoms.size();
  for (const Atom& a : mol.atoms) os << " " << a.Z;
  os << "\nnbf " << r.C_a.rows() << "\nelectrons " << r.nalpha << " " << r.nbeta
     << "\nrestricted " << (same_spin ? 1 : 0) << "\n" << std::setprecision(17);
  auto put = [&](const Mat& C) {
    for (int i = 0; i < C.rows(); ++i) {
      for (int j = 0; j < C.cols(); ++j) os << (j ? " " : "") << C(i, j);
      os << "\n";
    }
  };
  put(r.C_a);
  if (!same_spin) put(r.C_b);
}

bool read_orbitals(std::istream& is, const Input& in, const Molecule& mol, int nbf, ScfResult& r, std::string& why) {
  std::string tok, ver, basis;
  size_t nat = 0;
  int n = 0, na = 0, nb = 0, restricted = 1;
  if (!(is >> tok >> ver) || tok != "ecce-qm-orbitals") { why = "not an ecce-qm orbital file"; return false; }
  is >> tok >> basis >> tok >> nat;
  std::vector<int> z(nat);
  for (int& v : z) is >> v;
  is >> tok >> n >> tok >> na >> nb >> tok >> restricted;
  if (!is) { why = "the orbital file is damaged"; return false; }
  auto lower = [](std::string s) { for (char& c : s) c = static_cast<char>(std::tolower(c)); return s; };
  if (lower(basis) != lower(in.basis) || n != nbf) { why = "it was made with the basis set " + basis; return false; }
  bool atoms = nat == mol.atoms.size();
  for (size_t i = 0; atoms && i < nat; ++i) atoms = z[i] == mol.atoms[i].Z;
  if (!atoms) { why = "its atoms differ"; return false; }
  auto get = [&](Mat& C) {
    C.resize(n, n);
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < n; ++j) is >> C(i, j);
  };
  get(r.C_a);
  if (restricted) r.C_b = r.C_a; else get(r.C_b);
  if (!is) { why = "the orbital file is damaged"; return false; }
  r.nalpha = na;
  r.nbeta = nb;
  r.converged = true;
  return true;
}

}  // namespace qm
