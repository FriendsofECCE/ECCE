//  #325: Clean (UFF) on Fe(eta6-C6H6)(PH3)3 must leave the piano stool alone:
//  Metal bonds keep only a stretch term, so the ring stays planar, the
//  Fe-centroid distance stays near the placed 1.7 A and the P atoms stay on
//  the far side.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "tdat/Fragment.H"
#include "tdat/ForceField.H"
#include "tdat/UFFDesc.H"
#include "tdat/TBond.H"

static int failures = 0;

static void check(bool ok, const std::string& what)
{
  printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) failures++;
}

static double dot(const double* a, const double* b)
{ return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; }

int main()
{
  DisplayStyle style;
  Fragment f;
  const double o[3] = {0, 0, 0};
  f.addAtom("Fe", o);
  // Three PH3 on three tetrahedral directions; the fourth is the nub.
  const double dirs[4][3] = {{1, 1, 1}, {1, -1, -1}, {-1, 1, -1}, {-1, -1, 1}};
  for (int i = 0; i < 3; i++) {
    double l = sqrt(3.0), u[3] = {dirs[i][0]/l, dirs[i][1]/l, dirs[i][2]/l};
    double p[3] = {2.25*u[0], 2.25*u[1], 2.25*u[2]};
    f.addAtom("P", p);
    int ip = f.numAtoms() - 1;
    f.addBond(0, ip, 1.0);
    // Three H at 1.42 A, 94 degrees from the P-Fe axis.
    double w[3] = {u[1], -u[0], 0};
    double wl = sqrt(dot(w, w));
    if (wl < 1e-6) { w[0] = 0; w[1] = u[2]; w[2] = -u[1]; wl = sqrt(dot(w, w)); }
    for (int k = 0; k < 3; k++) w[k] /= wl;
    double v[3] = {u[1]*w[2]-u[2]*w[1], u[2]*w[0]-u[0]*w[2], u[0]*w[1]-u[1]*w[0]};
    for (int h = 0; h < 3; h++) {
      double ph = 2 * M_PI * h / 3, cp = cos(ph), sp = sin(ph);
      double hp[3];
      for (int k = 0; k < 3; k++)
        hp[k] = p[k] + 1.42 * (0.07 * u[k] + 0.9976 * (cp * w[k] + sp * v[k]));
      f.addAtom("H", hp);
      f.addBond(ip, f.numAtoms() - 1, 1.0);
    }
  }
  double l4 = sqrt(3.0), nb[3] = {2.2*dirs[3][0]/l4, 2.2*dirs[3][1]/l4, 2.2*dirs[3][2]/l4};
  f.addAtom("Nub", nb);
  int nubIdx = f.numAtoms() - 1;
  f.addBond(0, nubIdx, 1.0);

  int first = f.numAtoms();
  for (int i = 0; i < 6; i++) {
    double a = 2 * M_PI * i / 6;
    double c[3] = {8 + 1.39 * cos(a), 4 + 1.39 * sin(a), -3};
    f.addAtom("C", c);
  }
  for (int i = 0; i < 6; i++) {
    double a = 2 * M_PI * i / 6;
    double h[3] = {8 + 2.47 * cos(a), 4 + 2.47 * sin(a), -3};
    f.addAtom("H", h);
  }
  for (int i = 0; i < 6; i++) {
    f.addBond(first + i, first + (i + 1) % 6, 1.5);
    f.addBond(first + i, first + 6 + i, 1.0);
  }
  std::vector<TAtm*> pick(1, f.atomRef(first));
  std::string msg;
  f.bondPiLigand(f.atomRef(nubIdx), pick, style, msg);
  f.generateShapes();
  printf("     %s\n", msg.c_str());

  // One measurement; prints and returns the numbers.
  struct M { double cen, dev, pside; };
  M before, after;
  for (int pass = 0; pass < 2; pass++) {
    double c[3] = {0, 0, 0};
    for (int i = 0; i < 6; i++)
      for (int k = 0; k < 3; k++) c[k] += f.atomRef(f.numAtoms() - 12 + i)->coordinates()[k] / 6;
    double d[3] = {c[0] - f.atomRef(0)->coordinates()[0], c[1] - f.atomRef(0)->coordinates()[1],
                   c[2] - f.atomRef(0)->coordinates()[2]};
    double dl = sqrt(dot(d, d));
    double dev = 0, pside = -1e9;
    for (int i = 0; i < 6; i++) {
      const double* q = f.atomRef(f.numAtoms() - 12 + i)->coordinates();
      double r[3] = {q[0] - c[0], q[1] - c[1], q[2] - c[2]};
      dev = std::max(dev, fabs(dot(r, d)) / dl);
    }
    for (int i = 1; i <= 3; i++) {
      // The three P atoms are atoms 1, 5, 9.
      const double* q = f.atomRef(1 + 4 * (i - 1))->coordinates();
      pside = std::max(pside, dot(q, d) / dl);
    }
    M m = {dl, dev, pside};
    if (pass == 0) {
      before = m;
      printf("     before Clean: Fe-centroid %.3f, ring out-of-plane %.3f, "
             "max P along ring direction %.3f\n", m.cen, m.dev, m.pside);
      UFFDesc desc;
      std::vector<int> none;
      ForceField ff(&f, &none, &desc, 0);
      ff.optimize(200, 0.02, 100.0);
    } else {
      after = m;
      printf("     after  Clean: Fe-centroid %.3f, ring out-of-plane %.3f, "
             "max P along ring direction %.3f\n", m.cen, m.dev, m.pside);
    }
  }
  check(fabs(after.cen - before.cen) < 0.2, "Fe-centroid within 0.2 A of the placed value");
  check(after.dev < 0.05, "ring stays planar");
  check(after.pside < 0, "the three P stay on the far side of Fe");
  return failures ? 1 : 0;
}
