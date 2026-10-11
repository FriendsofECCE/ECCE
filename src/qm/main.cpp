// ecce-qm: command line driver.
//   ecce-qm [--basis-dir DIR] [--molden FILE] [-o OUT] [-v] input.qm
#include <chrono>
#include <cmath>
#include <fstream>
#include <memory>
#include <numeric>
#include <iostream>
#include <vector>
#include "ao.hpp"
#include "basis.hpp"
#include "gradient.hpp"
#include "input.hpp"
#include "output.hpp"
#include "scf.hpp"

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

    // The basis belongs to one geometry; keep the one of the latest evaluation.
    std::unique_ptr<qm::Basis> basis;
    qm::ScfResult r;
    qm::Molecule at;
    auto evaluate = [&](const qm::Molecule& m) {
      basis.reset(new qm::Basis(m, lib, in.basis, in.spherical));
      at = m;
      r = qm::run_scf(*basis, m, in.scf);
      qm::OptEval e;
      e.energy = r.energy;
      if (in.runtype != qm::RunType::Energy) {
        if (!r.converged) throw qm::Error("SCF did not converge");
        e.grad = qm::scf_gradient(*basis, m, in.scf, r);
      }
      return e;
    };
    auto same = [](const qm::Molecule& a, const qm::Molecule& b) {
      for (size_t i = 0; i < a.atoms.size(); ++i)
        for (int k = 0; k < 3; ++k)
          if (a.atoms[i].x[k] != b.atoms[i].x[k]) return false;
      return true;
    };

    if (in.runtype == qm::RunType::Optimize) {
      qm::OptResult res = qm::optimize_geometry(in.mol, in.opt, evaluate, [&](const qm::OptStep& st) {
        qm::write_opt_step(os, st);
        if (verbose)
          std::cerr << "step " << st.index << "  E = " << st.energy << "  max|g| = " << st.max_grad << "\n";
      });
      if (!same(at, res.mol)) evaluate(res.mol);  // the last trial was turned down
      qm::write_text_output(os, in, *basis, r, seconds(), &res);
      if (!molden.empty()) {
        std::ofstream f(molden);
        qm::write_molden(f, in, *basis, r);
      }
      return res.converged ? 0 : 1;
    }
    qm::OptEval e = evaluate(in.mol);
    if (in.runtype == qm::RunType::Gradient) {
      qm::OptStep st;
      st.index = 1;
      st.mol = in.mol;
      st.energy = e.energy;
      st.grad = e.grad;
      for (double v : e.grad) st.max_grad = std::max(st.max_grad, std::fabs(v));
      st.rms_grad = std::sqrt(std::inner_product(e.grad.begin(), e.grad.end(), e.grad.begin(), 0.0) / e.grad.size());
      qm::write_opt_step(os, st);
    }
    qm::write_text_output(os, in, *basis, r, seconds());
    if (!molden.empty()) {
      std::ofstream f(molden);
      qm::write_molden(f, in, *basis, r);
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
