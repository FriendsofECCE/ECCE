//  Coulomb integrals checked against cases whose answer is known in
//  closed form, because a wrong potential still renders as a smooth,
//  plausible-looking surface.
//
//  The reference values are derived, not hand-copied.  Two earlier
//  versions of this file failed against correct code: one compared the
//  Boys function at two DIFFERENT arguments either side of the
//  small/large-T branch and expected them equal, when F_0 genuinely
//  changes by 5e-6 across that interval; the other used a hand-written
//  normalisation constant for a Gaussian product.  Both looked like
//  bugs in the integrals.
#include <cstdio>
#include <cmath>
#include <vector>
#include "tdat/CoulombIntegrals.H"
using namespace std;

static const double PI = 3.14159265358979323846;
static int bad = 0;

static void check(const char* what, double got, double want, double tol) {
  bool ok = fabs(got-want) <= tol*(1.0 + fabs(want));
  printf("  %-52s %15.10f %15.10f  %s\n", what, got, want, ok ? "ok" : "FAIL");
  if (!ok) bad++;
}

/** F_n(T) by Simpson's rule -- independent of the code under test. */
static double boysQuad(int n, double T) {
  const int N = 200000;
  double h = 1.0/N, sum = 0.0;
  for (int i = 0; i <= N; i++) {
    double t = i*h;
    double f = pow(t, 2*n)*exp(-T*t*t);
    double w = (i == 0 || i == N) ? 1.0 : ((i % 2) ? 4.0 : 2.0);
    sum += w*f;
  }
  return sum*h/3.0;
}

/**
 * The exact potential at C of the product of two s Gaussians.
 *
 * Their product is a Gaussian at P of exponent p weighted by
 * exp(-mu |AB|^2), and a Gaussian of unit charge has potential
 * erf(sqrt(p) R)/R, so the unnormalised product gives (pi/p)^(3/2)
 * times that.
 */
static double exactSS(double a, double b, const double* A, const double* B,
                      const double* C) {
  double p = a + b, mu = a*b/p, d2 = 0.0, R2 = 0.0;
  for (int k = 0; k < 3; k++) {
    double ab = A[k]-B[k];
    d2 += ab*ab;
    double pc = (a*A[k] + b*B[k])/p - C[k];
    R2 += pc*pc;
  }
  double R = sqrt(R2);
  double shape = (R < 1e-12) ? 2.0*sqrt(p/PI) : erf(sqrt(p)*R)/R;
  return pow(PI/p, 1.5)*exp(-mu*d2)*shape;
}

int main() {
  vector<double> F;

  printf("  Boys function against numerical quadrature\n");
  printf("  %-52s %15s %15s\n", "", "got", "reference");
  double Ts[] = {0.0, 0.5, 5.0, 20.0, 29.999, 30.001, 100.0};
  for (int i = 0; i < 7; i++) {
    CoulombIntegrals::boys(4, Ts[i], F);
    for (int n = 0; n <= 4; n += 4) {
      char label[90];
      sprintf(label, "F_%d(%.3f)", n, Ts[i]);
      check(label, F[n], boysQuad(n, Ts[i]), 1e-9);
    }
  }

  printf("\n  Nuclear attraction integrals against the closed form\n");
  double a = 0.8, b = 1.3;
  int s[3] = {0,0,0};

  //  One centre, several distances.
  double A[3] = {0,0,0}, B[3] = {0,0,0};
  for (int i = 0; i < 3; i++) {
    double C[3] = {0.5 + i*1.5, 0, 0};
    char label[90]; sprintf(label, "s*s, one centre, R=%.1f", C[0]);
    check(label, CoulombIntegrals::potential(A, s, a, B, s, b, C),
          exactSS(a, b, A, B, C), 1e-9);
  }
  //  Two centres.
  {
    double B2[3] = {1.2, 0, 0};
    double C[3] = {5.0, 0.3, -0.7};
    check("s*s, two centres", 
          CoulombIntegrals::potential(A, s, a, B2, s, b, C),
          exactSS(a, b, A, B2, C), 1e-9);
  }
  //  Far field: the potential of the enclosed charge, 1/R.
  {
    double C[3] = {80.0, 0, 0};
    double q = pow(PI/(a+b), 1.5);
    check("s*s far field -> q/R", 
          CoulombIntegrals::potential(A, s, a, B, s, b, C), q/80.0, 1e-9);
  }
  //  At the centre of the cloud, where the point-charge form diverges
  //  and the true integral does not.
  {
    double C[3] = {0, 0, 0};
    check("s*s at the cloud centre is finite",
          CoulombIntegrals::potential(A, s, a, B, s, b, C),
          exactSS(a, b, A, B, C), 1e-9);
  }
  //  Odd parity must vanish exactly.
  {
    int px[3] = {1,0,0};
    double C[3] = {0.0, 2.0, 0.0};
    check("px*s vanishes on the x=0 plane",
          CoulombIntegrals::potential(A, px, a, B, s, b, C), 0.0, 1e-12);
    int dxy[3] = {1,1,0};
    check("dxy*s vanishes on the x=0 plane",
          CoulombIntegrals::potential(A, dxy, a, B, s, b, C), 0.0, 1e-12);
  }
  //  Symmetry: swapping the two primitives cannot change the integral.
  {
    int px[3] = {1,0,0}, dxz[3] = {1,0,1};
    double B2[3] = {0.7,-0.4,0.9}, C[3] = {1.1, 0.2, -0.5};
    double ab = CoulombIntegrals::potential(A, px, a, B2, dxz, b, C);
    double ba = CoulombIntegrals::potential(B2, dxz, b, A, px, a, C);
    check("p*d is symmetric under exchange", ab, ba, 1e-12);
  }

  printf("\n  %s\n", bad ? "FAIL" : "PASS");
  return bad ? 1 : 0;
}
