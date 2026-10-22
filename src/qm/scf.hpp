// Self-consistent field: RHF/UHF/ROHF and the Kohn-Sham equivalents.
#pragma once
#include <functional>
#include <string>
#include <vector>
#include "basis.hpp"
#include "common.hpp"
#include "molecule.hpp"
#include "xc.hpp"

namespace qm {

enum class Reference { Restricted, Unrestricted, RestrictedOpen };

// One SCF cycle, for progress reports.  dE is NaN on the first cycle.
struct ScfCycle {
  int cycle = 0;
  double energy = 0, dE = 0, grad = 0;
  bool converged = false;
};

struct ScfOptions {
  std::string method = "hf";      // hf, svwn, pbe, b3lyp, pbe0
  Reference reference = Reference::Restricted;
  int max_iter = 150;
  double conv_energy = 1e-10;     // hartree
  double conv_grad = 1e-7;        // max |orbital gradient|
  int grid_radial = 99;
  int grid_angular = 590;
  bool grid_prune = true;         // thin the angular grid near the nuclei
  bool verbose = false;
  // Unrestricted closed-shell-like start (na == nb): rotate the alpha HOMO
  // and LUMO of the starting orbitals by 45 degrees, so that a spin-polarised
  // (broken-symmetry) solution can be found where one exists; where it does
  // not (a stable closed-shell solution) the SCF returns to it.
  bool guess_mix = false;
  // Uniform electric field (a.u.): H += F.r.  Used by the finite-field
  // polarisability; the dipole printed is unchanged in definition.
  double field[3] = {0, 0, 0};
  std::function<void(const ScfCycle&)> on_cycle;  // called after every cycle
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
  Mat F_a, F_b;                   // Fock matrices of the last iteration (gradient)
  Mat S;
  std::vector<double> mulliken;   // net atomic charges
  double dipole[3] = {0, 0, 0};   // a.u., origin at the coordinate origin
  double s2 = 0;                  // <S^2> (UHF/UKS; S(S+1) otherwise)
  double xc_electrons = 0;        // grid integral of the density (DFT)
  int nalpha = 0, nbeta = 0;
};

// guess: the converged result of a nearby geometry of the same molecule and
// basis, used instead of the atomic-density guess (ignored if it does not fit).
ScfResult run_scf(Basis& basis, const Molecule& mol, const ScfOptions& opt, const ScfResult* guess = nullptr);

}  // namespace qm
