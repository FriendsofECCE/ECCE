// Analytic AO derivatives, the evaluator's density/gradient/Hessian/tau and the
// guards of ELF, LOL, s (#799, #800), against finite differences.
//
//   densityfields_test FIXTURE-ROOT      (tests/tdat/densityfields-fixtures)
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "util/EcceURL.H"
#include "tdat/DensityFields.H"
#include "tdat/EspField.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/IPropCalculation.H"
#include "dsm/Resource.H"

using namespace std;
using DensityPlane::FieldPoint;

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { ++failures; \
  fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); } } while (0)
static const double ANG = 0.52917721092;

// A d-like function with an s admixture and two primitives, off the origin.
static void basisFunction()
{
  EspBasisFunction f;
  f.center[0] = 0.3; f.center[1] = -0.2; f.center[2] = 0.1;
  f.exponent = { 1.3, 0.21 };
  f.contraction = { 0.7, 0.4 };
  f.angularCoef = { 1.0, -0.5, 0.8, 0.6, 0.9 };
  f.powerX = { 2, 0, 1, 3, 0 };
  f.powerY = { 0, 2, 1, 0, 1 };
  f.powerZ = { 0, 0, 1, 1, 0 };
  const double p[3] = { 0.9, 0.4, -0.7 }, h = 1e-4;
  double v, g[3], H[6];
  f.derivatives(p[0], p[1], p[2], v, g, H);
  CHECK(fabs(v - f.value(p[0], p[1], p[2])) < 1e-14, "derivatives() value equals value()");
  const int ka[6] = {0, 1, 2, 0, 0, 1}, kb[6] = {0, 1, 2, 1, 2, 2};
  for (int k = 0; k < 3; k++) {
    double q[3] = { p[0], p[1], p[2] }, m[3] = { p[0], p[1], p[2] };
    q[k] += h; m[k] -= h;
    const double fd = (f.value(q[0], q[1], q[2]) - f.value(m[0], m[1], m[2]))/(2*h);
    CHECK(fabs(fd - g[k]) < 1e-7, "AO gradient against finite differences");
    double vg1[3], vg2[3], dummy, hh[6];
    f.derivatives(q[0], q[1], q[2], dummy, vg1, hh);
    f.derivatives(m[0], m[1], m[2], dummy, vg2, hh);
    for (int l = 0; l < 3; l++) {
      int idx = -1;
      for (int s = 0; s < 6; s++)
        if ((ka[s] == k && kb[s] == l) || (ka[s] == l && kb[s] == k)) idx = s;
      CHECK(fabs((vg1[l] - vg2[l])/(2*h) - H[idx]) < 1e-6, "AO Hessian against finite differences");
    }
  }
}

static void guards()
{
  FieldPoint p;       // empty: rho = 0
  CHECK(DensityFields::elf(p) == 0.0 && DensityFields::lol(p) == 0.0, "ELF and LOL are 0 at rho = 0");
  CHECK(DensityFields::rdg(p) == DensityFields::RDG_LARGE, "s is large at rho = 0");

  // Uniform gas, closed shell: tau = tau_TF, grad = 0 gives D = 0.3 (3 pi^2)^(2/3) rho^(5/3) - 0 ... ELF 1/2 at D = D0.
  const double rho = 0.05, pi = 3.14159265358979323846;
  p.rho[0] = p.rho[1] = rho/2; p.rhoTotal = rho;
  p.tau[0] = p.tau[1] = 0.5*0.3*pow(3*pi*pi, 2.0/3.0)*pow(rho, 5.0/3.0);
  CHECK(fabs(DensityFields::elf(p) - 0.5) < 1e-12, "ELF of the uniform gas is 1/2");
  CHECK(fabs(DensityFields::lol(p) - 0.5) < 1e-12, "LOL of the uniform gas is 1/2");
}

