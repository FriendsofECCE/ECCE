// Shared types and entry points for the C++ cleansym migration.
#ifndef ECCE_SYMMETRY_CPP_H
#define ECCE_SYMMETRY_CPP_H

#include <array>
#include <string>
#include <vector>

namespace ecce_symmetry {
using Matrix = std::array<std::array<double, 3>, 3>;
using Point = std::array<double, 3>;

struct Atom {
  std::string tag;
  double charge = 0.0;
  Point xyz{{0.0, 0.0, 0.0}};
  double mass = 1.0;
};

struct SeitzOperation {
  Matrix rotation{};
  Point translation{{0.0, 0.0, 0.0}};
};

const std::vector<std::string>& molecularGroups();
bool generatorSetFor(const std::string& group, std::vector<Matrix>& generators);
std::vector<Matrix> generateOperations(const std::vector<Matrix>& generators);
std::vector<Point> projectToSymmetry(const std::vector<Point>& atoms,
                                     const std::vector<Matrix>& operations);
double determinant(const Matrix& matrix);
int errorExit(const std::string& message, int status);

// C++ ports of findfrag.f and dosymops.f.  Operations use the same order as
// gensym: the identity is implicit and omitted from this vector.
std::vector<Atom> findIrreducible(const std::vector<Atom>& atoms,
                                  const std::vector<Matrix>& operations,
                                  double tolerance,
                                  std::vector<int>* originalOrder = nullptr);
std::vector<Atom> applyOperations(const std::vector<Atom>& atoms,
                                  const std::vector<Matrix>& operations,
                                  int systemType, double tolerance,
                                  const Point& cellLengths,
                                  std::vector<int>* originalOrder = nullptr);
std::vector<Atom> applyOperations(const std::vector<Atom>& atoms,
                                  const std::vector<SeitzOperation>& operations,
                                  int systemType, double tolerance,
                                  const Point& cellLengths,
                                  std::vector<int>* originalOrder = nullptr);
bool molecularGroupOperations(const std::string& group,
                             std::vector<Matrix>& operations);

// Full spnames/spgen table interface. systemType matches the historic codes:
// 0 molecular, 1 line, 2 layer, 3 three-dimensional space group.
bool groupIndicesFor(const std::string& group, int& systemType, int& groupIndex);
std::string canonicalGroupName(int systemType, int groupIndex);
bool groupGenerators(int systemType, int groupIndex, int setting,
                     std::vector<SeitzOperation>& generators,
                     std::vector<Point>& centering,
                     std::vector<Point>& originOffsets);
std::vector<SeitzOperation> generateSeitzOperations(
    const std::vector<SeitzOperation>& generators,
    const std::vector<Point>& centering, int molecularGroupIndex = -1);
}

#endif
