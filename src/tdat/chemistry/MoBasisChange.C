///////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: MoBasisChange.C
//
// See include/tdat/MoBasisChange.H.
///////////////////////////////////////////////////////////////////////////

#include "tdat/MoBasisChange.H"

#include <cmath>

#include "tdat/PropTable.H"

namespace {

bool sameFunction(const EspBasisFunction& a, const EspBasisFunction& b)
{
  if (a.exponent.size() != b.exponent.size()) return false;
  for (size_t p = 0; p < a.exponent.size(); p++) {
    if (fabs(a.exponent[p] - b.exponent[p]) > 1e-10 * fabs(a.exponent[p]))
      return false;
    if (fabs(a.contraction[p] - b.contraction[p]) >
        1e-10 * (fabs(a.contraction[p]) + 1e-300))
      return false;
  }
  return true;
}

// In-place Cholesky solve of S X = B (S n x n, B n x m, row major).
bool choleskySolve(vector<double>& S, vector<double>& B, size_t n, size_t m)
{
  for (size_t j = 0; j < n; j++) {
    double d = S[j*n+j];
    for (size_t k = 0; k < j; k++) d -= S[j*n+k]*S[j*n+k];
    if (d <= 1e-14) return false;
    d = sqrt(d);
    S[j*n+j] = d;
    for (size_t i = j+1; i < n; i++) {
      double v = S[i*n+j];
      for (size_t k = 0; k < j; k++) v -= S[i*n+k]*S[j*n+k];
      S[i*n+j] = v / d;
    }
  }
  for (size_t c = 0; c < m; c++) {
    for (size_t i = 0; i < n; i++) {
      double v = B[i*m+c];
      for (size_t k = 0; k < i; k++) v -= S[i*n+k]*B[k*m+c];
      B[i*m+c] = v / S[i*n+i];
    }
    for (size_t i = n; i-- > 0; ) {
      double v = B[i*m+c];
      for (size_t k = i+1; k < n; k++) v -= S[k*n+i]*B[k*m+c];
      B[i*m+c] = v / S[i*n+i];
    }
  }
  return true;
}

}  // namespace

namespace MoBasisChange {

PropTable* project(PropTable *mo, const vector<EspBasisFunction>& from,
                   const vector<EspBasisFunction>& to)
{
  if (mo == 0) return mo;
  const size_t nb = to.size();
  if (from.size() != nb || mo->columns() != (int)nb) return mo;

  // Atoms by centre; functions on one atom only mix among themselves.
  vector<int> atomOf(nb, -1);
  vector< vector<size_t> > atoms;
  for (size_t i = 0; i < nb; i++) {
    for (size_t a = 0; a < atoms.size() && atomOf[i] < 0; a++) {
      const EspBasisFunction& r = to[atoms[a][0]];
      if (r.center[0] == to[i].center[0] && r.center[1] == to[i].center[1] &&
          r.center[2] == to[i].center[2]) atomOf[i] = (int)a;
    }
    if (atomOf[i] < 0) { atomOf[i] = (int)atoms.size(); atoms.push_back({}); }
    atoms[atomOf[i]].push_back(i);
  }

  const int rows = mo->rows();
  vector<double> c(rows * nb);
  for (int m = 0; m < rows; m++)
    for (size_t i = 0; i < nb; i++) c[m*nb+i] = mo->value(m, i);

  bool changed = false;
  for (size_t a = 0; a < atoms.size(); a++) {
    const vector<size_t>& idx = atoms[a];
    bool differs = false;
    for (size_t i : idx) if (!sameFunction(from[i], to[i])) differs = true;
    if (!differs) continue;
    const size_t n = idx.size();
    vector<double> Stt(n*n), X(n*n);
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < n; j++) {
        Stt[i*n+j] = EspField::overlapOf(to[idx[i]], to[idx[j]]);
        X[i*n+j] = EspField::overlapOf(to[idx[i]], from[idx[j]]);
      }
    vector<double> Sxt = X;   // <to_i|from_j>, kept for the residual
    if (!choleskySolve(Stt, X, n, n)) return mo;
    // from_j = sum_i X_ij to_i; a residual means a different span.
    for (size_t j = 0; j < n; j++) {
      double proj = 0;
      for (size_t i = 0; i < n; i++) proj += Sxt[i*n+j] * X[i*n+j];
      double self = EspField::overlapOf(from[idx[j]], from[idx[j]]);
      if (self - proj > 1e-6 * self) return mo;
    }
    for (int m = 0; m < rows; m++) {
      vector<double> out(n, 0.0);
      for (size_t i = 0; i < n; i++)
        for (size_t j = 0; j < n; j++) out[i] += X[i*n+j] * c[m*nb+idx[j]];
      for (size_t i = 0; i < n; i++) c[m*nb+idx[i]] = out[i];
    }
    changed = true;
  }
  if (!changed) return mo;

  PropTable *result = new PropTable(*mo);
  result->values(rows, nb, c);
  return result;
}

}  // namespace MoBasisChange