static void pointsOf(const string& root, const string& name, const double pts[][3], int npts)
{
  string url = "file://" + root + "/" + name;
  EDSIServerCentral central;
  Resource *res = EDSIFactory::getResource(EcceURL(url));
  IPropCalculation *calc = dynamic_cast<IPropCalculation*>(res);
  DensityPlane::Evaluator ev;
  if (!calc || !ev.build(calc)) { ++failures; cerr << "FAIL: cannot open " << url << "\n"; return; }
  const double h = 5e-4;        // Angstrom
  for (int n = 0; n < npts; n++) {
    const double *x = pts[n];
    FieldPoint p;
    ev.fieldsAt(x[0], x[1], x[2], p);
    CHECK(fabs(p.rhoTotal - ev.density(x[0], x[1], x[2])) < 1e-12*(1 + p.rhoTotal), "rho equals density()");
    CHECK(fabs(p.rho[0] - p.rho[1] - ev.spinDensity(x[0], x[1], x[2])) < 1e-12, "alpha - beta equals spinDensity()");
    double g[3];
    FieldPoint a, b;
    for (int k = 0; k < 3; k++) {
      double q[3] = { x[0], x[1], x[2] }, m[3] = { x[0], x[1], x[2] };
      q[k] += h; m[k] -= h;
      ev.fieldsAt(q[0], q[1], q[2], a);
      ev.fieldsAt(m[0], m[1], m[2], b);
      g[k] = (a.rhoTotal - b.rhoTotal)/(2*h)*ANG;           // per bohr
      if (fabs(g[k] - p.gradTotal[k]) >= 1e-5*(1 + fabs(g[k])))
        fprintf(stderr, "  %s point %d axis %d: fd %.8g analytic %.8g\n", name.c_str(), n, k, g[k], p.gradTotal[k]);
      CHECK(fabs(g[k] - p.gradTotal[k]) < 1e-5*(1 + fabs(g[k])), "density gradient against finite differences");
      for (int l = 0; l < 3; l++) {
        const int idx = k == l ? k : (k + l == 1 ? 3 : (k + l == 2 ? 4 : 5));
        CHECK(fabs((a.gradTotal[l] - b.gradTotal[l])/(2*h)*ANG - p.hess[idx]) < 1e-4*(1 + fabs(p.hess[idx])),
              "density Hessian against finite differences of the gradient");
      }
    }
    CHECK(p.eig[0] <= p.eig[1] && p.eig[1] <= p.eig[2], "eigenvalues ascending");
    CHECK(fabs(p.eig[0] + p.eig[1] + p.eig[2] - p.hess[0] - p.hess[1] - p.hess[2]) < 1e-9*(1 + fabs(p.hess[0])),
          "eigenvalues sum to the trace");
    // det and the sum of principal 2x2 minors are invariants too
    const double *h6 = p.hess;
    const double det = h6[0]*(h6[1]*h6[2] - h6[5]*h6[5]) - h6[3]*(h6[3]*h6[2] - h6[5]*h6[4])
                     + h6[4]*(h6[3]*h6[5] - h6[1]*h6[4]);
    CHECK(fabs(p.eig[0]*p.eig[1]*p.eig[2] - det) < 1e-7*(1 + fabs(det)), "eigenvalues multiply to the determinant");
    // tau against sum of n |grad psi|^2 / 2 from finite differences of the orbitals
    double tau[2] = {0, 0};
    for (int s = 0; s < 2; s++)
      for (int mo = 1; mo <= ev.orbitals(ev.unrestricted && s == 1); mo++) {
        const vector<double>& oc = ev.unrestricted && s == 1 ? ev.occBeta : ev.occ;
        const double n = ev.unrestricted ? oc[mo-1] : (s == 0 ? min(oc[mo-1], 1.0) : oc[mo-1] - min(oc[mo-1], 1.0));
        if (n == 0.0) continue;
        double gg = 0;
        for (int k = 0; k < 3; k++) {
          double q[3] = { x[0], x[1], x[2] }, m[3] = { x[0], x[1], x[2] };
          q[k] += h; m[k] -= h;
          const bool beta = ev.unrestricted && s == 1;
          const double d = (ev.orbital(mo, beta, q[0], q[1], q[2]) - ev.orbital(mo, beta, m[0], m[1], m[2]))/(2*h)*ANG;
          gg += d*d;
        }
        tau[s] += 0.5*n*gg;
      }
    CHECK(fabs(tau[0] - p.tau[0]) < 1e-5*(1 + tau[0]) && fabs(tau[1] - p.tau[1]) < 1e-5*(1 + tau[1]),
          "tau against finite differences of the orbitals");
  }
}

int main(int argc, char **argv)
{
  if (argc < 2) { cerr << "usage: densityfields_test FIXTURE-ROOT\n"; return 2; }
  basisFunction();
  guards();
  const double water[][3] = { {0, 0, 0.5}, {0.3, 0.4, -0.9}, {0.2, 0.9, -0.3}, {0, 1.5, 0.2}, {1.1, -0.6, 0.7} };
  const double o2[][3] = { {0.1, 0.2, 0.3}, {0, 0.9, 0}, {0.5, 0, 1.2}, {0.9, 0.3, 0.2} };
  pointsOf(argv[1], "water", water, 5);
  pointsOf(argv[1], "o2", o2, 4);          // unrestricted
  if (failures) { cerr << failures << " failures\n"; return 1; }
  cout << "ok\n";
  return 0;
}
