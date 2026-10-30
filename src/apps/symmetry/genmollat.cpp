#include "symmetry_cpp.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {
bool samePeriodic(const ecce_symmetry::Atom& a, const ecce_symmetry::Atom& b,
                  double tolerance, const ecce_symmetry::Point& cell) {
  if (a.charge != b.charge) return false;
  for (int axis = 0; axis < 3; ++axis) {
    double delta = a.xyz[axis] - b.xyz[axis];
    delta -= std::round(delta);
    const double axisTolerance = cell[axis] > 0.0 ? tolerance / cell[axis] : tolerance;
    if (std::abs(delta) > axisTolerance) return false;
  }
  return true;
}
}

// Expand a fractional asymmetric unit using the selected space, layer, or
// line-group Seitz operators, then retain original atoms first and remove
// periodic duplicates just as genmollat.f does.
int main() {
  int count = 0;
  double tolerance = 0.0;
  if (!(std::cin >> count >> tolerance)) return 1;
  if (count < 0 || count > 2000) return 2;
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  std::string group;
  if (!std::getline(std::cin, group)) return 1;
  if (!group.empty() && group.back() == '\r') group.pop_back();

  ecce_symmetry::Point cell{{0.0, 0.0, 0.0}};
  double angles[3] = {0.0, 0.0, 0.0};
  if (!(std::cin >> cell[0] >> cell[1] >> cell[2]
        >> angles[0] >> angles[1] >> angles[2])) return 1;
  std::vector<ecce_symmetry::Atom> input(static_cast<std::size_t>(count));
  for (ecce_symmetry::Atom& atom : input)
    if (!(std::cin >> atom.tag >> atom.charge >> atom.xyz[0]
          >> atom.xyz[1] >> atom.xyz[2])) return 1;

  int systemType = -1;
  int groupIndex = -1;
  if (!ecce_symmetry::groupIndicesFor(group, systemType, groupIndex)) {
    std::cerr << "genmollat_cpp: unsupported group name: " << group << '\n';
    return 3;
  }
  std::vector<ecce_symmetry::SeitzOperation> generators;
  std::vector<ecce_symmetry::Point> centering, originOffsets;
  if (!ecce_symmetry::groupGenerators(systemType, groupIndex, 1,
                                      generators, centering, originOffsets)) return 3;
  const auto operations = ecce_symmetry::generateSeitzOperations(
      generators, centering, systemType == 0 ? groupIndex : -1);
  auto expanded = ecce_symmetry::applyOperations(
      input, operations, systemType, tolerance, cell);

  std::vector<ecce_symmetry::Atom> output;
  for (ecce_symmetry::Atom atom : input) {
    for (int axis = 0; axis < 3; ++axis) {
      atom.xyz[axis] -= std::floor(atom.xyz[axis]);
      if (std::abs(atom.xyz[axis]) < 1e-10 || std::abs(atom.xyz[axis] - 1.0) < 1e-10)
        atom.xyz[axis] = 0.0;
    }
    output.push_back(atom);
  }
  for (const ecce_symmetry::Atom& atom : expanded) {
    bool isOriginal = false;
    for (const ecce_symmetry::Atom& original : output)
      if (samePeriodic(atom, original, tolerance, cell)) { isOriginal = true; break; }
    if (!isOriginal) output.push_back(atom);
  }
  std::vector<ecce_symmetry::Atom> unique;
  for (const ecce_symmetry::Atom& atom : output) {
    bool duplicate = false;
    for (const ecce_symmetry::Atom& previous : unique)
      if (samePeriodic(atom, previous, tolerance, cell)) { duplicate = true; break; }
    if (!duplicate) unique.push_back(atom);
  }

  std::cout << std::setw(8) << unique.size() << "\n\n";
  for (const ecce_symmetry::Atom& atom : unique) {
    std::cout << std::left << std::setw(16) << atom.tag << std::right;
    for (double coordinate : atom.xyz)
      std::cout << std::uppercase << std::scientific << std::setprecision(14)
                << std::setw(24) << coordinate;
    std::cout << '\n';
  }
  (void)angles; // Symmetry operators are already expressed in fractional axes.
  (void)originOffsets; // The offsets are retained as part of the generator API.
  return 0;
}
