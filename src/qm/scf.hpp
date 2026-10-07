// Self-consistent field: RHF/UHF/ROHF and the Kohn-Sham equivalents.
#pragma once
#include <string>
#include <vector>
#include "basis.hpp"
#include "common.hpp"
#include "molecule.hpp"
#include "xc.hpp"

namespace qm {

enum class Reference { Restricted, Unrestricted, RestrictedOpen };

struct ScfOptions {
  std::string method = "hf";      // hf, svwn, pbe, b3lyp, pbe0
  Reference reference = Reference::Restricted;
  int max_iter = 150;
  double conv_energy = 1e-10;     // hartree
  double conv_grad = 1e-7;        // max |orbital gradient|
  int grid_radial = 99;
  int grid_angular = 590;
  bool verbose = false;
};

struct ScfResult {
  bool converged = false;
  int iterations = 0;
  double energy = 0, e_nuc = 0, e_one = 0, e_coulomb = 0, e_exchange_hf = 0, e_xc = 0;
  Reference reference = Reference::Restricted;
  // Orbitals.  Restricted and restricted-open: eps_b = eps_a, C_b = C_a.
  Vec eps_a, eps_b;
  Mat C_a, C_b;
  Vec occ_a, occ_b;               // 0 or 1 per spin orbital
  Mat D_a, D_b;
  Mat S;
  std::vector<double> mulliken;   // net atomic charges
  double dipole[3] = {0, 0, 0};   // a.u., origin at the coordinate origin
  double s2 = 0;                  // <S^2> (UHF/UKS; S(S+1) otherwise)
  double xc_electrons = 0;        // grid integral of the density (DFT)
  int nalpha = 0, nbeta = 0;
};

ScfResult run_scf(Basis& basis, const Molecule& mol, const ScfOptions& opt);

}  // namespace qm
