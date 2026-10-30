#include "symmetry_cpp.h"

#include <array>
#include <cmath>

#include "spgen_tables.inc"

namespace ecce_symmetry {
namespace {

int recipeField(int systemType, int groupIndex, int field) {
  if (field < 0 || field >= 40 || groupIndex < 1) return 0;
  if (systemType == 3 && groupIndex <= 230)
    return spaceGroupRecipe[static_cast<std::size_t>(groupIndex - 1 + field * 230)];
  if (systemType == 2 && groupIndex <= 80)
    return layerGroupRecipe[static_cast<std::size_t>(groupIndex - 1 + field * 80)];
  if (systemType == 1 && groupIndex <= 99)
    return lineGroupRecipe[static_cast<std::size_t>(groupIndex - 1 + field * 99)];
  if (systemType == 0 && groupIndex <= 46)
    return molecularGroupRecipe[static_cast<std::size_t>(groupIndex - 1 + field * 46)];
  return 0;
}

Point translationVector(int vectorIndex) {
  Point result{{0.0, 0.0, 0.0}};
  if (vectorIndex < 1 || vectorIndex > 46) return result;
  for (int axis = 0; axis < 3; ++axis)
    result[axis] = translations[static_cast<std::size_t>(vectorIndex - 1 + axis * 46)];
  return result;
}

SeitzOperation multiply(const SeitzOperation& left, const SeitzOperation& right) {
  SeitzOperation result;
  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 3; ++col)
      for (int k = 0; k < 3; ++k)
        result.rotation[row][col] += left.rotation[row][k] * right.rotation[k][col];
    for (int k = 0; k < 3; ++k)
      result.translation[row] += left.rotation[row][k] * right.translation[k];
    result.translation[row] += left.translation[row];
    double& translation = result.translation[row];
    if (translation >= -1e-7 && translation <= 1e-7) translation = 0.0;
    else if (translation >= 1.0 - 1e-7 && translation <= 1.0 + 1e-7) translation = 1.0;
    if (translation < 0.0) translation += 1.0;
    else if (translation >= 1.0) translation -= 1.0;
    if (translation >= 0.3333 && translation <= 0.3334) translation = 1.0 / 3.0;
    else if (translation >= 0.6666 && translation <= 0.6667) translation = 2.0 / 3.0;
  }
  return result;
}

bool same(const SeitzOperation& a, const SeitzOperation& b) {
  const double tolerance = 1e-7;
  for (int row = 0; row < 3; ++row) {
    if (std::abs(a.translation[row] - b.translation[row]) > tolerance) return false;
    for (int col = 0; col < 3; ++col)
      if (std::abs(a.rotation[row][col] - b.rotation[row][col]) > tolerance) return false;
  }
  return true;
}

bool identityRotation(const SeitzOperation& op) {
  const double trace = op.rotation[0][0] + op.rotation[1][1] + op.rotation[2][2];
  return trace >= 3.0 - 1e-7 && trace <= 3.0 + 1e-7;
}
} // namespace

