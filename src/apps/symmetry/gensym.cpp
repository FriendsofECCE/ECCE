#include "symmetry_cpp.h"

#include <cmath>

namespace ecce_symmetry {
namespace {
Matrix multiply(const Matrix& a, const Matrix& b) {
  Matrix result{};
  for (int row = 0; row < 3; ++row)
    for (int col = 0; col < 3; ++col)
      for (int k = 0; k < 3; ++k) result[row][col] += a[row][k] * b[k][col];
  return result;
}
bool same(const Matrix& a, const Matrix& b) {
  constexpr double tolerance = 1e-7; // gensym's historical duplicate screen
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      if (std::abs(a[i][j] - b[i][j]) > tolerance) return false;
  return true;
}
Matrix unit() { return {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}}; }
}

// Expand the compact generator list using the legacy left-multiplication
// walk. Repeated operations are screened with gensym's 1e-7 matrix tolerance.
// The identity stays first during closure and is removed from the h-1 result.
std::vector<Matrix> generateOperations(const std::vector<Matrix>& generators) {
  std::vector<Matrix> group{unit()};
  for (const Matrix& generator : generators)
    if (std::abs(std::abs(determinant(generator)) - 1.0) > 1e-7) return {};
  for (std::size_t cursor = 0; cursor < group.size() && group.size() < 192; ++cursor) {
    for (const Matrix& generator : generators) {
      const Matrix candidate = multiply(generator, group[cursor]);
      bool exists = false;
      for (const Matrix& operation : group) exists = exists || same(candidate, operation);
      if (!exists) group.push_back(candidate);
    }
  }
  if (group.size() >= 192) return {};
  group.erase(group.begin());
  return group;
}

} // namespace ecce_symmetry
