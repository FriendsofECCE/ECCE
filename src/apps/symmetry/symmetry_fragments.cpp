#include "symmetry_cpp.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <map>

namespace ecce_symmetry {
namespace {
bool closeCoordinate(const Point& a, const Point& b, double tolerance) {
  return std::abs(a[0] - b[0]) <= tolerance &&
         std::abs(a[1] - b[1]) <= tolerance &&
         std::abs(a[2] - b[2]) <= tolerance;
}

Point transformPoint(const Matrix& matrix, const Point& p) {
  Point result{{0.0, 0.0, 0.0}};
  for (int row = 0; row < 3; ++row)
    for (int col = 0; col < 3; ++col)
      result[row] += matrix[row][col] * p[col];
  return result;
}

void wrap(Point& p, int systemType) {
  const int dimensions = systemType == 3 ? 3 : systemType == 2 ? 2 : 0;
  for (int axis = 0; axis < dimensions; ++axis) {
    p[axis] -= std::floor(p[axis]);
    if (std::abs(p[axis]) < 1e-10 || std::abs(p[axis] - 1.0) < 1e-10)
      p[axis] = 0.0;
  }
  if (systemType == 1) {
    p[2] -= std::floor(p[2]);
    if (std::abs(p[2]) < 1e-10 || std::abs(p[2] - 1.0) < 1e-10) p[2] = 0.0;
  }
}

bool samePeriodic(const Point& a, const Point& b, double tolerance,
                  int systemType, const Point& cellLengths) {
  const int dimensions = systemType == 3 ? 3 : systemType == 2 ? 2 : 0;
  for (int axis = 0; axis < dimensions; ++axis) {
    double delta = a[axis] - b[axis];
    delta -= std::round(delta);
    const double axisTolerance = cellLengths[axis] > 0.0
        ? tolerance / cellLengths[axis] : tolerance;
    if (std::abs(delta) > axisTolerance) return false;
  }
  const double zTolerance = cellLengths[2] > 0.0
      ? tolerance / cellLengths[2] : tolerance;
  if (systemType == 2) return std::abs(a[2] - b[2]) <= zTolerance;
  const double xTolerance = cellLengths[0] > 0.0 ? tolerance / cellLengths[0] : tolerance;
  const double yTolerance = cellLengths[1] > 0.0 ? tolerance / cellLengths[1] : tolerance;
  if (systemType == 1)
    return std::abs(a[0] - b[0]) <= xTolerance &&
           std::abs(a[1] - b[1]) <= yTolerance &&
           std::abs((a[2] - b[2]) - std::round(a[2] - b[2])) <= zTolerance;
  return true;
}
} // namespace

bool molecularGroupOperations(const std::string& requested,
                             std::vector<Matrix>& operations) {
  std::string group = requested;
  std::transform(group.begin(), group.end(), group.begin(),
                 [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  std::vector<Matrix> generators;
  if (!generatorSetFor(group, generators)) return false;
  operations = generateOperations(generators);
  // gensym emits an established order that is different from a generic
  // breadth-first closure for several non-cyclic groups. Keep that order for
  // the migrated atom generators, whose serialized atom order is observable.
  static const std::map<std::string, std::vector<int>> legacyOrder = {
    {"C4V", {2,0,5,1,6,3,4}},
    {"C5H", {5,1,7,3,4,0,6,2,8}},
    {"D2D", {2,0,5,1,6,3,4}},
    {"D3D", {9,4,0,6,7,8,1,5,10,3,2}},
    {"D4H", {3,0,8,1,9,4,6,2,10,5,13,7,14,11,12}},
    {"D5H", {17,2,12,9,13,0,16,5,8,1,15,6,7,14,18,3,11,10,4}},
    {"D6H", {19,7,0,21,10,1,12,13,3,16,17,14,2,11,18,4,15,20,6,5,22,9,8}},
    {"OH", {32,10,13,16,43,45,11,4,34,28,31,0,36,17,7,23,35,38,6,8,41,20,39,9,42,37,40,1,24,18,21,25,22,5,19,12,46,44,33,3,15,27,30,26,14,2,29}},
    {"S4", {1,0,2}},
    {"TD", {19,13,11,0,15,18,12,2,16,8,17,22,1,6,14,21,7,3,9,20,4,10,5}},
    {"TH", {0,21,22,13,14,15,16,4,6,7,8,9,10,11,12,1,2,3,5,17,18,19,20}}
  };
  const auto ordering = legacyOrder.find(group);
  if (ordering != legacyOrder.end() && ordering->second.size() == operations.size()) {
    std::vector<Matrix> compatible;
    compatible.reserve(operations.size());
    for (int index : ordering->second) compatible.push_back(operations.at(index));
    operations.swap(compatible);
  }
  return !operations.empty() || group == "C1";
}

// Greedily select the atom nearest the origin, then repeatedly choose the
// unaccounted atom nearest the current fragment. Each selected atom's orbit
// marks equivalent atoms of the same tag as accounted for (the legacy
// findfrag.f rule).
std::vector<Atom> findIrreducible(const std::vector<Atom>& atoms,
                                  const std::vector<Matrix>& operations,
                                  double tolerance,
                                  std::vector<int>* originalOrder) {
  std::vector<Atom> result;
  if (originalOrder) originalOrder->clear();
  if (atoms.empty()) return result;
  std::vector<bool> used(atoms.size(), false);
  std::size_t chosen = 0;
  double best = std::numeric_limits<double>::infinity();
  for (std::size_t i = 0; i < atoms.size(); ++i) {
    const Point& p = atoms[i].xyz;
    const double r = p[0]*p[0] + p[1]*p[1] + p[2]*p[2];
    if (r < best) { chosen = i; best = r; }
  }
  for (;;) {
    if (!used[chosen]) {
      used[chosen] = true;
      result.push_back(atoms[chosen]);
      if (originalOrder) originalOrder->push_back(static_cast<int>(chosen + 1));
    }
    std::vector<Point> orbit;
    for (const Matrix& op : operations) orbit.push_back(transformPoint(op, atoms[chosen].xyz));
    for (std::size_t i = 0; i < atoms.size(); ++i) {
      if (used[i] || atoms[i].tag != atoms[chosen].tag) continue;
      for (const Point& p : orbit) {
        if (closeCoordinate(atoms[i].xyz, p, tolerance)) { used[i] = true; break; }
      }
    }
    std::size_t next = atoms.size();
    best = std::numeric_limits<double>::infinity();
    for (std::size_t j = 0; j < result.size(); ++j) {
      for (std::size_t i = 0; i < atoms.size(); ++i) {
        if (used[i]) continue;
        double d2 = 0.0;
        for (int axis = 0; axis < 3; ++axis) {
          const double d = atoms[i].xyz[axis] - result[j].xyz[axis];
          d2 += d*d;
        }
        if (d2 < best) { best = d2; next = i; }
      }
    }
    if (next == atoms.size()) break;
    chosen = next;
  }
  // Legacy behavior preserves the original order if no reduction occurred.
  if (result.size() == atoms.size()) {
    result = atoms;
  }
  return result;
}

// Apply each non-identity operation to each input atom. The identity image is
// retained first; later images are filtered against earlier coordinates. For
// molecular inputs, already-present atoms are kept in their supplied order.
std::vector<Atom> applyOperations(const std::vector<Atom>& input,
                                  const std::vector<Matrix>& operations,
                                  int systemType, double tolerance,
                                  const Point& cellLengths,
                                  std::vector<int>* originalOrder) {
  std::vector<Atom> atoms = input;
  for (Atom& atom : atoms) wrap(atom.xyz, systemType);
  std::vector<Atom> result;
  std::vector<int> order;
  for (std::size_t i = 0; i < atoms.size(); ++i) {
    if (systemType == 0) {
      bool duplicate = false;
      for (const Atom& old : result)
        if (closeCoordinate(atoms[i].xyz, old.xyz, tolerance)) { duplicate = true; break; }
      if (duplicate) continue;
    }
    order.push_back(static_cast<int>(result.size() + 1));
    result.push_back(atoms[i]);
    std::vector<Point> images;
    for (const Matrix& op : operations) {
      Point p = transformPoint(op, atoms[i].xyz);
      wrap(p, systemType);
      bool duplicate = closeCoordinate(p, atoms[i].xyz, tolerance);
      for (const Point& previous : images)
        duplicate = duplicate || (systemType == 0
            ? closeCoordinate(p, previous, tolerance)
            : samePeriodic(p, previous, tolerance, systemType, cellLengths));
      if (!duplicate) images.push_back(p);
    }
    for (const Point& p : images) {
      Atom generated = atoms[i];
      generated.xyz = p;
      result.push_back(generated);
    }
  }
  // Map the asymmetric input atoms back to the output positions. This is used
  // by the builder to retain the original fragment selection.
  if (originalOrder) {
    originalOrder->clear();
    for (const Atom& atom : input) {
      int found = 0;
      for (std::size_t i = 0; i < result.size(); ++i)
        if (closeCoordinate(atom.xyz, result[i].xyz, tolerance)) {
          found = static_cast<int>(i + 1); break;
        }
      originalOrder->push_back(found);
    }
  }
  return result;
}

// Periodic counterpart applies the translation column of each Seitz operator
// in addition to its rotation matrix; the resulting fractional coordinates
// are wrapped into the unit cell before duplicate screening.
std::vector<Atom> applyOperations(const std::vector<Atom>& input,
                                  const std::vector<SeitzOperation>& operations,
                                  int systemType, double tolerance,
                                  const Point& cellLengths,
                                  std::vector<int>* originalOrder) {
  std::vector<Atom> atoms = input;
  for (Atom& atom : atoms) wrap(atom.xyz, systemType);
  std::vector<Atom> result;
  for (const Atom& atom : atoms) {
    result.push_back(atom);
    std::vector<Point> images;
    for (const SeitzOperation& operation : operations) {
      Point image = transformPoint(operation.rotation, atom.xyz);
      for (int axis = 0; axis < 3; ++axis) image[axis] += operation.translation[axis];
      // dosymops normalizes line-group input coordinates but historically
      // leaves generated z coordinates unwrapped; genmollat's final duplicate
      // scan handles periodic z differences separately.
      if (systemType != 1) wrap(image, systemType);
      bool duplicate = closeCoordinate(image, atom.xyz, tolerance);
      for (const Point& old : images)
        duplicate = duplicate || closeCoordinate(image, old, tolerance);
      if (!duplicate) images.push_back(image);
    }
    for (const Point& image : images) {
      Atom generated = atom;
      generated.xyz = image;
      result.push_back(generated);
    }
  }
  if (originalOrder) {
    originalOrder->clear();
    for (const Atom& atom : input) {
      int found = 0;
      for (std::size_t i = 0; i < result.size(); ++i)
        if (samePeriodic(atom.xyz, result[i].xyz, tolerance, systemType, cellLengths)) {
          found = static_cast<int>(i + 1);
          break;
        }
      originalOrder->push_back(found);
    }
  }
  return result;
}
} // namespace ecce_symmetry
