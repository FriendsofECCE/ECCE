#include "symmetry_cpp.h"

namespace ecce_symmetry {

// Keep the tiny determinant helper from deter3.f available to the C++ port.
double determinant(const Matrix& m) {
  return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
         m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
         m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

} // namespace ecce_symmetry
