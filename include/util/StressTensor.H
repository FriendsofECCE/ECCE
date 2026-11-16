#ifndef ECCE_STRESS_TENSOR_H
#define ECCE_STRESS_TENSOR_H

#include <algorithm>
#include <cmath>
#include <string>
#include "util/Diag3.H"

// Display quantities are tension-positive GPa; stored QE/MLP values are
// compression-positive kbar. Symmetrising removes output rounding noise.
namespace StressTensor {
struct Result {
  double sigma[3][3];
  double principal[3];
  double axis[3][3]; // axis[i] belongs to principal[i].
  double pressure, vonMises, maxShear;
};

inline bool analyse(const double stored[9], Result& r)
{
  for (int i = 0; i < 9; ++i)
    if (!std::isfinite(stored[i])) return false;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      r.sigma[i][j] = -0.05 * (stored[3*i+j] + stored[3*j+i]);
  double w[3], v[3][3];
  Diag3(r.sigma[0][0], r.sigma[0][1], r.sigma[0][2],
        r.sigma[1][1], r.sigma[1][2], r.sigma[2][2])
    .calculate(w, v[0], v[1], v[2]);
  int order[] = {0, 1, 2};
  std::sort(order, order+3, [&](int a, int b) { return w[a] > w[b]; });
  for (int i = 0; i < 3; ++i) {
    r.principal[i] = w[order[i]];
    for (int j = 0; j < 3; ++j) r.axis[i][j] = v[order[i]][j];
  }
  r.pressure = -(r.sigma[0][0]+r.sigma[1][1]+r.sigma[2][2])/3;
  double d01=w[0]-w[1], d12=w[1]-w[2], d20=w[2]-w[0];
  r.vonMises = std::sqrt((d01*d01+d12*d12+d20*d20)/2);
  r.maxShear = (r.principal[0]-r.principal[2])/2;
  return true;
}

// Normal stress along a cell vector, including a nonorthogonal cell.
inline double normal(const Result& r, const double n[3])
{
  double norm=0, value=0;
  for (int i=0; i<3; ++i) {
    norm += n[i]*n[i];
    for (int j=0; j<3; ++j) value += n[i]*r.sigma[i][j]*n[j];
  }
  return norm > 1e-20 ? value/norm : 0;
}

inline std::string cellWords(const Result& r, const double cell[3][3])
{
  std::string shrink, expand, neutral;
  for (int i=0; i<3; ++i) {
    double s = normal(r, cell[i]);
    std::string& group = s > 1e-6 ? shrink : s < -1e-6 ? expand : neutral;
    if (!group.empty()) group += " and ";
    group += char('a'+i);
  }
  std::string text = "The cell wants to ";
  if (!shrink.empty()) text += "shrink along " + shrink;
  if (!expand.empty()) text += (shrink.empty() ? "" : ", ") +
                               std::string("expand along ") + expand;
  if (shrink.empty() && expand.empty()) text = "No resolved normal stress";
  if (!neutral.empty()) text += "; near zero along " + neutral;
  return text + ".";
}
}
#endif
