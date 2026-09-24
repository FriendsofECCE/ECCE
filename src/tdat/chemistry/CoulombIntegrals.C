#include <cmath>
#include <cstddef>

#include "tdat/CoulombIntegrals.H"

namespace {

  const double PI = 3.14159265358979323846;

  //  Above this the series below converges slowly and the asymptotic
  //  form is both faster and more accurate.
  const double BOYS_ASYMPTOTIC = 30.0;

  /**
   * Hermite expansion coefficient E^{ij}_t for one Cartesian direction.
   *
   * Recursive rather than tabulated: the depth is the angular momentum,
   * which is at most 4 here, and a table indexed wrongly is the kind of
   * error that produces a plausible surface.
   */
  double hermite(int i, int j, int t, double dist, double a, double b)
  {
    const double p = a + b;
    const double mu = a*b/p;

    if (t < 0 || t > i + j) return 0.0;

    if (i == 0 && j == 0) {
      return exp(-mu*dist*dist);
    }
    if (j == 0) {
      return (1.0/(2.0*p)) * hermite(i-1, j, t-1, dist, a, b)
             - (mu*dist/a)  * hermite(i-1, j, t,   dist, a, b)
             + (t+1)        * hermite(i-1, j, t+1, dist, a, b);
    }
    return (1.0/(2.0*p)) * hermite(i, j-1, t-1, dist, a, b)
           + (mu*dist/b)  * hermite(i, j-1, t,   dist, a, b)
           + (t+1)        * hermite(i, j-1, t+1, dist, a, b);
  }

  /**
   * Hermite Coulomb integral R^n_{tuv}.
   */
  double hermiteCoulomb(int t, int u, int v, int n, double p,
                        double PCx, double PCy, double PCz,
                        const vector<double>& F)
  {
    //  A negative index means the term does not exist.  Without this
    //  the t<0 case falls through to the v branch and recurses on
    //  v-1 forever.
    if (t < 0 || u < 0 || v < 0) return 0.0;

    if (t == 0 && u == 0 && v == 0) {
      double factor = 1.0;
      for (int k = 0; k < n; k++) factor *= -2.0*p;
      return factor*F[n];
    }

    if (t > 0) {
      double value = (t-1)*hermiteCoulomb(t-2, u, v, n+1, p, PCx, PCy, PCz, F);
      return value + PCx*hermiteCoulomb(t-1, u, v, n+1, p, PCx, PCy, PCz, F);
    }
    if (u > 0) {
      double value = (u-1)*hermiteCoulomb(t, u-2, v, n+1, p, PCx, PCy, PCz, F);
      return value + PCy*hermiteCoulomb(t, u-1, v, n+1, p, PCx, PCy, PCz, F);
    }
    double value = (v-1)*hermiteCoulomb(t, u, v-2, n+1, p, PCx, PCy, PCz, F);
    return value + PCz*hermiteCoulomb(t, u, v-1, n+1, p, PCx, PCy, PCz, F);
  }
}


void CoulombIntegrals::boys(int n, double T, vector<double>& values)
{
  values.assign(n+1, 0.0);
  if (n < 0) return;

  if (T < BOYS_ASYMPTOTIC) {
    //  Series for the highest order, then downward recursion, which is
    //  the stable direction.  Upward recursion at small T subtracts two
    //  nearly equal numbers and loses most of the precision.
    double term = 1.0/(2.0*n + 1.0);
    double sum = term;
    for (int k = 1; k < 200; k++) {
      term *= (2.0*T)/(2.0*n + 2.0*k + 1.0);
      sum += term;
      if (term < 1.0e-16*sum) break;
    }
    values[n] = sum*exp(-T);

    for (int m = n; m > 0; m--) {
      values[m-1] = (2.0*T*values[m] + exp(-T))/(2.0*m - 1.0);
    }
  } else {
    //  Large T: the integrand is dominated by the lower limit and F_0
    //  approaches half the Gaussian integral.  Here upward recursion is
    //  the stable direction.
    values[0] = 0.5*sqrt(PI/T);
    for (int m = 1; m <= n; m++) {
      values[m] = ((2.0*m - 1.0)*values[m-1] - exp(-T))/(2.0*T);
    }
  }
}


double CoulombIntegrals::potential(const double* A, const int* la,
                                   double alpha,
                                   const double* B, const int* lb,
                                   double beta,
                                   const double* C)
{
  const double p = alpha + beta;

  double P[3], PC[3];
  double PC2 = 0.0;
  for (int k = 0; k < 3; k++) {
    P[k]  = (alpha*A[k] + beta*B[k])/p;
    PC[k] = P[k] - C[k];
    PC2  += PC[k]*PC[k];
  }

  const int maxOrder = la[0]+lb[0] + la[1]+lb[1] + la[2]+lb[2];

  vector<double> F;
  boys(maxOrder, p*PC2, F);

  double total = 0.0;
  for (int t = 0; t <= la[0]+lb[0]; t++) {
    const double Ex = hermite(la[0], lb[0], t, A[0]-B[0], alpha, beta);
    if (Ex == 0.0) continue;

    for (int u = 0; u <= la[1]+lb[1]; u++) {
      const double Ey = hermite(la[1], lb[1], u, A[1]-B[1], alpha, beta);
      if (Ey == 0.0) continue;

      for (int v = 0; v <= la[2]+lb[2]; v++) {
        const double Ez = hermite(la[2], lb[2], v, A[2]-B[2], alpha, beta);
        if (Ez == 0.0) continue;

        total += Ex*Ey*Ez
                 * hermiteCoulomb(t, u, v, 0, p, PC[0], PC[1], PC[2], F);
      }
    }
  }

  return total*2.0*PI/p;
}


double CoulombIntegrals::overlap(const double* A, const int* la,
                                 double alpha,
                                 const double* B, const int* lb,
                                 double beta)
{
  const double p = alpha + beta;

  //  Only the t = u = v = 0 Hermite coefficient survives an overlap:
  //  the higher ones integrate to zero over all space.  That is the
  //  whole difference from the potential above, which contracts every
  //  coefficient against the Hermite Coulomb integrals.
  double out = pow(PI/p, 1.5);
  for (int k = 0; k < 3; k++) {
    out *= hermite(la[k], lb[k], 0, A[k]-B[k], alpha, beta);
  }
  return out;
}
