// ecce-qm: command line driver.
//   ecce-qm [--basis-dir DIR] [--molden FILE] [-o OUT] [-v] input.qm
#include <chrono>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <memory>
#include <mutex>
#include <numeric>
#include <iostream>
#include <vector>
#include "ao.hpp"
#include "basis.hpp"
#include "charges.hpp"
#include "gradient.hpp"
#include "hessian.hpp"
#include "input.hpp"
#include "integrals.hpp"
#include "output.hpp"
#include "polar.hpp"
#include "scf.hpp"
#include "vibration.hpp"

int main(int argc, char** argv) {
  std::string inp, outp, molden, bdir;
  bool verbose = false;
  std::vector<double> probe;  // x y z in angstrom
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--basis-dir" && i + 1 < argc) bdir = argv[++i];
    else if (a == "--molden" && i + 1 < argc) molden = argv[++i];
    else if (a == "-o" && i + 1 < argc) outp = argv[++i];
    else if (a == "-v") verbose = true;
    else if (a == "--probe" && i + 3 < argc) { for (int k = 0; k < 3; ++k) probe.push_back(std::atof(argv[++i])); }
    else if (a == "-h" || a == "--help") {
      std::cout << "usage: ecce-qm [--basis-dir DIR] [--molden FILE] [-o OUT] [-v] input\n";
      return 0;
    } else inp = a;
  }
  if (inp.empty()) { std::cerr << "ecce-qm: no input file\n"; return 2; }
  try {
    qm::Input in = qm::parse_input_file(inp);
    in.scf.verbose = verbose;
    qm::BasisLibrary lib(bdir);
    for (auto& kv : in.explicit_basis) lib.set_explicit(kv.first, kv.second);
    std::ofstream fout;
    if (!outp.empty()) fout.open(outp);
    std::ostream& os = outp.empty() ? std::cout : fout;
    auto t0 = std::chrono::steady_clock::now();
    auto seconds = [&] { return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(); };

    in.scf.on_cycle = [&](const qm::ScfCycle& c) { qm::write_progress_scf(os, c, in.scf); };

    // The basis belongs to one geometry; keep the one of the latest evaluation.
    std::unique_ptr<qm::Basis> basis;
    qm::ScfResult r;
    qm::Molecule at;        // the molecule the SCF saw: no dummy atoms
    qm::Molecule at_full;   // ... and with them, as the optimiser has it
    // The previous geometry's orbitals start the next SCF (optimisation steps).
    bool have_prev = false;
    auto evaluate = [&](const qm::Molecule& mfull) {
      const qm::Molecule m = mfull.without_dummies();
      basis.reset(new qm::Basis(m, lib, in.basis, in.spherical));
      at = m;
      at_full = mfull;
      qm::ScfResult next = qm::run_scf(*basis, m, in.scf, have_prev ? &r : nullptr);
      r = std::move(next);
      have_prev = true;
      if (!r.converged) qm::write_progress_scf_failed(os, r.iterations);
      qm::OptEval e;
      e.energy = r.energy;
      if (in.runtype != qm::RunType::Energy) {
        if (!r.converged) throw qm::Error("SCF did not converge");
        const std::vector<double> g = qm::scf_gradient(*basis, m, in.scf, r);
        e.grad.assign(3 * mfull.atoms.size(), 0.0);   // a dummy atom feels no force
        const std::vector<int> idx = mfull.real_atoms();
        for (size_t i = 0; i < idx.size(); ++i)
          for (int k = 0; k < 3; ++k) e.grad[3 * idx[i] + k] = g[3 * i + k];
      }
      return e;
    };
    auto extra_charges = [&]() {
      std::unique_ptr<qm::ChargeSets> cs;
      if (in.charges.any() && r.converged) cs.reset(new qm::ChargeSets(qm::compute_charges(*basis, at, r, in.charges)));
      return cs;
    };
    // Static polarisability of the geometry the last SCF saw.
    auto polar_output = [&]() {
      std::unique_ptr<qm::PolarOutput> po;
      if ((in.polarizability || in.raman) && r.converged) {
        po.reset(new qm::PolarOutput);
        po->step = in.field_step;
        po->alpha = qm::static_polarizability(*basis, at, in.scf, r, in.field_step);
      }
      return po;
    };
    auto same = [](const qm::Molecule& a, const qm::Molecule& b) {
      for (size_t i = 0; i < a.atoms.size(); ++i)
        for (int k = 0; k < 3; ++k)
          if (a.atoms[i].x[k] != b.atoms[i].x[k]) return false;
      return true;
    };

    // Frequencies: finite differences of the analytic gradient around the
    // reference geometry, every displaced SCF started from the reference
    // orbitals and converged tighter than an energy needs.
    auto frequencies = [&](const qm::Molecule& ref, const std::vector<double>& g0) {
      qm::FrequencyOutput fo;
      fo.step = in.hessian_step;
      for (double v : g0) fo.max_gradient = std::max(fo.max_gradient, std::fabs(v));
      qm::ScfResult r_ref = r;
      qm::ScfOptions so = in.scf;
      so.conv_grad = std::min(so.conv_grad, 1e-8);
      so.conv_energy = std::min(so.conv_energy, 1e-11);
      so.max_iter = std::max(so.max_iter, 200);
      so.on_cycle = [&](const qm::ScfCycle& c) { qm::write_progress_scf(os, c, so); };
      // Closed-shell RHF: analytic Hessian and dipole derivatives (one tightly
      // converged SCF).  Raman needs polarisability derivatives, and the
      // in-core integrals may not fit; both keep the finite-difference path.
      if (!in.numeric_hessian && !std::getenv("ECCE_QM_NUMERIC_HESSIAN") && !in.raman &&
          qm::analytic_hessian_available(in.scf, r_ref)) {
        try {
          qm::ScfOptions so2 = so;
          so2.on_cycle = nullptr;
          qm::ScfResult rt = qm::run_scf(*basis, ref, so2, &r_ref);
          if (!rt.converged) throw qm::Error("SCF did not converge");
          const int np = 3 * static_cast<int>(ref.atoms.size());
          qm::write_progress_freq(os, 0, np);
          qm::Derivatives d = qm::rhf_hessian(*basis, ref, so2, rt);
          fo.step = 0;  // no displacements
          fo.vib = qm::analyse_vibrations(ref, d);
          fo.thermo = qm::thermochemistry(ref, fo.vib, in.temperature, in.pressure, in.symmetry_number);
          qm::write_progress_freq_end(os, np, fo);
          return fo;
        } catch (const qm::Error& e) {
          std::cerr << "ecce-qm: analytic Hessian unavailable (" << e.what() << "); using finite differences\n";
        }
      }
      // Displaced points run side by side when memory allows; the per-cycle
      // SCF lines of concurrent points would interleave, so they are dropped.
      qm::Concurrency conc;
      if (in.parallel_points) {
        qm::Basis b0(ref, lib, in.basis, in.spherical);
        conc = qm::plan_concurrency(6 * static_cast<int>(ref.atoms.size()),
                                    1.3 * qm::EriStore::bytes_needed(b0.nbf()) + 3e8);
      }
      if (conc.workers > 1) so.on_cycle = nullptr;
      if (verbose) std::cerr << "frequency points: " << conc.workers << " at once\n";
      std::mutex os_mu;
      auto point = [&](const qm::Molecule& m) {
        qm::Basis b(m, lib, in.basis, in.spherical);
        qm::ScfResult rr = qm::run_scf(b, m, so, &r_ref);
        if (!rr.converged) {
          std::lock_guard<std::mutex> lk(os_mu);
          qm::write_progress_scf_failed(os, rr.iterations);
          throw qm::Error("SCF did not converge at a displaced geometry");
        }
        qm::PointEval pe;
        pe.grad = qm::scf_gradient(b, m, so, rr);
        for (int k = 0; k < 3; ++k) pe.dipole[k] = rr.dipole[k];
        if (in.raman) {
          const qm::Mat a = qm::static_polarizability(b, m, so, rr, in.field_step);
          for (int ij = 0; ij < 9; ++ij) pe.alpha[ij] = a(ij / 3, ij % 3);
        }
        return pe;
      };
      const int points = 6 * static_cast<int>(ref.atoms.size());
      qm::write_progress_freq(os, 0, points);
      qm::Derivatives d = qm::finite_difference_derivatives(ref, in.hessian_step, point, [&](int k, int n) {
        qm::write_progress_freq(os, k, n);
        if (verbose) std::cerr << "hessian point " << k << "/" << n << "\n";
      }, conc.workers, conc.threads);
      fo.vib = qm::analyse_vibrations(ref, d);
      fo.thermo = qm::thermochemistry(ref, fo.vib, in.temperature, in.pressure, in.symmetry_number);
      qm::write_progress_freq_end(os, points, fo);
      return fo;
    };

    if (qm::does_optimize(in.runtype)) {
      int last_step = 0;
      qm::OptResult res = qm::optimize_geometry(in.mol, in.opt, evaluate, [&](const qm::OptStep& st) {
        qm::write_opt_step(os, st);
        qm::write_progress_opt(os, st, in.opt);
        last_step = st.index;
        if (verbose)
          std::cerr << "step " << st.index << "  E = " << st.energy << "  max|g| = " << st.max_grad << "\n";
      });
      qm::write_progress_opt_end(os, last_step, res.converged);
      if (!same(at_full, res.mol)) evaluate(res.mol);  // the last trial was turned down
      std::unique_ptr<qm::FrequencyOutput> fo;
      if (in.runtype == qm::RunType::OptFreq && res.converged) fo.reset(new qm::FrequencyOutput(frequencies(res.mol.without_dummies(), res.grad)));
      auto cs = extra_charges();
      auto po = polar_output();
      qm::write_text_output(os, in, *basis, r, seconds(), &res, fo.get(), cs.get(), &at_full, po.get());
      if (!molden.empty()) {
        std::ofstream f(molden);
        qm::write_molden(f, in, *basis, r, fo.get());
      }
      return res.converged ? 0 : 1;
    }
    qm::OptEval e = evaluate(in.mol);
    if (in.runtype == qm::RunType::Gradient || in.runtype == qm::RunType::Frequency) {
      qm::OptStep st;
      st.index = 1;
      st.mol = in.mol;
      st.energy = e.energy;
      st.grad = e.grad;
      for (double v : e.grad) st.max_grad = std::max(st.max_grad, std::fabs(v));
      st.rms_grad = std::sqrt(std::inner_product(e.grad.begin(), e.grad.end(), e.grad.begin(), 0.0) / e.grad.size());
      qm::write_opt_step(os, st);
    }
    std::unique_ptr<qm::FrequencyOutput> fo;
    if (in.runtype == qm::RunType::Frequency && r.converged) fo.reset(new qm::FrequencyOutput(frequencies(in.mol.without_dummies(), e.grad)));
    auto cs = extra_charges();
    auto po = polar_output();
    qm::write_text_output(os, in, *basis, r, seconds(), nullptr, fo.get(), cs.get(), &at_full, po.get());
    if (!molden.empty()) {
      std::ofstream f(molden);
      qm::write_molden(f, in, *basis, r, fo.get());
    }
    if (probe.size() == 3) {
      // orbital values and electron density at a point, for checking exports
      double p[3] = {probe[0] * qm::kAngstromToBohr, probe[1] * qm::kAngstromToBohr, probe[2] * qm::kAngstromToBohr};
      qm::AoValues ao;
      qm::eval_ao(*basis, p, 1, 0, ao);
      qm::Vec psi = (ao.phi * r.C_a).transpose();
      double rho = (ao.phi * (r.D_a + r.D_b) * ao.phi.transpose())(0, 0);
      std::cout << "probe_density " << rho << "\n";
      for (int i = 0; i < std::min<int>(psi.size(), 12); ++i) std::cout << "probe_mo_alpha " << i + 1 << " " << psi(i) << "\n";
    }
    return r.converged ? 0 : 1;
  } catch (const std::exception& e) {
    std::cerr << "ecce-qm: " << e.what() << "\n";
    return 2;
  }
}
