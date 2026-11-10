// Grimme's DFT-D3 dispersion correction with Becke-Johnson damping, D3(BJ):
// S. Grimme, J. Antony, S. Ehrlich, H. Krieg, J. Chem. Phys. 132, 154104
// (2010); S. Grimme, S. Ehrlich, L. Goerigk, J. Comput. Chem. 32, 1456 (2011).
// Two-body C6 and C8 terms only; the three-body (ATM) term is left out, as in
// ORCA's D3BJ.  Reference data: third_party/simple-dftd3-data.
#pragma once
#include <string>
#include <vector>
#include "molecule.hpp"

namespace qm {

struct D3Params {
  double s6 = 1, s8 = 0, a1 = 0, a2 = 0;  // a2 in bohr
};

// The published D3(BJ) parameters for an ECCE-QM method (hf, pbe, b3lyp,
// pbe0); false when there are none (svwn).
bool d3bj_params(const std::string& method, D3Params& p);

// Dispersion energy (hartree) of the molecule (bohr; Z = 0 atoms ignored).
// With grad, also dE/dx: 3 values per atom of mol, added to *grad, which must
// hold 3 * mol.atoms.size() values.
double d3bj_energy(const Molecule& mol, const D3Params& p, std::vector<double>* grad = nullptr);

}  // namespace qm
