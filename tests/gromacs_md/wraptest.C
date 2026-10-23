// GromacsStructure::wrapCompact (#476): a solvated dodecahedron from gmx
// solvate (a brick of waters) comes out with every molecule whole and its
// centre inside the compact cell, each atom moved only by lattice vectors.
// The cell test is brute force over lattice neighbours: a point is inside
// the Wigner-Seitz cell when no lattice translate of the centre is nearer.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "dsm/GromacsStructure.H"

using std::string;
using std::vector;

static int failures = 0;
static void check(bool ok, const string& what)
{
  printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) failures++;
}

struct Gro {
  vector<string> res, name;
  vector<double> x;
  double box[9];
};

static Gro parse(const string& text)
{
  Gro g;
  std::istringstream in(text);
  string l;
  std::getline(in, l);
  std::getline(in, l);
  int n = atoi(l.c_str());
  for (int i = 0; i < n; i++) {
    std::getline(in, l);
    g.res.push_back(l.substr(0, 10));
    g.name.push_back(l.substr(10, 5));
    for (int k = 0; k < 3; k++) g.x.push_back(atof(l.substr(20 + 8*k, 8).c_str()));
  }
  std::getline(in, l);
  for (int i = 0; i < 9; i++) g.box[i] = 0;
  sscanf(l.c_str(), "%lf %lf %lf %lf %lf %lf %lf %lf %lf", &g.box[0], &g.box[1],
         &g.box[2], &g.box[3], &g.box[4], &g.box[5], &g.box[6], &g.box[7], &g.box[8]);
  return g;
}

static void lattice(const Gro& g, double a[3][3])
{
  double v[3][3] = { { g.box[0], g.box[3], g.box[4] },
                     { g.box[5], g.box[1], g.box[6] },
                     { g.box[7], g.box[8], g.box[2] } };
  for (int i = 0; i < 3; i++) for (int k = 0; k < 3; k++) a[i][k] = v[i][k];
}

static bool inCell(const double a[3][3], const double* p)
{
  double c[3];
  for (int k = 0; k < 3; k++) c[k] = 0.5 * (a[0][k] + a[1][k] + a[2][k]);
  double d0 = 0;
  for (int k = 0; k < 3; k++) d0 += (p[k] - c[k]) * (p[k] - c[k]);
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

// Fractional coordinates differ by whole numbers: only lattice moves.
static bool latticeShift(const double a[3][3], const double* p, const double* q)
{
  double d[3] = { q[0]-p[0], q[1]-p[1], q[2]-p[2] };
  double f[3];
  f[2] = d[2] / a[2][2];
  f[1] = (d[1] - f[2]*a[2][1]) / a[1][1];
  f[0] = (d[0] - f[1]*a[1][0] - f[2]*a[2][0]) / a[0][0];
  for (int k = 0; k < 3; k++)
    if (std::fabs(f[k] - std::floor(f[k] + 0.5)) > 0.01) return false;
  return true;
}

static double dist(const Gro& g, int i, int j)
{
  double s = 0;
  for (int k = 0; k < 3; k++) {
    double d = g.x[3*i+k] - g.x[3*j+k];
    s += d * d;
  }
  return std::sqrt(s);
}

int main(int argc, char** argv)
{
  if (argc < 2) return 2;
  std::ifstream f(argv[1]);
  std::stringstream ss;
  ss << f.rdbuf();
  const string before = ss.str();
  const string after = GromacsStructure::wrapCompact(before);
  Gro g0 = parse(before), g1 = parse(after);
  double a[3][3];
  lattice(g0, a);
  const int n = (int)g0.res.size();
  check(n > 100 && g1.res.size() == g0.res.size(), "atom count kept");

  int outside0 = 0, outside1 = 0, moved = 0, torn = 0, notLattice = 0;
  for (int m = 0; m + 2 < n; m += 3) {
    double c0[3] = {0,0,0}, c1[3] = {0,0,0};
    for (int i = m; i < m + 3; i++)
      for (int k = 0; k < 3; k++) {
        c0[k] += g0.x[3*i+k] / 3;
        c1[k] += g1.x[3*i+k] / 3;
        if (!latticeShift(a, &g0.x[3*i], &g1.x[3*i])) { notLattice++; break; }
      }
    if (!inCell(a, c0)) outside0++;
    if (!inCell(a, c1)) outside1++;
    if (std::fabs(c0[0]-c1[0]) > 1e-3 || std::fabs(c0[1]-c1[1]) > 1e-3 ||
        std::fabs(c0[2]-c1[2]) > 1e-3) moved++;
    if (dist(g1, m, m + 1) > 0.11 || dist(g1, m, m + 2) > 0.11) torn++;
  }
  printf("molecules outside the cell: %d before, %d after; moved %d\n",
         outside0, outside1, moved);
  check(outside0 > 10, "the brick has waters outside the compact cell");
  check(outside1 == 0, "every molecule centre inside the compact cell");
  check(torn == 0, "every water whole");
  check(notLattice == 0, "atoms moved by lattice vectors only");

  // A two-residue molecule (peptide-like C-N bond) across the cell edge
  // must stay together; a rectangular box is left alone.
  const string chain =
      "t\n    4\n"
      "    1ALA      N    1   0.950   0.100   0.100\n"
      "    1ALA      C    2   0.080   0.100   0.100\n"
      "    2GLY      N    3   0.213   0.100   0.100\n"
      "    2GLY     CA    4   0.340   0.100   0.100\n"
      "   1.00000   1.00000   1.00000   0.00000   0.00000   0.00000"
      "   0.00000   0.50000   0.50000\n";
  Gro c = parse(GromacsStructure::wrapCompact(chain));
  check(dist(c, 1, 2) < 0.2 && dist(c, 0, 3) < 0.5, "bonded residues stay whole");
  string rect = chain.substr(0, chain.rfind("   1.00000")) +
                "   1.00000   1.00000   1.00000\n";
  check(GromacsStructure::wrapCompact(rect) == rect, "rectangular box unchanged");

  return failures ? 1 : 0;
}
