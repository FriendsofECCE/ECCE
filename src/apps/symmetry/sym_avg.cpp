#include "symmetry_cpp.h"

#include <cmath>
#include <limits>

namespace ecce_symmetry {

// Apply each point operation, map every transformed nucleus to its nearest
// input nucleus (the historical cleanersym behavior), and average each atom's
// orbit. Adding the identity produces the Reynolds projection onto the
// totally symmetric geometry.
std::vector<Point> projectToSymmetry(const std::vector<Point>& atoms,
                                     const std::vector<Matrix>& operations) {
  std::vector<Point> projected(atoms.size(), Point{0, 0, 0});
  auto accumulate = [&](const Matrix& operation) {
    std::vector<Point> moved(atoms.size());
    for (std::size_t source = 0; source < atoms.size(); ++source)
      for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col)
          moved[source][row] += operation[row][col] * atoms[source][col];

    for (std::size_t atom = 0; atom < atoms.size(); ++atom) {
      std::size_t nearest = 0;
      double best = std::numeric_limits<double>::infinity();
      for (std::size_t candidate = 0; candidate < atoms.size(); ++candidate) {
        double distance2 = 0;
        for (int axis = 0; axis < 3; ++axis) {
          const double delta = moved[candidate][axis] - atoms[atom][axis];
          distance2 += delta * delta;
        }
        if (distance2 < best) { best = distance2; nearest = candidate; }
      }
      for (int axis = 0; axis < 3; ++axis) projected[atom][axis] += moved[nearest][axis];
    }
  };

  // The old Fortran loop accumulates all nonidentity operations and then the
  // original coordinates once, which is exactly the identity contribution.
  for (const Matrix& operation : operations) accumulate(operation);
  for (std::size_t atom = 0; atom < atoms.size(); ++atom)
    for (int axis = 0; axis < 3; ++axis)
      projected[atom][axis] = (projected[atom][axis] + atoms[atom][axis]) /
                              static_cast<double>(operations.size() + 1);
  return projected;
}

} // namespace ecce_symmetry
