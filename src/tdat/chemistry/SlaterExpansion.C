#include <cmath>
#include <cstddef>

#include "tdat/SlaterExpansion.H"

namespace {

  //  The radial integration grid.  The integrand is weighted by r^2, so
  //  the useful range scales as 1/zeta; 60/zeta covers a 6s function's
  //  tail to well below the precision anything downstream keeps.
  const int    GRID_POINTS = 6000;
  const double GRID_EXTENT = 60.0;

  //  Even-tempered ladders are scanned rather than assumed: a ratio
  //  tuned for a 1s function is a poor one for 6s, and the scan costs
  //  microseconds against a rendering pass.
  const double RATIOS[] = { 1.8, 2.0, 2.2, 2.5, 3.0, 3.5, 4.0, 5.0 };
  const int    NUM_RATIOS = 8;
  const double SHIFT_MIN = -3.0;
  const double SHIFT_MAX =  3.0;
  const double SHIFT_STEP = 0.25;

  /** Solve a symmetric positive definite system by Cholesky. */
  bool solveSPD(vector< vector<double> >& a, vector<double>& b)
  {
    const size_t n = b.size();

    for (size_t i = 0; i < n; i++) {
      for (size_t j = 0; j <= i; j++) {
        double sum = a[i][j];
        for (size_t k = 0; k < j; k++) sum -= a[i][k]*a[j][k];
        if (i == j) {
          if (sum <= 0.0) return false;       // not positive definite
          a[i][i] = sqrt(sum);
        } else {
          a[i][j] = sum/a[j][j];
        }
      }
    }
    for (size_t i = 0; i < n; i++) {          // forward substitution
      double sum = b[i];
      for (size_t k = 0; k < i; k++) sum -= a[i][k]*b[k];
      b[i] = sum/a[i][i];
    }
    for (size_t i = n; i-- > 0; ) {           // back substitution
      double sum = b[i];
      for (size_t k = i+1; k < n; k++) sum -= a[k][i]*b[k];
      b[i] = sum/a[i][i];
    }
    return true;
  }
}


double SlaterExpansion::fitFixed(int n, int l, double zeta,
                                 const vector<double>& exponents,
                                 vector<double>& coefficients)
{
  const size_t ng = exponents.size();
  if (ng == 0 || zeta <= 0.0) return -1.0;

  const double rmax = GRID_EXTENT/zeta;
  const double dr   = rmax/GRID_POINTS;

  //  Normal equations for the least-squares fit, integrated on the grid
  //  with the r^2 radial measure.
  vector< vector<double> > a(ng, vector<double>(ng, 0.0));
  vector<double> rhs(ng, 0.0);
  double targetNorm = 0.0;

  vector<double> basis(ng);

  for (int p = 1; p <= GRID_POINTS; p++) {
    const double r  = p*dr;
    const double r2 = r*r;
    const double w  = r2*dr;

    double rl = 1.0;
    for (int k = 0; k < l; k++) rl *= r;
    double rn = 1.0;
    for (int k = 0; k < n-1; k++) rn *= r;

    const double target = rn*exp(-zeta*r);
    targetNorm += target*target*w;

    for (size_t i = 0; i < ng; i++) basis[i] = rl*exp(-exponents[i]*r2);

    for (size_t i = 0; i < ng; i++) {
      rhs[i] += basis[i]*target*w;
      for (size_t j = 0; j <= i; j++) a[i][j] += basis[i]*basis[j]*w;
    }
  }
  for (size_t i = 0; i < ng; i++)
    for (size_t j = i+1; j < ng; j++) a[i][j] = a[j][i];

  //  A ladder that is too dense makes the normal equations singular;
  //  nudging the diagonal keeps the solve well posed and costs nothing
  //  at the accuracy that matters here.
  double trace = 0.0;
  for (size_t i = 0; i < ng; i++) trace += a[i][i];
  for (size_t i = 0; i < ng; i++) a[i][i] += 1.0e-12*trace/ng;

  coefficients = rhs;
  if (!solveSPD(a, coefficients)) return -1.0;

  //  Overlap of the fit with the exact function, both normalised.
  double num = 0.0, fitNorm = 0.0;
  for (int p = 1; p <= GRID_POINTS; p++) {
    const double r  = p*dr;
    const double r2 = r*r;
    const double w  = r2*dr;

    double rl = 1.0;
    for (int k = 0; k < l; k++) rl *= r;
    double rn = 1.0;
    for (int k = 0; k < n-1; k++) rn *= r;

    double value = 0.0;
    for (size_t i = 0; i < ng; i++)
      value += coefficients[i]*rl*exp(-exponents[i]*r2);

    const double target = rn*exp(-zeta*r);
    num     += value*target*w;
    fitNorm += value*value*w;
  }
  if (fitNorm <= 0.0 || targetNorm <= 0.0) return -1.0;
  return num/sqrt(fitNorm*targetNorm);
}


double SlaterExpansion::fit(int n, int l, double zeta, int nGauss,
                            vector<double>& exponents,
                            vector<double>& coefficients)
{
  exponents.clear();
  coefficients.clear();
  if (n < 1 || l < 0 || l > n-1 || zeta <= 0.0 || nGauss < 1) return -1.0;

  double best = -1.0;
  vector<double> trialExp(nGauss), trialCoef;

  for (int ri = 0; ri < NUM_RATIOS; ri++) {
    const double b = RATIOS[ri];

    for (double shift = SHIFT_MIN; shift <= SHIFT_MAX+1e-9; shift += SHIFT_STEP) {
      //  Centre the ladder on the Slater's own scale, zeta^2, then slide
      //  it; the widest and tightest primitives bracket that centre.
      const double a0 = zeta*zeta*pow(b, shift)/pow(b, (nGauss-1)/2.0);

      double alpha = a0;
      for (int k = 0; k < nGauss; k++) { trialExp[k] = alpha; alpha *= b; }

      const double overlap = fitFixed(n, l, zeta, trialExp, trialCoef);
      if (overlap > best) {
        best = overlap;
        exponents = trialExp;
        coefficients = trialCoef;
      }
    }
  }
  return best;
}
