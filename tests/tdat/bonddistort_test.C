// BondDistort on octahedral SF6 and [Ti(H2O)6]-like: every distance moves by
// exactly dr along its bond; the Jahn-Teller preset gives +d / -d/2 (#619).
#include <cmath>
#include <cstdio>
#include <vector>
#include <string>
using namespace std;
#include "tdat/BondDistort.H"
#include "tdat/Fragment.H"
#include "tdat/TAtm.H"

static int fails = 0;
static void check(bool ok, const char *what)
{
  printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) fails++;
}
static double dist(Fragment& f, int a, int b)
{
  const double *p = f.atomRef(a)->coordinates(), *q = f.atomRef(b)->coordinates();
  return sqrt((p[0]-q[0])*(p[0]-q[0]) + (p[1]-q[1])*(p[1]-q[1]) + (p[2]-q[2])*(p[2]-q[2]));
}
static Fragment *octahedron(bool aqua)
{
  const double r = 1.6;
  const double dir[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
  vector<string> tags(1, aqua ? "Ti" : "S");
  vector<double> xyz(3, 0.0);
  for (int i = 0; i < 6; i++) {
    tags.push_back(aqua ? "O" : "F");
    for (int k = 0; k < 3; k++) xyz.push_back(r * dir[i][k]);
  }
  if (aqua)
    for (int i = 0; i < 6; i++) {  // one H per O, further out
      tags.push_back("H");
      for (int k = 0; k < 3; k++) xyz.push_back(1.6 * dir[i][k] * 1.6 + 0.3 * (k == 1));
    }
  Fragment *f = new Fragment("oct", tags, &xyz[0]);
  for (int i = 1; i <= 6; i++) f->addBond(0, i, 1.0);
  if (aqua) for (int i = 1; i <= 6; i++) f->addBond(i, i + 6, 1.0);
  return f;
}

int main()
{
  for (int aqua = 0; aqua < 2; aqua++) {
    Fragment *f = octahedron(aqua);
    vector<int> nb = BondDistort::neighbours(*f, 0);
    check(nb.size() == 6, "six neighbours");
    vector<double> d0;
    for (size_t i = 0; i < nb.size(); i++) d0.push_back(dist(*f, 0, nb[i]));
    check(BondDistort::apply(*f, 0, nb, BondDistort::uniform(6, 0.1)), "apply");
    bool ok = true;
    for (size_t i = 0; i < nb.size(); i++)
      ok = ok && fabs(dist(*f, 0, nb[i]) - d0[i] - 0.1) < 1e-6;
    check(ok, "every distance old + 0.1");
    const double *c = f->atomRef(0)->coordinates();
    check(c[0] == 0 && c[1] == 0 && c[2] == 0, "centre stays");
    // Direction: the neighbour is still on its axis.
    const double *o = f->atomRef(nb[0])->coordinates();
    check(fabs(o[1]) < 1e-9 && fabs(o[2]) < 1e-9 && o[0] > 0, "bond direction unchanged");
    if (aqua) {
      // O-H length unchanged: the H travelled with the O.
      Fragment *g = octahedron(true);
      double oh = dist(*g, 1, 7);
      BondDistort::apply(*g, 0, BondDistort::neighbours(*g, 0), BondDistort::uniform(6, 0.1));
      check(fabs(dist(*g, 1, 7) - oh) < 1e-9, "O-H length unchanged");
      delete g;
    }

    Fragment *g = octahedron(aqua);
    vector<double> dr;
    check(BondDistort::jahnTeller(*g, 0, nb, 0.2, "z", dr), "jahnTeller z");
    int axial = 0, eq = 0;
    for (size_t i = 0; i < 6; i++) {
      if (fabs(dr[i] - 0.2) < 1e-12) axial++;
      if (fabs(dr[i] + 0.1) < 1e-12) eq++;
    }
    check(axial == 2 && eq == 4, "two +d, four -d/2");
    check(fabs(dr[4] - 0.2) < 1e-12 && fabs(dr[5] - 0.2) < 1e-12, "the z pair is axial");
    check(!BondDistort::jahnTeller(*g, 0, nb, 0.2, "q", dr), "bad axis refused");
    delete g;
    delete f;
  }
  return fails ? 1 : 0;
}
