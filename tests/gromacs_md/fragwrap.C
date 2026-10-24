// #512: an existing calculation's stored structure (a PDB with CRYST1 from a
// pre-9.3.3 run, the brick as mdrun wrote it) comes out of the Fragment wrapped
// into the compact cell, waters whole and bonded.
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>

#include "dsm/GromacsStructure.H"
#include "tdat/Fragment.H"
#include "tdat/LatticeDef.H"
#include "tdat/TAtm.H"

static int failures = 0;
static void check(bool ok, const char* what)
{
  printf("%s %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) failures++;
}

static bool inCell(const double a[3][3], const double* p)
{
  double c[3], d0 = 0;
  for (int k = 0; k < 3; k++) {
    c[k] = 0.5 * (a[0][k] + a[1][k] + a[2][k]);
    d0 += (p[k] - c[k]) * (p[k] - c[k]);
  }
  for (int i = -2; i <= 2; i++)
    for (int j = -2; j <= 2; j++)
      for (int m = -2; m <= 2; m++) {
        double d = 0;
        for (int k = 0; k < 3; k++) {
          double t = p[k] - c[k] - (i*a[0][k] + j*a[1][k] + m*a[2][k]);
          d += t * t;
        }
        if (d < d0 - 1e-6) return false;
      }
  return true;
}

int main(int argc, char** argv)
{
  if (argc < 2) return 2;
  std::ifstream f(argv[1]);
  std::stringstream ss;
  ss << f.rdbuf();
  std::string pdb, err;
  check(GromacsStructure::groToPdb(ss.str(), pdb, err) > 100, "gro to pdb");
  std::istringstream is(pdb);
  Fragment frag;
  check(frag.restorePDB(is, 1.0, true), "restorePDB");
  check(frag.getLattice() != 0, "lattice from CRYST1");
  if (!frag.getLattice()) return 1;
  std::vector<MPoint>* v = frag.getLattice()->toVectors();
  double a[3][3];
  for (int i = 0; i < 3; i++) {
    a[i][0] = (*v)[i].x(); a[i][1] = (*v)[i].y(); a[i][2] = (*v)[i].z();
  }
  delete v;

  const int n = (int)frag.numAtoms();
  int out0 = 0, out1 = 0, torn = 0;
  for (int m = 0; m + 2 < n; m += 3) {
    double c[3] = {0,0,0};
    for (int i = m; i < m + 3; i++)
      for (int k = 0; k < 3; k++) c[k] += frag.atomRef(i)->coordinates()[k] / 3;
    if (!inCell(a, c)) out0++;
  }
  check(out0 > 10, "stored structure has waters outside the compact cell");
  check(frag.wrapIntoCompactCell(), "wrapIntoCompactCell");
  for (int m = 0; m + 2 < n; m += 3) {
    double c[3] = {0,0,0};
    const double* o = frag.atomRef(m)->coordinates();
    for (int i = m; i < m + 3; i++) {
      const double* p = frag.atomRef(i)->coordinates();
      double d = 0;
      for (int k = 0; k < 3; k++) { c[k] += p[k] / 3; d += (p[k]-o[k])*(p[k]-o[k]); }
      if (std::sqrt(d) > 1.2) torn++;
    }
    if (!inCell(a, c)) out1++;
  }
  check(out1 == 0, "every molecule centre inside the compact cell");
  check(torn == 0, "every water whole");
  check((int)frag.numBonds() >= 2 * (n / 3), "O-H bonds present after rebond");
  return failures ? 1 : 0;
}