// Resolve one of spgen's packed generator recipes into Seitz operators.
// The packed arrays retain their original Fortran column-major layout to make
// each pointer calculation correspond directly to the old table definition.
bool groupGenerators(int systemType, int groupIndex, int setting,
                     std::vector<SeitzOperation>& generators,
                     std::vector<Point>& centering,
                     std::vector<Point>& originOffsets) {
  generators.clear();
  centering.clear();
  originOffsets.clear();
  const int groupCount = systemType == 3 ? 230 : systemType == 2 ? 80 :
                         systemType == 1 ? 99 : systemType == 0 ? 46 : 0;
  if (groupIndex < 1 || groupIndex > groupCount) return false;

  std::array<int, 40> recipe{};
  for (int field = 0; field < 40; ++field)
    recipe[field] = recipeField(systemType, groupIndex, field);
  const bool alternateSetting = setting > 1;
  const int centerCount = recipe[alternateSetting ? 21 : 1];
  const int centerPointerStart = alternateSetting ? 22 : 2;
  const int generatorStart = alternateSetting ? 25 : 6;
  for (int i = 0; i < centerCount; ++i)
    centering.push_back(translationVector(recipe[centerPointerStart + i]));

  int count = recipe[5];
  const bool icosahedralSpecial = systemType == 0 && recipe[0] == 3;
  if (icosahedralSpecial) ++count;
  if (count < 0 || count > 6) return false;
  for (int i = 0; i < count; ++i) {
    const int field = icosahedralSpecial && i == 5 ? 25 : generatorStart + 3 * i;
    const int translationIndex = recipe[field];
    const int rotationIndex = recipe[field + 1];
    const int originIndex = recipe[field + 2];
    if (rotationIndex < 1 || rotationIndex > 30) return false;

    SeitzOperation operation;
    for (int row = 0; row < 3; ++row)
      for (int col = 0; col < 3; ++col) {
        const std::size_t index = static_cast<std::size_t>(row + 3 * col + 12 * (rotationIndex - 1));
        operation.rotation[row][col] = rotoinversions[index];
      }
    const Point translation = translationVector(translationIndex);
    const Point origin = translationVector(originIndex);
    if (originIndex > 1) {
      for (int row = 0; row < 3; ++row) {
        double rotatedOrigin = 0.0;
        for (int col = 0; col < 3; ++col)
          rotatedOrigin += operation.rotation[row][col] * origin[col];
        operation.translation[row] = (translationIndex ? translation[row] : 0.0)
                                   - rotatedOrigin + origin[row];
      }
    } else if (translationIndex != 0) {
      operation.translation = translation;
    }
    for (double& value : operation.translation) {
      if (value < 0.0) value += 1.0;
      else if (value > 1.0) value -= 1.0;
      else if (value >= 0.3333 && value <= 0.3334) value = 1.0 / 3.0;
      else if (value >= 0.6666 && value <= 0.6667) value = 2.0 / 3.0;
    }
    generators.push_back(operation);
    originOffsets.push_back(originIndex > 1 ? origin : Point{{0.0, 0.0, 0.0}});
  }
  return true;
}

// Ordered port of gensym's generator walk, including its historical rotation
// trace identity test and the subsequent centering-coset expansion.
std::vector<SeitzOperation> generateSeitzOperations(
    const std::vector<SeitzOperation>& generators,
    const std::vector<Point>& centering, int molecularGroupIndex) {
  std::vector<SeitzOperation> operations;
  for (std::size_t g = 0; g < generators.size(); ++g) {
    operations.push_back(generators[g]);
    const std::size_t finalMultiplier = operations.size() - 1;
    SeitzOperation left = generators[g];
    bool squared = false;
    bool generatorDone = false;
    while (!generatorDone) {
      bool nextGenerator = false;
      for (std::size_t m = 0; m <= finalMultiplier; ++m) {
        const SeitzOperation candidate = multiply(left, operations[m]);
        bool duplicate = false;
        for (const SeitzOperation& old : operations)
          if (same(candidate, old)) { duplicate = true; break; }
        if (duplicate || identityRotation(candidate)) {
          if (m != finalMultiplier) continue;
          nextGenerator = true;
          break;
        }
        operations.push_back(candidate);
        if (operations.size() >= 192) return {};
        if (m != finalMultiplier) continue;
        if (g == 0 && g + 1 < generators.size()) {
          nextGenerator = true;
        } else if (generators.size() == 1 ||
                   (g + 1 == generators.size() && squared)) {
          generatorDone = true;
        } else if (squared) {
          nextGenerator = true;
        } else {
          left = operations.back();
          squared = true;
        }
        break;
      }
      if (nextGenerator || generatorDone) break;
    }
  }

  const std::vector<SeitzOperation> primitive = operations;
  for (const Point& center : centering) {
    SeitzOperation translatedIdentity;
    for (int i = 0; i < 3; ++i) translatedIdentity.rotation[i][i] = 1.0;
    translatedIdentity.translation = center;
    operations.push_back(translatedIdentity);
    for (const SeitzOperation& op : primitive) {
      SeitzOperation translated = op;
      for (int i = 0; i < 3; ++i) translated.translation[i] += center[i];
      operations.push_back(translated);
    }
  }
  if (molecularGroupIndex == 1) operations.clear();
  return operations;
}
} // namespace ecce_symmetry
