#ifndef MOFOLLOW__H
#define MOFOLLOW__H

#include <cmath>
#include <vector>

// Which orbital of a new IRC point is the one that was shown on the previous
// point.  No wx here, so the match is testable.
namespace mofollow {

// The row of <rows> (nbf coefficients each) with the largest |prev^T S row|,
// S the nbf x nbf AO overlap (row-major) at the new geometry; -1 if the sizes
// disagree.  A MO is followed by its character, so the index may change.
inline int best(const std::vector<double>& prev,
                const std::vector< std::vector<double> >& rows,
                const std::vector<double>& S, double *overlap = 0)
{
  const size_t n = prev.size();
  if (n == 0 || S.size() != n * n) return -1;
  std::vector<double> sp(n, 0.0);          // S prev
  for (size_t i = 0; i < n; i++) {
    double v = 0.0;
    for (size_t j = 0; j < n; j++) v += S[i * n + j] * prev[j];
    sp[i] = v;
  }
  int pick = -1;
  double top = -1.0;
  for (size_t k = 0; k < rows.size(); k++) {
    if (rows[k].size() != n) continue;
    double o = 0.0;
    for (size_t i = 0; i < n; i++) o += rows[k][i] * sp[i];
    if (std::fabs(o) > top) { top = std::fabs(o); pick = (int)k; }
  }
  if (overlap) *overlap = top;
  return pick;
}

}

#endif
