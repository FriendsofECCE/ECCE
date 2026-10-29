#include "symmetry_cpp.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

// Read the Builder's atom list, construct the group operators, and print the
// irreducible fragment size followed by each selected input atom's 1-based
// position. The caller uses those positions to preserve atom identity.
int main() {
  int count = 0;
  double tolerance = 0.0;
  std::string group;
  if (!(std::cin >> count >> tolerance >> group)) return 1;
  if (count < 0 || count > 2000) return 2;
  std::vector<ecce_symmetry::Atom> atoms(static_cast<std::size_t>(count));
  for (ecce_symmetry::Atom& atom : atoms)
    if (!(std::cin >> atom.tag >> atom.charge >> atom.xyz[0] >> atom.xyz[1] >> atom.xyz[2])) return 1;
  std::vector<ecce_symmetry::Matrix> operations;
  if (!ecce_symmetry::molecularGroupOperations(group, operations)) return 3;
  std::vector<int> order;
  const auto fragment = ecce_symmetry::findIrreducible(atoms, operations, tolerance, &order);
  std::cout << std::setw(8) << fragment.size() << '\n';
  for (int atom : order) std::cout << std::setw(8) << atom << '\n';
  return 0;
}
