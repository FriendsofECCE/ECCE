#pragma once
#include <string>
#include <vector>
#include "common.hpp"

namespace qm {

struct Atom {
  int Z = 0;
  double x[3] = {0, 0, 0};  // bohr
};

struct Molecule {
  std::vector<Atom> atoms;
  int charge = 0;
  int multiplicity = 1;

  int nelectrons() const;            // sum(Z) - charge
  int nalpha() const;                // from multiplicity
  int nbeta() const;
  double nuclear_repulsion() const;  // hartree
  void check() const;                // electron count vs multiplicity

  // Dummy atoms (Z = 0, symbol X) are centres of the geometry only: no
  // nucleus, electrons or basis.  The electronic structure code sees
  // without_dummies(); the optimiser and the output keep the full list so
  // atom numbers match ECCE's.
  bool has_dummy() const;
  Molecule without_dummies() const;
  std::vector<int> real_atoms() const;   // full index of each non-dummy atom
};

int atomic_number(const std::string& symbol);  // case-insensitive; throws
const char* element_symbol(int Z);             // "H", "He", ...; "X" if unknown

}  // namespace qm
