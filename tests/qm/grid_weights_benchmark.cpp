// Manual timing: qm_grid_weights_benchmark BASISDIR INPUT.qm [...]
// Generate inputs from molecules.py (h2o, hexane, dodecane), B3LYP/6-31G*,
// grid 99 590, grid_prune true; run with OMP_NUM_THREADS=3 under srun -c 3.
// Time one full analytic gradient separately from the preceding SCF.
#include <chrono>
#include <iomanip>
#include <iostream>
#include "gradient.hpp"
#include "input.hpp"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "usage: qm_grid_weights_benchmark BASISDIR INPUT.qm [...]\n";
    return 2;
  }
  try {
    qm::BasisLibrary lib(argv[1]);
    for (int i = 2; i < argc; ++i) {
      const qm::Input in = qm::parse_input_file(argv[i]);
      qm::Basis basis(in.mol, lib, in.basis, in.spherical);
      using Clock = std::chrono::steady_clock;
      const auto start = Clock::now();
      const qm::ScfResult scf = qm::run_scf(basis, in.mol, in.scf);
      const auto ready = Clock::now();
      if (!scf.converged) throw qm::Error("benchmark SCF did not converge");
      const auto grad = qm::scf_gradient(basis, in.mol, in.scf, scf);
      const auto done = Clock::now();
      std::cout << std::setprecision(17) << "case " << in.title
                << " atoms " << in.mol.atoms.size() << " nbf " << basis.nbf()
                << " scf_seconds " << std::chrono::duration<double>(ready - start).count()
                << " gradient_seconds " << std::chrono::duration<double>(done - ready).count()
                << " energy " << scf.energy << "\ngradient";
      for (double v : grad) std::cout << ' ' << v;
      std::cout << std::endl;
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
