///////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: ShellRotation.C
//
// See ShellRotation.H.  The method:
//
//   1. Enumerate the degree-l monomial basis (l = shell_type): all
//      (a,b,c) with a+b+c=l.
//   2. Express each of the shell's m functions in that basis (A, an
//      M x m matrix, M = (l+1)(l+2)/2 >= m).
//   3. For each function, substitute R^-1 into its monomial expansion
//      -- multinomial expansion of each of the three linear forms,
//      multiplied together -- giving the rotated function, ALSO in the
//      degree-l monomial basis (b, length M).
//   4. Solve A x = b by least squares (normal equations; m is at most
//      a handful for any shipped shell) for the rotated function's
//      coefficients over the shell's own basis, and check the fit is
//      (near) exact.
///////////////////////////////////////////////////////////////////////////

#include "tdat/ShellRotation.H"
#include "tdat/SymmetryAnalysis.H"
#include "tdat/TGBSAngFunc.H"
#include "tdat/BasisFlatten.H"

#include <cmath>
#include <map>
#include <algorithm>
using std::map;

namespace {

  struct Triple { int a, b, c; };

  bool operator<(const Triple& x, const Triple& y)
  {
    if (x.a != y.a) return x.a < y.a;
    if (x.b != y.b) return x.b < y.b;
    return x.c < y.c;
  }

  /** All (a,b,c), a+b+c==l, a,b,c>=0, in a fixed canonical order. */
  vector<Triple> monomialsOfDegree(int l)
  {
    vector<Triple> out;
    for (int a = l; a >= 0; a--)
      for (int b = l - a; b >= 0; b--) {
        int c = l - a - b;
        Triple t; t.a = a; t.b = b; t.c = c;
        out.push_back(t);
      }
    return out;
  }

  double factorial(int n)
  {
    double r = 1.0;
    for (int i = 2; i <= n; i++) r *= i;
    return r;
  }

  /**
   * Multinomial expansion of (row[0]*x + row[1]*y + row[2]*z)^power,
   * as a monomial-degree-"power" polynomial: map (i,j,k) -> coefficient.
   */
  map<Triple, double> expandLinearPower(const double row[3], int power)
  {
    map<Triple, double> out;
    if (power == 0) {
      Triple t; t.a = t.b = t.c = 0;
      out[t] = 1.0;
      return out;
    }
    const double fp = factorial(power);
    for (int i = 0; i <= power; i++) {
      for (int j = 0; j <= power - i; j++) {
        int k = power - i - j;
        const double coef = fp / (factorial(i)*factorial(j)*factorial(k))
                           * pow(row[0], i) * pow(row[1], j) * pow(row[2], k);
        if (coef == 0.0) continue;
        Triple t; t.a = i; t.b = j; t.c = k;
        out[t] += coef;
      }
    }
    return out;
  }

  /**
   * The rotated monomial X^a Y^b Z^c, where X,Y,Z are the linear forms
   * given by rowX/rowY/rowZ (rows of R^-1) -- i.e. the substitution
   * that turns f(R^-1 p) into a polynomial in p's own coordinates.
   */
  map<Triple, double> rotateMonomial(int a, int b, int c,
                                     const double rowX[3],
                                     const double rowY[3],
                                     const double rowZ[3])
  {
    map<Triple, double> X = expandLinearPower(rowX, a);
    map<Triple, double> Y = expandLinearPower(rowY, b);
    map<Triple, double> Z = expandLinearPower(rowZ, c);

    map<Triple, double> out;
    for (map<Triple,double>::const_iterator ix = X.begin(); ix != X.end(); ix++) {
      for (map<Triple,double>::const_iterator iy = Y.begin(); iy != Y.end(); iy++) {
        const double cxy = ix->second * iy->second;
        if (cxy == 0.0) continue;
        for (map<Triple,double>::const_iterator iz = Z.begin(); iz != Z.end(); iz++) {
          Triple t;
          t.a = ix->first.a + iy->first.a + iz->first.a;
          t.b = ix->first.b + iy->first.b + iz->first.b;
          t.c = ix->first.c + iy->first.c + iz->first.c;
          out[t] += cxy * iz->second;
        }
      }
    }
    return out;
  }

  /** Solve a small SYMMETRIC positive (semi-)definite system Ax=b by
   *  Gaussian elimination with partial pivoting. */
  bool solveSmall(vector< vector<double> > A, vector<double> b,
                  vector<double>& x)
  {
    const int n = (int)b.size();
    x.assign(n, 0.0);
    for (int col = 0; col < n; col++) {
      int piv = col;
      double best = fabs(A[col][col]);
      for (int r = col+1; r < n; r++) {
        if (fabs(A[r][col]) > best) { best = fabs(A[r][col]); piv = r; }
      }
      if (best < 1.0e-13) continue;   // singular column -> leave x[col]=0
      if (piv != col) { std::swap(A[piv], A[col]); std::swap(b[piv], b[col]); }

      for (int r = col+1; r < n; r++) {
        const double f = A[r][col] / A[col][col];
        if (f == 0.0) continue;
        for (int cc = col; cc < n; cc++) A[r][cc] -= f*A[col][cc];
        b[r] -= f*b[col];
      }
    }
    for (int r = n-1; r >= 0; r--) {
      double s = b[r];
      for (int cc = r+1; cc < n; cc++) s -= A[r][cc]*x[cc];
      x[r] = (fabs(A[r][r]) > 1.0e-13) ? s / A[r][r] : 0.0;
    }
    return true;
  }
}


