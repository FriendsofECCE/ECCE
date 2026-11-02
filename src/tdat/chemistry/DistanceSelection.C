#include <cmath>
#include <stdexcept>
#include "tdat/DistanceSelection.H"
#include "tdat/Fragment.H"
#include "tdat/LatticeDef.H"
#include "tdat/TBond.H"

std::vector<int> selectWithinDistance(const Fragment& frag,
    const std::vector<int>& seeds, double radius, bool molecules, bool keep)
{
  if (!std::isfinite(radius) || radius < 0.1 || radius > 50.0)
    throw std::invalid_argument("Distance must be between 0.1 and 50 Angstrom.");
  const int n = frag.numAtoms();
  std::vector<bool> original(n, false), selected(n, false);
  for (int s : seeds) {
    if (s < 0 || s >= n) throw std::invalid_argument("Invalid selected atom.");
    original[s] = selected[s] = true;
  }
  MPoint basis[3], inv[3];
  bool periodic = false;
  if (frag.getLattice()) {
    std::vector<MPoint> *v = frag.getLattice()->toVectors();
    for (int k = 0; k < 3; ++k) basis[k] = (*v)[k];
    delete v;
    double volume = basis[0].crossProduct1(basis[1]).dotProduct(basis[2]);
    if (std::fabs(volume) > 1.e-8) {
      inv[0] = basis[1].crossProduct1(basis[2]);
      inv[1] = basis[2].crossProduct1(basis[0]);
      inv[2] = basis[0].crossProduct1(basis[1]);
      for (int k = 0; k < 3; ++k) inv[k].scale(1.0 / volume);
      periodic = true;
    }
  }
  auto within = [&](MPoint d) {
    if (!periodic) return d.lengthSqr() <= radius * radius;
    // Rounding fractional coordinates alone misses neighbours in skew cells.
    // Reciprocal-vector bounds include every image that can be within R.
    MPoint rounded = d;
    for (int k = 0; k < 3; ++k) {
      MPoint shift = basis[k];
      shift.scale(std::floor(d.dotProduct(inv[k]) + 0.5));
      rounded.subtract(shift);
    }
    if (rounded.lengthSqr() <= radius * radius) return true;
    double f[3];
    int lo[3], hi[3];
    for (int k = 0; k < 3; ++k) {
      f[k] = d.dotProduct(inv[k]);
      double bound = radius * inv[k].length();
      lo[k] = static_cast<int>(std::ceil(f[k] - bound - 1.e-10));
      hi[k] = static_cast<int>(std::floor(f[k] + bound + 1.e-10));
    }
    for (int a = lo[0]; a <= hi[0]; ++a)
      for (int b = lo[1]; b <= hi[1]; ++b)
        for (int c = lo[2]; c <= hi[2]; ++c) {
          MPoint image = d;
          const int shifts[] = {a, b, c};
          for (int k = 0; k < 3; ++k) {
            MPoint shift = basis[k];
            shift.scale(shifts[k]);
            image.subtract(shift);
          }
          if (image.lengthSqr() <= radius * radius) return true;
        }
    return false;
  };
  for (int i = 0; i < n; ++i) {
    if (original[i]) continue;
    for (int s : seeds) {
      MPoint d, p;
      d.xyz(frag.atomRef(i)->coordinates());
      p.xyz(frag.atomRef(s)->coordinates());
      d.subtract(p);
      if (within(d)) { selected[i] = true; break; }
    }
  }
  if (molecules) {
    // Follow the same bonded connectivity as Select Molecule, without wx.
    std::vector<int> stack;
    for (int i = 0; i < n; ++i)
      if (selected[i]) stack.push_back(i);
    while (!stack.empty()) {
      TAtm *atom = frag.atomRef(stack.back());
      stack.pop_back();
      for (TBond *bond : atom->bondList()) {
        TAtm *next = bond->atom1() == atom ? bond->atom2() : bond->atom1();
        int j = next->index();
        if (!selected[j]) { selected[j] = true; stack.push_back(j); }
      }
    }
  }
  std::vector<int> result;
  for (int i = 0; i < n; ++i)
    if (original[i] ? keep : selected[i]) result.push_back(i);
  return result;
}
