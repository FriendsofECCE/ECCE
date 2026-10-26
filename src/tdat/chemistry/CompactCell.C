#include <algorithm>
#include <cmath>

#include "tdat/CompactCell.H"

using std::vector;

namespace {

typedef double V3[3];

// Minimum image of a difference d in the lattice of rows a, b, c: reduce
// along c, b, a, then try the neighbours (the cell is reduced, not orthogonal).
void minImage(V3 d, const V3 a[3])
{
  for (int k = 2; k >= 0; k--) {
    double n = std::floor(d[k] / a[k][k] + 0.5);
    for (int i = 0; i <= k; i++) d[i] -= n * a[k][i];
  }
  double best = d[0]*d[0] + d[1]*d[1] + d[2]*d[2];
  V3 bd = { d[0], d[1], d[2] };
  for (int i = -1; i <= 1; i++)
    for (int j = -1; j <= 1; j++)
      for (int k = -1; k <= 1; k++) {
        V3 t;
        for (int m = 0; m < 3; m++)
          t[m] = d[m] + i * a[0][m] + j * a[1][m] + k * a[2][m];
        double l = t[0]*t[0] + t[1]*t[1] + t[2]*t[2];
        if (l < best - 1e-12) {
          best = l;
          for (int m = 0; m < 3; m++) bd[m] = t[m];
        }
      }
  for (int m = 0; m < 3; m++) d[m] = bd[m];
}

}  // namespace

bool CompactCell::wrap(std::vector<std::array<double, 3> >& x,
                       const std::vector<int>& residueOf,
                       const std::vector<char>& heavy,
                       const double cell0[3][3], double bondLength)
{
  const int n = (int)x.size();
  if (n == 0 || (int)residueOf.size() != n || (int)heavy.size() != n)
    return false;
  if (cell0[0][1] != 0 || cell0[0][2] != 0 || cell0[1][2] != 0) return false;
  if (cell0[0][0] <= 0 || cell0[1][1] <= 0 || cell0[2][2] <= 0) return false;
  // A rectangular box is wrapped too: molecules broken over its faces would
  // otherwise be bonded across the cell.
  const vector<std::array<double, 3> > original(x);
  V3 cell[3];
  for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++) cell[i][k] = cell0[i][k];
  const V3 centre = { 0.5 * (cell[0][0] + cell[1][0] + cell[2][0]),
                      0.5 * (cell[1][1] + cell[2][1]), 0.5 * cell[2][2] };

  vector<int> rstart;
  for (int i = 0; i < n; i++)
    if (i == 0 || residueOf[i] != residueOf[i - 1]) rstart.push_back(i);
  rstart.push_back(n);
  const int nres = (int)rstart.size() - 1;
  const double bond2 = bondLength * bondLength;

  // Whole residues: each atom at the image nearest the one before it.
  for (int i = 1; i < n; i++) {
    if (residueOf[i] != residueOf[i - 1]) continue;
    V3 d = { x[i][0] - x[i-1][0], x[i][1] - x[i-1][1], x[i][2] - x[i-1][2] };
    V3 o = { d[0], d[1], d[2] };
    minImage(d, cell);
    for (int k = 0; k < 3; k++) x[i][k] += d[k] - o[k];
  }

  // Molecules: runs of residues joined by a heavy-atom bond.
  vector<int> mol(nres, 0);
  for (int r = 1; r < nres; r++) {
    bool bonded = false;
    V3 shift = { 0, 0, 0 };
    for (int i = rstart[r - 1]; i < rstart[r] && !bonded; i++) {
      if (!heavy[i]) continue;
      for (int j = rstart[r]; j < rstart[r + 1]; j++) {
        if (!heavy[j]) continue;
        V3 d = { x[j][0] - x[i][0], x[j][1] - x[i][1], x[j][2] - x[i][2] };
        V3 o = { d[0], d[1], d[2] };
        minImage(d, cell);
        if (d[0]*d[0] + d[1]*d[1] + d[2]*d[2] < bond2) {
          bonded = true;
          for (int k = 0; k < 3; k++) shift[k] = d[k] - o[k];
          break;
        }
      }
    }
    mol[r] = bonded ? mol[r - 1] : mol[r - 1] + 1;
    if (bonded)
      for (int j = rstart[r]; j < rstart[r + 1]; j++)
        for (int k = 0; k < 3; k++) x[j][k] += shift[k];
  }

  // Each molecule's centre to the image nearest the box centre.
  int r = 0;
  while (r < nres) {
    int r2 = r;
    while (r2 < nres && mol[r2] == mol[r]) r2++;
    const int a0 = rstart[r], a1 = rstart[r2];
    V3 c = { 0, 0, 0 };
    for (int i = a0; i < a1; i++)
      for (int k = 0; k < 3; k++) c[k] += x[i][k] / (a1 - a0);
    V3 d = { c[0] - centre[0], c[1] - centre[1], c[2] - centre[2] };
    V3 o = { d[0], d[1], d[2] };
    minImage(d, cell);
    for (int i = a0; i < a1; i++)
      for (int k = 0; k < 3; k++) x[i][k] += d[k] - o[k];
    // A molecule as long as the cell is periodic (a sheet, a chain); making
    // it whole would only smear it, so it stays as it was.
    bool endless = false;
    for (int k = 0; k < 3 && !endless; k++) {
      double lo = x[a0][k], hi = x[a0][k];
      for (int i = a0; i < a1; i++) {
        lo = std::min(lo, x[i][k]);
        hi = std::max(hi, x[i][k]);
      }
      endless = hi - lo >= 0.9 * cell[k][k];
    }
    if (endless)
      for (int i = a0; i < a1; i++) x[i] = original[i];
    r = r2;
  }
  return true;
}
