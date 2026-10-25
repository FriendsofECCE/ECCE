// qm_unit: checks of the pieces that have an independent answer.
//  * Lebedev rules integrate monomials exactly (analytic values);
//  * AO values on the grid, integrated pairwise, reproduce libcint's overlap
//    matrix (two independent routes to <i|j>: quadrature and libcint);
//  * AO gradients and second derivatives agree with finite differences;
//  * model Hessians give the closed-form frequency and IR intensity, rotational
//    symmetry numbers of standard molecules, and the Sackur-Tetrode entropy.
#include <cmath>
#include <iostream>
#include "ao.hpp"
#include "basis.hpp"
#include "grid.hpp"
#include "gradient.hpp"
#include "hessian.hpp"
#include "scf.hpp"
#include "integrals.hpp"
#include "irc.hpp"
#include "localise.hpp"
#include "solvent.hpp"
#include "vibration.hpp"
#include <array>

using namespace qm;

static int fails = 0;
static void check(bool ok, const std::string& what, double val = 0) {
  std::cout << (ok ? "ok    " : "FAIL  ") << what << "  " << val << "\n";
  if (!ok) ++fails;
}

int main(int argc, char** argv) {
  std::string bdir = argc > 1 ? argv[1] : "";
  BasisLibrary lib(bdir);

  // Lebedev: integral of x^2 y^2 z^2 over the unit sphere / 4pi = 1/105 (needs degree 6).
  {
    Molecule m;
    Atom a; a.Z = 1;
    m.atoms.push_back(a);
    for (int n : lebedev_sizes()) {
      if (n < 26) continue;  // degree >= 7
      Grid g = make_grid(m, 1, n, false);  // one radial shell: weights = 4 pi r^2 w_r * w_ang
      // recover the angular weights: normalise by their sum
      double s = 0, v = 0;
      for (size_t i = 0; i < g.size(); ++i) {
        double r = std::sqrt(g.x[i] * g.x[i] + g.y[i] * g.y[i] + g.z[i] * g.z[i]);
        double x = g.x[i] / r, y = g.y[i] / r, z = g.z[i] / r;
        s += g.w[i];
        v += g.w[i] * x * x * y * y * z * z;
      }
      check(std::fabs(v / s - 1.0 / 105.0) < 1e-14, "lebedev " + std::to_string(n) + " x2y2z2", v / s - 1.0 / 105.0);
    }
  }

  // AO overlap by quadrature vs libcint, and normalisation, on water.
  for (const char* bname : {"def2-SVP", "6-31G*", "STO-3G"}) {
    for (int sph : {1, 0}) {
      Molecule m;
      m.atoms = {{8, {0, 0, 0.2216}}, {1, {0, 1.4309, -0.8867}}, {1, {0, -1.4309, -0.8867}}};
      Basis b(m, lib, bname, sph);
      OneElectron oe = one_electron(b);
      Grid g = make_grid(m, 120, 590);
      std::vector<double> xyz(3 * g.size());
      for (size_t i = 0; i < g.size(); ++i) { xyz[3 * i] = g.x[i]; xyz[3 * i + 1] = g.y[i]; xyz[3 * i + 2] = g.z[i]; }
      AoValues ao;
      eval_ao(b, xyz.data(), static_cast<int>(g.size()), 1, ao);
      Mat W = Eigen::Map<Eigen::VectorXd>(g.w.data(), g.size());
      Mat S = ao.phi.transpose() * (W.asDiagonal() * ao.phi);
      double err = (S - oe.S).cwiseAbs().maxCoeff();
      check(err < 2e-6, std::string("overlap quadrature vs libcint ") + bname + (sph ? " sph" : " cart"), err);
      if (sph) {
        double dn = 0;
        for (int i = 0; i < b.nbf(); ++i) dn = std::max(dn, std::fabs(oe.S(i, i) - 1.0));
        check(dn < 1e-12, std::string("AO norm ") + bname, dn);
      }
      // gradient by central differences at a few points
      double gerr = 0;
      for (int p = 0; p < 20; ++p) {
        size_t i = (p * 7919) % g.size();
        double h = 1e-6, pt[3] = {g.x[i], g.y[i], g.z[i]};
        AoValues a0, ap, am;
        eval_ao(b, pt, 1, 1, a0);
        for (int c = 0; c < 3; ++c) {
          double q[3] = {pt[0], pt[1], pt[2]};
          q[c] += h; eval_ao(b, q, 1, 0, ap);
          q[c] -= 2 * h; eval_ao(b, q, 1, 0, am);
          Mat& an = c == 0 ? a0.dx : (c == 1 ? a0.dy : a0.dz);
          gerr = std::max(gerr, ((ap.phi - am.phi) / (2 * h) - an).cwiseAbs().maxCoeff());
        }
      }
      check(gerr < 1e-6, std::string("AO gradient vs finite difference ") + bname, gerr);
      // second derivatives (DFT gradients) by differences of the gradients, relative to the largest
      double herr = 0;
      for (int p = 0; p < 20; ++p) {
        size_t i = (p * 7919) % g.size();
        double h = 1e-6, pt[3] = {g.x[i], g.y[i], g.z[i]};
        AoValues a0, ap, am;
        eval_ao(b, pt, 1, 2, a0);
        const double scale = std::max(1.0, a0.dxx.cwiseAbs().maxCoeff());
        for (int c = 0; c < 3; ++c) {
          double q[3] = {pt[0], pt[1], pt[2]};
          q[c] += h; eval_ao(b, q, 1, 1, ap);
          q[c] -= 2 * h; eval_ao(b, q, 1, 1, am);
          const Mat* row[3] = {c == 0 ? &a0.dxx : c == 1 ? &a0.dxy : &a0.dxz,
                               c == 0 ? &a0.dxy : c == 1 ? &a0.dyy : &a0.dyz,
                               c == 0 ? &a0.dxz : c == 1 ? &a0.dyz : &a0.dzz};
          const Mat* gp[3] = {&ap.dx, &ap.dy, &ap.dz};
          const Mat* gm[3] = {&am.dx, &am.dy, &am.dz};
          for (int d = 0; d < 3; ++d)
            herr = std::max(herr, ((*gp[d] - *gm[d]) / (2 * h) - *row[d]).cwiseAbs().maxCoeff() / scale);
        }
      }
      check(herr < 1e-6, std::string("AO second derivatives vs finite difference ") + bname, herr);
    }
  }
  // Vibrations.  A diatomic with a given Hessian and dipole derivative has
  // closed-form answers: nu = 5140.487 sqrt(k/mu) cm-1 for k in hartree/bohr^2
  // and mu in amu, and I = 974.88 (dmu/dQ)^2 km/mol for dmu/dQ in e/sqrt(amu).
  {
    Molecule m;
    for (double z : {0.0, 1.4}) { Atom a; a.Z = 1; a.x[2] = z; m.atoms.push_back(a); }
    const double k = 0.37, q = 0.1, mH = isotope_mass(1);
    Derivatives d;
    d.hessian = Mat::Zero(6, 6);
    d.hessian(2, 2) = d.hessian(5, 5) = k;
    d.hessian(2, 5) = d.hessian(5, 2) = -k;
    d.dmu = Mat::Zero(3, 6);
    d.dmu(2, 2) = q;
    d.dmu(2, 5) = -q;
    VibResult v = analyse_vibrations(m, d);
    const double nu = 5140.487 * std::sqrt(k / (mH / 2));
    check(v.ntransrot == 5 && v.freq.size() == 1, "H2 model: 5 translations/rotations, one vibration", v.freq.size());
    check(std::fabs(v.freq[0] / nu - 1) < 1e-4, "H2 model: frequency", v.freq[0] - nu);
    const double ir = 974.88 * 2 * q * q / mH;
    check(std::fabs(v.ir[0] / ir - 1) < 1e-3, "H2 model: IR intensity", v.ir[0] - ir);
    check(rotational_symmetry_number(m) == 2, "H2 symmetry number 2");
    // translations and rotations of the model Hessian come out at zero
    check(std::fabs(v.residual.front()) < 1e-3 && std::fabs(v.residual.back()) < 1e-3, "H2 model: TR residual", v.residual.back());
  }
  {
    auto mol = [](std::initializer_list<std::array<double, 4>> list) {
      Molecule m;
      for (auto& r : list) { Atom a; a.Z = static_cast<int>(r[0]); for (int k = 0; k < 3; ++k) a.x[k] = r[k + 1]; m.atoms.push_back(a); }
      return m;
    };
    check(rotational_symmetry_number(mol({{8, 0, 0, 0.22}, {1, 0, 1.43, -0.89}, {1, 0, -1.43, -0.89}})) == 2, "H2O symmetry number 2");
    const double c = 1.1 / std::sqrt(3.0) * 1.8897;
    check(rotational_symmetry_number(mol({{6, 0, 0, 0}, {1, c, c, c}, {1, c, -c, -c}, {1, -c, c, -c}, {1, -c, -c, c}})) == 12, "CH4 symmetry number 12");
    const double r = 1.9, h = 0.8;
    check(rotational_symmetry_number(mol({{7, 0, 0, 0.3}, {1, r, 0, -h}, {1, -r / 2, r * 0.8660254, -h}, {1, -r / 2, -r * 0.8660254, -h}})) == 3, "NH3 symmetry number 3");
    check(rotational_symmetry_number(mol({{6, 0, 0, 0}, {8, 0, 0, 2.13}})) == 1, "CO symmetry number 1");
    check(rotational_symmetry_number(mol({{7, 0, 0, 0}, {7, 0, 0, 2.07}})) == 2, "N2 symmetry number 2");
    std::vector<std::array<double, 4>> bz;
    for (int k = 0; k < 6; ++k) {
      const double a = k * kPi / 3;
      bz.push_back({6, 2.64 * std::cos(a), 2.64 * std::sin(a), 0});
      bz.push_back({1, 4.69 * std::cos(a), 4.69 * std::sin(a), 0});
    }
    Molecule b;
    for (auto& r2 : bz) { Atom a; a.Z = static_cast<int>(r2[0]); for (int k = 0; k < 3; ++k) a.x[k] = r2[k + 1]; b.atoms.push_back(a); }
    check(rotational_symmetry_number(b) == 12, "benzene symmetry number 12");
    // translational entropy of Ar at 298.15 K, 1 bar (Sackur-Tetrode, 1 bar): 154.846 J/(mol K)
    Molecule ar = mol({{18, 0, 0, 0}});
    VibResult none = analyse_vibrations(ar, Derivatives{Mat::Zero(3, 3), Mat::Zero(3, 3)});
    Thermo t = thermochemistry(ar, none, 298.15, 1e5 / 101325.0, 0);
    const double sJ = t.s_trans * 4.3597447222071e-18 * 6.02214076e23;
    check(none.ntransrot == 3 && std::fabs(sJ - 154.846) < 0.02, "Ar translational entropy", sJ);
  }

  // Displaced points run side by side must give the Hessian and dipole
  // derivatives of the serial loop (water, B3LYP/6-31G*, coarse grid).
  {
    Molecule w;
    w.atoms = {{8, {0, 0, 0.2216}}, {1, {0, 1.4309, -0.8867}}, {1, {0, -1.4309, -0.8867}}};
    ScfOptions so;
    so.method = "b3lyp";
    so.grid_radial = 50;
    so.grid_angular = 194;
    so.conv_grad = 1e-8;
    so.conv_energy = 1e-11;
    Basis b0(w, lib, "6-31G*", 1);
    const ScfResult ref = run_scf(b0, w, so);
    auto point = [&](const Molecule& m) {
      Basis b(m, lib, "6-31G*", 1);
      ScfResult r = run_scf(b, m, so, &ref);
      PointEval pe;
      pe.grad = scf_gradient(b, m, so, r);
      for (int k = 0; k < 3; ++k) pe.dipole[k] = r.dipole[k];
      return pe;
    };
    int calls = 0;
    Derivatives ds = finite_difference_derivatives(w, 0.005, point);
    Derivatives dp = finite_difference_derivatives(w, 0.005, point, [&](int, int) { ++calls; }, 3, 1);
    check(calls == 18, "parallel points: on_point called once per point", calls);
    check((ds.hessian - dp.hessian).cwiseAbs().maxCoeff() < 1e-7, "parallel Hessian equals serial",
          (ds.hessian - dp.hessian).cwiseAbs().maxCoeff());
    check((ds.dmu - dp.dmu).cwiseAbs().maxCoeff() < 1e-7, "parallel dipole derivatives equal serial",
          (ds.dmu - dp.dmu).cwiseAbs().maxCoeff());
  }

  // Analytic RHF Hessian, dipole derivatives and harmonic frequencies against
  // finite differences of the analytic gradient (slightly distorted
  // geometries, so the gradient is not zero).
  {
    struct Case { const char* name; const char* basis; std::vector<std::array<double, 4>> atoms; };
    const std::vector<Case> cases = {
        {"H2O", "STO-3G", {{8, 0, 0, 0.2216}, {1, 0, 1.4309, -0.8867}, {1, 0, -1.37, -0.9}}},
        {"H2O", "6-31G*", {{8, 0, 0, 0.2216}, {1, 0, 1.4309, -0.8867}, {1, 0, -1.37, -0.9}}},
        {"NH3", "STO-3G", {{7, 0, 0, 0.12}, {1, 1.77, 0, -0.55}, {1, -0.9, 1.5, -0.5}, {1, -0.85, -1.55, -0.6}}},
        {"NH3", "6-31G*", {{7, 0, 0, 0.12}, {1, 1.77, 0, -0.55}, {1, -0.9, 1.5, -0.5}, {1, -0.85, -1.55, -0.6}}},
        {"C2H4", "STO-3G", {{6, 0, 0, 1.26}, {6, 0, 0.1, -1.27}, {1, 0, 1.75, 2.33}, {1, 0.1, -1.75, 2.35}, {1, 0, 1.72, -2.36}, {1, 0, -1.74, -2.33}}},
        {"C2H4", "6-31G*", {{6, 0, 0, 1.26}, {6, 0, 0.1, -1.27}, {1, 0, 1.75, 2.33}, {1, 0.1, -1.75, 2.35}, {1, 0, 1.72, -2.36}, {1, 0, -1.74, -2.33}}}};
    for (const Case& cs : cases) {
      Molecule m;
      for (auto& r : cs.atoms) { Atom a; a.Z = static_cast<int>(r[0]); for (int k = 0; k < 3; ++k) a.x[k] = r[k + 1]; m.atoms.push_back(a); }
      ScfOptions so;
      so.conv_grad = 1e-9;
      so.conv_energy = 1e-12;
      Basis b0(m, lib, cs.basis, 1);
      const ScfResult ref = run_scf(b0, m, so);
      auto point = [&](const Molecule& mm) {
        Basis b(mm, lib, cs.basis, 1);
        ScfResult r = run_scf(b, mm, so, &ref);
        PointEval pe;
        pe.grad = scf_gradient(b, mm, so, r);
        for (int k = 0; k < 3; ++k) pe.dipole[k] = r.dipole[k];
        return pe;
      };
      const Derivatives num = finite_difference_derivatives(m, 0.002, point);  // the step's own error is ~1e-5 at 0.005
      const Derivatives ana = rhf_hessian(b0, m, so, ref);
      const std::string tag = std::string(cs.name) + "/" + cs.basis;
      check((ana.hessian - num.hessian).cwiseAbs().maxCoeff() < 1e-5, "analytic Hessian matches finite difference " + tag,
            (ana.hessian - num.hessian).cwiseAbs().maxCoeff());
      {
        const VibResult va = analyse_vibrations(m, ana), vn = analyse_vibrations(m, num);
        double df = 0;
        for (size_t k = 0; k < va.freq.size() && k < vn.freq.size(); ++k) df = std::max(df, std::fabs(va.freq[k] - vn.freq[k]));
        check(va.freq.size() == vn.freq.size() && df < 0.1, "analytic frequencies match finite difference (cm-1) " + tag, df);
      }
      check((ana.dmu - num.dmu).cwiseAbs().maxCoeff() < 1e-5, "analytic dipole derivatives match finite difference " + tag,
            (ana.dmu - num.dmu).cwiseAbs().maxCoeff());
    }
  }

  // Localised orbitals: the same occupied space (density, orbital-energy trace)
  // and the valence-bond picture of ethene and formamide.
  {
    struct Case { const char* name; std::vector<Atom> atoms; int core, sigma, pi, lp; };
    const double A = kAngstromToBohr;
    auto at = [&](int Z, double x, double y, double z) { Atom a; a.Z = Z; a.x[0] = x * A; a.x[1] = y * A; a.x[2] = z * A; return a; };
    const std::vector<Case> cases = {
        {"ethene", {at(6, 0, 0, .665), at(6, 0, 0, -.665), at(1, 0, .9235, 1.237), at(1, 0, -.9235, 1.237),
                    at(1, 0, .9235, -1.237), at(1, 0, -.9235, -1.237)}, 2, 5, 1, 0},
        {"formamide", {at(6, 0, .133, 0), at(8, 1.17, .2, 0), at(7, -.7, -1.02, 0), at(1, -.16, 1.08, 0),
                       at(1, -1.68, -1.02, 0), at(1, -.21, -1.89, 0)}, 3, 5, 1, 3}};
    for (const Case& cs : cases) {
      Molecule m;
      m.atoms = cs.atoms;
      Basis b(m, lib, "6-31G*", 1);
      ScfOptions so;
      const ScfResult r = run_scf(b, m, so);
      const Localised L = localise_occupied(m, b, r);
      const int no = r.nalpha;
      const Mat Dc = 2 * r.C_a.leftCols(no) * r.C_a.leftCols(no).transpose();
      const Mat Dl = 2 * L.C * L.C.transpose();
      check((Dc - Dl).cwiseAbs().maxCoeff() < 1e-9, std::string("localised density equals canonical ") + cs.name,
            (Dc - Dl).cwiseAbs().maxCoeff());
      check((L.C.transpose() * r.S * L.C - Mat::Identity(no, no)).cwiseAbs().maxCoeff() < 1e-9,
            std::string("localised orbitals orthonormal ") + cs.name);
      double tl = 0, tc = 0;
      for (int i = 0; i < no; ++i) { tl += L.C.col(i).dot(r.F_a * L.C.col(i)); tc += r.eps_a(i); }
      check(std::fabs(tl - tc) < 1e-8, std::string("localised orbital energies sum to the canonical sum ") + cs.name, tl - tc);
      int core = 0, sigma = 0, pi = 0, lp = 0, other = 0;
      for (const auto& o : L.orb) {
        if (o.kind == LocOrbital::Core) ++core;
        else if (o.kind == LocOrbital::Bond) (o.pi ? pi : sigma)++;
        else if (o.kind == LocOrbital::LonePair) ++lp;
        else ++other;
        std::cout << "      " << o.label << "\n";
      }
      check(core == cs.core && sigma == cs.sigma && pi == cs.pi && lp == cs.lp && other == 0,
            std::string("valence-bond labels of ") + cs.name, other);
    }
  }

  // Open shells: each spin's localised set spans the same occupied space as the
  // canonical one (per-spin density), for ROHF and UHF.
  {
    const double A = kAngstromToBohr;
    auto at = [&](int Z, double x, double y, double z) { Atom a; a.Z = Z; a.x[0] = x * A; a.x[1] = y * A; a.x[2] = z * A; return a; };
    struct OCase { const char* name; std::vector<Atom> atoms; Reference ref; };
    const std::vector<Atom> o2 = {at(8, 0, 0, 0.604), at(8, 0, 0, -0.604)};
    const std::vector<Atom> ch2 = {at(6, 0, 0, 0), at(1, 0.98, 0, 0.45), at(1, -0.98, 0, 0.45)};
    const std::vector<OCase> ocases = {{"O2 ROHF", o2, Reference::RestrictedOpen},
                                       {"O2 UHF", o2, Reference::Unrestricted},
                                       {"CH2 triplet UHF", ch2, Reference::Unrestricted}};
    for (const OCase& oc : ocases) {
      Molecule m;
      m.atoms = oc.atoms;
      m.multiplicity = 3;
      Basis b(m, lib, "STO-3G", 1);
      ScfOptions so;
      so.reference = oc.ref;
      const ScfResult r = run_scf(b, m, so);
      check(r.converged, std::string("converged ") + oc.name);
      for (int spin = 0; spin < 2; ++spin) {
        const Mat& C = spin ? r.C_b : r.C_a;
        const int no = spin ? r.nbeta : r.nalpha;
        const Localised L = localise_occupied(m, b, r, nullptr, spin);
        const Mat Dc = C.leftCols(no) * C.leftCols(no).transpose();
        const Mat Dl = L.C * L.C.transpose();
        check((Dc - Dl).cwiseAbs().maxCoeff() < 1e-9,
              std::string("localised ") + (spin ? "beta" : "alpha") + " density equals canonical " + oc.name,
              (Dc - Dl).cwiseAbs().maxCoeff());
        check((int)L.orb.size() == no, std::string("one localised orbital per occupied ") + oc.name);
        for (const auto& o : L.orb) std::cout << "      " << (spin ? "beta  " : "alpha ") << o.label << "\n";
      }
    }
  }

  // Improved virtual orbitals (Hunt-Goddard, F - J_h + 2 K_h): occupied set
  // untouched, virtuals orthonormal, F' symmetric, and eps'_a - eps_h equal to
  // the singlet h->a excitation energy built from MO integrals (a second
  // route); for He in aug-cc-pVDZ the lowest one is bound and near the
  // experimental 2 1S energy, 0.7578 Eh.
  for (const char* bn : {"aug-cc-pVDZ", "6-31+G*"}) {
    Molecule m;
    Atom a; a.Z = 2;
    m.atoms.push_back(a);
    Basis b(m, lib, bn, 1);
    ScfOptions so;
    const ScfResult r = run_scf(b, m, so);
    so.ivo = true;
    const ScfResult q = run_scf(b, m, so);
    const int n = b.nbf(), no = r.nalpha;
    const std::string tag = std::string(" He ") + bn;
    check(q.ivo_applied, "ivo applied" + tag);
    check((q.C_a.leftCols(no) - r.C_a.leftCols(no)).cwiseAbs().maxCoeff() < 1e-12 &&
          std::fabs(q.energy - r.energy) < 1e-12, "ivo leaves occupied and energy unchanged" + tag);
    check((q.C_a.transpose() * q.S * q.C_a - Mat::Identity(n, n)).cwiseAbs().maxCoeff() < 1e-9,
          "ivo orbitals orthonormal (virtuals orthogonal to occupied)" + tag);
    EriStore eri(b);
    Mat Cm = r.C_a, Fp;
    Vec e = r.eps_a;
    ivo_virtuals(r.F_a, r.S, eri, no, Cm, e, &Fp);
    check((Fp - Fp.transpose()).cwiseAbs().maxCoeff() < 1e-12, "F' Hermitian" + tag);
    double worst = 0;
    for (int k = no; k < n; ++k) {
      const Vec h = r.C_a.col(no - 1), v = q.C_a.col(k);
      double J = 0, K = 0;
      for (int i = 0; i < n; ++i) for (int j = 0; j < n; ++j) for (int kk = 0; kk < n; ++kk) for (int l = 0; l < n; ++l) {
        const double g = h(i) * h(j) * v(kk) * v(l);
        J += g * eri.get(i, j, kk, l);
        K += h(i) * v(j) * h(kk) * v(l) * eri.get(i, j, kk, l);
      }
      const double exc = v.dot(r.F_a * v) - r.eps_a(no - 1) - J + 2 * K;
      worst = std::max(worst, std::fabs(exc - (q.eps_a(k) - r.eps_a(no - 1))));
    }
    check(worst < 1e-9, "eps'_a - eps_h equals <a|F|a> - eps_h - J + 2K" + tag, worst);
    const double x = q.eps_a(no) - r.eps_a(no - 1);
    std::cout << "      lowest IVO " << q.eps_a(no) << "  canonical LUMO " << r.eps_a(no) << "  excitation " << x << "\n";
    if (std::string(bn) == "aug-cc-pVDZ")  // 6-31+G* has no diffuse function on He
      check(q.eps_a(no) < 0 && x > 0.6 && x < 0.9, "He lowest IVO bound, excitation near 0.7578 Eh" + tag, x);
  }

  // Open shells keep canonical virtuals and say so.
  {
    Molecule m;
    Atom a; a.Z = 3;
    m.atoms.push_back(a);
    m.multiplicity = 2;
    Basis b(m, lib, "STO-3G", 1);
    ScfOptions so;
    so.ivo = true;
    so.reference = Reference::Unrestricted;
    const ScfResult r = run_scf(b, m, so);
    check(!r.ivo_applied && !r.ivo_note.empty(), "ivo on an open shell is declined with a note");
  }

  // IRC stop rule (#452): a near-flat TS has a gradient below the end
  // thresholds from the first step, yet the path must go on to the minima
  // of the model E(x) = -a x^2 + c x^4 (x = r - r0, H2 masses) at |x| = 1.
  {
    const double a = 1e-3, c = 5e-4, r0 = 2.0;
    Molecule ts;
    ts.atoms = {{1, {0, 0, 0}}, {1, {0, 0, r0}}};
    auto eval = [&](const Molecule& m) {
      const double x = m.atoms[1].x[2] - m.atoms[0].x[2] - r0, d = -2 * a * x + 4 * c * x * x * x;
      OptEval e;
      e.energy = -a * x * x + c * x * x * x * x;
      e.grad = {0, 0, -d, 0, 0, d};
      return e;
    };
    Mat H = Mat::Zero(6, 6);
    Vec b(6);
    b << 0, 0, -1, 0, 0, 1;
    H = -2 * a * b * b.transpose();
    IrcOptions opt;
    opt.max_points = 40;
    for (int sign : {1, -1}) {
      std::string why;
      auto path = irc_path(ts, H, sign, opt, eval, [&](const Molecule&) { return H; }, nullptr, &why);
      const double x = path.back().mol.atoms[1].x[2] - path.back().mol.atoms[0].x[2] - r0;
      check(path.size() >= 5 && std::fabs(std::fabs(x) - 1.0) < 0.05 && why.find("minimum reached") != std::string::npos,
            "irc runs from a flat TS to the minimum (" + std::to_string(path.size()) + " points: " + why + ")", x);
    }
  }
  // Implicit solvent (CPCM).  Born ion: a charge in the middle of one sphere
  // has the energy -(1 - 1/eps) Z^2 / (2R) exactly; eps -> 1 gives nothing; the
  // energy of a fixed external charge is smooth when an atom moves through
  // the other's cavity (no point switches abruptly).
  {
    const double eps = 78.3553;
    Molecule ion;
    Atom a; a.Z = 8;  // radius from the table; the nuclear charge is the potential, scaled below
    ion.atoms.push_back(a);
    Cpcm c(ion, eps);
    const double R = 1.2 * cavity_radius(8) * kAngstromToBohr;
    Vec V = Vec::Constant(c.size(), 1.0 / R);  // a unit charge at the centre
    const double born = -0.5 * (1 - 1 / eps) / R;
    const Vec q = c.charges(V);
    check(std::fabs(0.5 * q.dot(V) / born - 1) < 0.005, "Born ion energy against the analytic formula (ratio - 1)", 0.5 * q.dot(V) / born - 1);
    check(std::fabs(q.sum() / (-(1 - 1 / eps)) - 1) < 0.005, "Born ion total surface charge is -(1 - 1/eps)", q.sum());
    Cpcm c1(ion, 1.0);
    check(c1.charges(V).norm() == 0.0, "eps = 1: no surface charge");

    // two atoms, one moving by 0.002 bohr steps through 1 bohr; the probe is a unit charge
    Molecule two;
    Atom o; o.Z = 8;
    Atom h; h.Z = 1;
    two.atoms = {o, h};
    const double probe[3] = {2.0, 3.0, 1.0};
    std::vector<double> en;
    for (int i = 0; i <= 500; ++i) {
      two.atoms[1].x[0] = 3.0 + 0.002 * i;
      Cpcm cc(two, eps);
      Vec v(cc.size());
      for (int k = 0; k < cc.size(); ++k) {
        const auto& p = cc.points()[k];
        v(k) = 1.0 / std::sqrt((p[0] - probe[0]) * (p[0] - probe[0]) + (p[1] - probe[1]) * (p[1] - probe[1]) + (p[2] - probe[2]) * (p[2] - probe[2]));
      }
      en.push_back(0.5 * cc.charges(v).dot(v));
    }
    double maxd2 = 0;
    for (size_t i = 1; i + 1 < en.size(); ++i) maxd2 = std::max(maxd2, std::fabs(en[i + 1] - 2 * en[i] + en[i - 1]));
    check(maxd2 < 2e-5, "solvation energy continuous while one cavity sphere slides through another (max second difference)", maxd2);
  }
  {
    // eps = 1 changes nothing in a whole SCF; a real solvent lowers the energy of water
    Molecule w;
    const double A = kAngstromToBohr;
    w.atoms = {{8, {0, 0, 0.1173 * A}}, {1, {0, 0.7572 * A, -0.4692 * A}}, {1, {0, -0.7572 * A, -0.4692 * A}}};
    Basis b(w, lib, "STO-3G", 1);
    ScfOptions o;
    const ScfResult r0 = run_scf(b, w, o);
    o.solvent_eps = 1.0;
    const ScfResult r1 = run_scf(b, w, o);
    check(r0.converged && std::fabs(r1.energy - r0.energy) < 1e-9 && r1.e_solvent == 0.0, "scf with eps = 1 is the vacuum energy", r1.energy - r0.energy);
    o.solvent_eps = 78.3553;
    const ScfResult r2 = run_scf(b, w, o);
    check(r2.converged && r2.e_solvent < -0.005 && r2.energy < r0.energy - 0.005, "water in water: energy lowered by the reaction field", r2.energy - r0.energy);
  }

  return fails ? 1 : 0;
}
