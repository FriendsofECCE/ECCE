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
};

int atomic_number(const std::string& symbol);  // case-insensitive; throws
const char* element_symbol(int Z);             // "H", "He", ...; "X" if unknown

}  // namespace qm
