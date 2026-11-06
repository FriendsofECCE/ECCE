#ifndef VIBFREQUENCY_H
#define VIBFREQUENCY_H

#include <cmath>

namespace VibFrequency {

// Numerical translations and rotations must not flag a stationary point.
// Keep the Python export threshold in agreement with this value (cm-1).
constexpr double IMAG_THRESHOLD = 1.0;

inline bool isImaginary(double wavenumber)
{
  return wavenumber < -IMAG_THRESHOLD;
}

inline bool isZero(double wavenumber)
{
  return std::fabs(wavenumber) < IMAG_THRESHOLD;
}

inline bool isReal(double wavenumber)
{
  return wavenumber >= IMAG_THRESHOLD;
}

} // namespace VibFrequency

#endif
