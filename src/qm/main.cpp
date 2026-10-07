// ecce-qm: command line driver.
//   ecce-qm [--basis-dir DIR] [--molden FILE] [-o OUT] [-v] input.qm
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include "ao.hpp"
#include "basis.hpp"
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
    qm::Basis basis(in.mol, lib, in.basis, in.spherical);
    auto t0 = std::chrono::steady_clock::now();
    qm::ScfResult r = qm::run_scf(basis, in.mol, in.scf);
    double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    if (outp.empty()) qm::write_text_output(std::cout, in, basis, r, sec);
    else {
      std::ofstream f(outp);
      qm::write_text_output(f, in, basis, r, sec);
    }
    if (!molden.empty()) {
      std::ofstream f(molden);
      qm::write_molden(f, in, basis, r);
    }
    if (probe.size() == 3) {
      // orbital values and electron density at a point, for checking exports
      double p[3] = {probe[0] * qm::kAngstromToBohr, probe[1] * qm::kAngstromToBohr, probe[2] * qm::kAngstromToBohr};
      qm::AoValues ao;
      qm::eval_ao(basis, p, 1, 0, ao);
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
