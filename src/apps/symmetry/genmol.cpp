#include "symmetry_cpp.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {
bool same(const ecce_symmetry::Atom& a, const ecce_symmetry::Atom& b, double tol) {
  return a.charge == b.charge && std::abs(a.xyz[0] - b.xyz[0]) <= tol &&
         std::abs(a.xyz[1] - b.xyz[1]) <= tol && std::abs(a.xyz[2] - b.xyz[2]) <= tol;
}
}

// Expand a molecular asymmetric unit. Keep the caller's original coordinates
// first, followed by only the generated images, matching genmol.f's contract.
int main() {
  int count = 0;
  double tolerance = 0.0;
  std::string group;
  if (!(std::cin >> count >> tolerance >> group)) return 1;
  if (count < 0 || count > 2000) return 2;
  std::vector<ecce_symmetry::Atom> input(static_cast<std::size_t>(count));
  for (ecce_symmetry::Atom& atom : input)
    if (!(std::cin >> atom.tag >> atom.charge >> atom.xyz[0] >> atom.xyz[1] >> atom.xyz[2])) return 1;
  std::vector<ecce_symmetry::Matrix> operations;
  if (!ecce_symmetry::molecularGroupOperations(group, operations)) return 3;
  const ecce_symmetry::Point cell{{0.0, 0.0, 0.0}};
  auto expanded = ecce_symmetry::applyOperations(input, operations, 0, tolerance, cell);
  std::vector<ecce_symmetry::Atom> output = input;
  for (const auto& atom : expanded) {
    bool original = false;
    for (const auto& source : input) if (same(atom, source, tolerance)) { original = true; break; }
    if (!original) output.push_back(atom);
  }
  std::cout << std::setw(8) << output.size() << "\n\n";
  for (const auto& atom : output) {
    std::cout << std::left << std::setw(16) << atom.tag << std::right;
    for (double value : atom.xyz)
      std::cout << std::uppercase << std::scientific << std::setprecision(14) << std::setw(24) << value;
    std::cout << '\n';
  }
  return 0;
}
