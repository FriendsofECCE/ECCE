#include "symmetry_cpp.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

// Command-line counterpart to cleansym.f. The input/output layout remains
// identical so SymmetryOps and the corpus runner can swap implementations.
int main() {
  int atomCount = 0;
  double threshold = 0.0;
  std::string group;
  if (!(std::cin >> atomCount)) return 1;
  if (atomCount > 2000) return 2;
  if (atomCount < 0 || !(std::cin >> threshold >> group)) return 1;

  std::vector<ecce_symmetry::Point> atoms(static_cast<std::size_t>(atomCount));
  for (auto& atom : atoms)
    if (!(std::cin >> atom[0] >> atom[1] >> atom[2])) return 1;

  std::vector<ecce_symmetry::Matrix> generators;
  if (!ecce_symmetry::generatorSetFor(group, generators))
    return ecce_symmetry::errorExit("cleansym: unsupported point group " + group, 3);
  const auto operations = ecce_symmetry::generateOperations(generators);
  if (operations.empty() && group != "C1")
    return ecce_symmetry::errorExit("cleansym: could not generate point group " + group, 1);
  const auto cleaned = ecce_symmetry::projectToSymmetry(atoms, operations);

  std::cout << std::setw(8) << atomCount << '\n' << std::scientific
            << std::setprecision(14);
  for (const auto& atom : cleaned) {
    for (double coordinate : atom) std::cout << std::setw(24) << coordinate;
    std::cout << '\n';
  }
  (void)threshold; // The Fortran implementation reads but does not use it.
  return 0;
}
