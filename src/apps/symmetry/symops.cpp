#include "symmetry_cpp.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>

// Public matrix view of the same operators used by the symmetry utilities.
// The identity is printed first because gensym's internal contract omits it.
int main(int argc, char** argv) {
  std::string group;
  if (argc > 1) group = argv[1];
  else if (!(std::cin >> group)) return 2;
  if (std::find(ecce_symmetry::molecularGroups().begin(),
                ecce_symmetry::molecularGroups().end(), group) ==
      ecce_symmetry::molecularGroups().end()) return 2;
  std::vector<ecce_symmetry::Matrix> operations;
  if (!ecce_symmetry::molecularGroupOperations(group, operations)) return 2;
  std::cout << std::setw(8) << operations.size() + 1 << '\n';
  const ecce_symmetry::Matrix identity{{{{1,0,0}},{{0,1,0}},{{0,0,1}}}};
  auto print = [](const ecce_symmetry::Matrix& matrix) {
    for (int row = 0; row < 3; ++row)
      std::cout << std::uppercase << std::scientific << std::setprecision(14)
                << std::setw(24) << matrix[row][0] << std::setw(24) << matrix[row][1]
                << std::setw(24) << matrix[row][2] << std::setw(24) << 0.0 << '\n';
  };
  print(identity);
  for (const auto& operation : operations) print(operation);
  return 0;
}
