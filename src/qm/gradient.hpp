// Analytic nuclear gradient of the SCF energy (RHF/UHF/ROHF and the Kohn-Sham
// equivalents).  The DFT part neglects the derivative of the grid weights.
#pragma once
#include <vector>
#include "basis.hpp"
#include "molecule.hpp"
#include "scf.hpp"

namespace qm {

// dE/dR in hartree/bohr, 3 per atom (x, y, z of atom 0, then atom 1, ...).
// r must come from run_scf on this basis, molecule and options.
std::vector<double> scf_gradient(Basis& basis, const Molecule& mol, const ScfOptions& opt, const ScfResult& r);

}  // namespace qm