bool ShellRotation::buildD(int shell_type, TGBSAngFunc *angfunc,
                           const SymOp& R, double residualTol,
                           vector< vector<double> >& D)
{
  D.clear();
  if (angfunc == 0 || shell_type < 0) return false;

  const int m = angfunc->numFuncs(shell_type);
  if (m <= 0) return false;
  const int l = shell_type;

  vector<Triple> mono = monomialsOfDegree(l);
  const int M = (int)mono.size();
  map<Triple, int> index;
  for (int i = 0; i < M; i++) index[mono[i]] = i;

  //  A: each column is one shell function's expansion in the degree-l
  //  monomial basis, WITH the odd-normalisation factor folded in --
  //  the same scaling BasisFlatten::flatten() applies, since a
  //  component-dependent scale (spherical d/f) changes what "a linear
  //  combination of the shell's own functions" means.
  vector< vector<double> > A(M, vector<double>(m, 0.0));
  for (int deg = 0; deg < m; deg++) {
    AngMomFunc terms = angfunc->getFunc(shell_type, deg);
    const double oddN = BasisFlatten::getoddNormalize(shell_type, deg, angfunc);
    for (size_t t = 0; t < terms.size(); t++) {
      if (terms[t].m_k != 0) return false;   // no shipped table uses r^k
      Triple key; key.a = terms[t].m_l; key.b = terms[t].m_m; key.c = terms[t].m_n;
      map<Triple,int>::const_iterator it = index.find(key);
      if (it == index.end()) return false;   // not degree l -- malformed table
      A[it->second][deg] += terms[t].m_coefficient * oddN;
    }
  }

  //  R^-1 = R^T (R is orthogonal): row i of R^-1 is column i of R.
  double rowX[3] = { R.m[0][0], R.m[1][0], R.m[2][0] };
  double rowY[3] = { R.m[0][1], R.m[1][1], R.m[2][1] };
  double rowZ[3] = { R.m[0][2], R.m[1][2], R.m[2][2] };

  //  A^T A (m x m) once; reused for every column's least-squares solve.
  vector< vector<double> > AtA(m, vector<double>(m, 0.0));
  for (int i = 0; i < m; i++)
    for (int j = 0; j < m; j++) {
      double s = 0.0;
      for (int k = 0; k < M; k++) s += A[k][i]*A[k][j];
      AtA[i][j] = s;
    }

  D.assign(m, vector<double>(m, 0.0));
  double worstResidual = 0.0;

  for (int deg = 0; deg < m; deg++) {
    AngMomFunc terms = angfunc->getFunc(shell_type, deg);
    const double oddN = BasisFlatten::getoddNormalize(shell_type, deg, angfunc);

    vector<double> b(M, 0.0);
    for (size_t t = 0; t < terms.size(); t++) {
      if (terms[t].m_k != 0) return false;
      map<Triple,double> rotated = rotateMonomial(terms[t].m_l, terms[t].m_m,
                                                   terms[t].m_n, rowX, rowY, rowZ);
      const double c = terms[t].m_coefficient * oddN;
      for (map<Triple,double>::const_iterator ir = rotated.begin();
           ir != rotated.end(); ir++) {
        map<Triple,int>::const_iterator it = index.find(ir->first);
        if (it == index.end()) return false;  // rotation left degree l -- can't happen
        b[it->second] += c * ir->second;
      }
    }

    vector<double> Atb(m, 0.0);
    for (int i = 0; i < m; i++) {
      double s = 0.0;
      for (int k = 0; k < M; k++) s += A[k][i]*b[k];
      Atb[i] = s;
    }

    vector<double> x;
    if (!solveSmall(AtA, Atb, x)) return false;

    //  Residual over the FULL monomial basis, not just AtA's -- the
    //  check that the shell's span is actually rotation-invariant, not
    //  only that a least-squares line was drawn through it.
    double sse = 0.0, bnorm = 0.0;
    for (int k = 0; k < M; k++) {
      double fit = 0.0;
      for (int i = 0; i < m; i++) fit += A[k][i]*x[i];
      sse += (fit - b[k])*(fit - b[k]);
      bnorm += b[k]*b[k];
    }
    const double residual = sqrt(sse / (bnorm > 1.0e-30 ? bnorm : 1.0));
    if (residual > worstResidual) worstResidual = residual;
    if (residual > residualTol) return false;

    for (int i = 0; i < m; i++) D[i][deg] = x[i];
  }

  return true;
}
