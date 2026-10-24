#ifndef MOVINGAVERAGE_HH
#define MOVINGAVERAGE_HH

// The smoothing curve of an MD plot, without wx so that a test can run it.

#include <algorithm>
#include <cmath>
#include <vector>

namespace MovingAverage {

// Points in the window: 10 % of the series, never fewer than 2.
inline size_t windowPoints(size_t n)
{
  return std::max<size_t>(2, (size_t)std::floor(0.1 * (double)n + 0.5));
}

// The mean of the `w` points centred on each i (cut short at the two ends).
// A cumulative mean from the first point keeps the start-up transient for
// ever and sits below a series that rises and then levels off; this one
// follows it.
inline std::vector<double> centred(const std::vector<double>& y, size_t w)
{
  const size_t n = y.size();
  std::vector<double> r(n);
  if (n == 0) return r;
  w = std::max<size_t>(1, std::min(w, n));
  std::vector<double> cum(n + 1, 0.0);
  for (size_t i = 0; i < n; i++) cum[i + 1] = cum[i] + y[i];
  const size_t before = w / 2, after = w - before;   // [i-before, i+after)
  for (size_t i = 0; i < n; i++) {
    const size_t lo = i >= before ? i - before : 0;
    const size_t hi = std::min(n, i + after);
    r[i] = (cum[hi] - cum[lo]) / (double)(hi - lo);
  }
  return r;
}

}

#endif
