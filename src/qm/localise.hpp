// Localised occupied orbitals (Pipek-Mezey, Loewdin populations) and their
// valence-bond labels: core, sigma/pi bond, lone pair, with hybridisation.
#pragma once
#include <string>
#include <vector>
#include "basis.hpp"
#include "molecule.hpp"
#include "scf.hpp"

namespace qm {

struct LocOrbital {
  enum Kind { Core, Bond, LonePair, Multicentre } kind = Multicentre;
  bool pi = false;               // Bond only
  std::vector<int> atoms;        // basis-molecule atom indices, largest population first
  std::vector<double> pops;      // Loewdin populations of those atoms (sum <= 1)
  std::vector<double> hybrid_n;  // p/s ratio per centre, -1 where the centre has no p
  double energy = 0;             // <phi|F|phi>, hartree
  std::string label;
};

struct Localised {
  Mat C;                         // nbf x nocc, same AO order as ScfResult::C_a
  std::vector<LocOrbital> orb;   // one per column
  int sweeps = 0;
};

// Closed-shell (Restricted) results only; the occupied orbitals are the first
// nalpha columns of C_a.  atom_number: number shown for each atom of mol
// (default index + 1), so a molecule with dummy atoms keeps ECCE's numbering.
Localised localise_occupied(const Molecule& mol, const Basis& basis, const ScfResult& r,
                            const std::vector<int>* atom_number = nullptr);

}  // namespace qm
