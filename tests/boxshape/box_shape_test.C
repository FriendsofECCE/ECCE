// GROMACS box shapes (tdat/BoxShape.H): face counts, and every atom of
// `gmx trjconv -pbc atom -ur compact` output (the fixtures, 400 random
// points each) lies inside the shape centred on (a+b+c)/2 and the points
// fill it, which an independent program decides, not our construction.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "tdat/BoxShape.H"
#include "tdat/LatticeDef.H"

using namespace boxshape;

static int failures = 0;
static void check(bool ok, const std::string& what)
{
  printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) failures++;
}

static void one(const std::string& dir, const char* name, int faces, int verts)
{
  std::ifstream in((dir + "/" + name + ".gro").c_str());
  std::string line;
  std::vector<Vec> xs;
  std::getline(in, line);
  std::getline(in, line);
  int n = atoi(line.c_str());
  for (int i=0; i<n; i++) {
    std::getline(in, line);
    Vec x = {{atof(line.substr(20,8).c_str()), atof(line.substr(28,8).c_str()),
              atof(line.substr(36,8).c_str())}};
    xs.push_back(x);
  }
  std::getline(in, line);
  double v[9];
  std::istringstream bs(line);
  for (int i=0; i<9; i++) bs >> v[i];
  Vec a = {{v[0], v[3], v[4]}}, b = {{v[5], v[1], v[6]}}, c = {{v[7], v[8], v[2]}};
  std::string tag = name;

  check(isGromacsCompactBox(a, b, c), tag + " recognised");
  Vec sq = {{1, 0, 0}};
  check(!isGromacsCompactBox(sq, sq, sq), tag + ": cube not recognised");

  Shape s;
  check(wignerSeitz(a, b, c, s), tag + " shape computed");
  check(s.nfaces == faces, tag + " faces " + std::to_string(s.nfaces));
  check((int)s.vertices.size() == verts, tag + " vertices " + std::to_string(s.vertices.size()));
  // Euler: V - E + F = 2.
  check((int)s.vertices.size() - (int)s.edges.size() + s.nfaces == 2, tag + " Euler");

  // Inside every bisector plane, relative to the centre (a+b+c)/2, and
  // touching them: the nearest plane of the farthest points is reached.
  double worst = -1e9, maxfill = 0;
  for (size_t i=0; i<xs.size(); i++) {
    Vec r;
    for (int k=0; k<3; k++) r[k] = xs[i][k] - 0.5*(a[k]+b[k]+c[k]);
    double fill = 0;
    for (int ii=-1; ii<=1; ii++) for (int j=-1; j<=1; j++) for (int l=-1; l<=1; l++) {
      if (!ii && !j && !l) continue;
      double p[3];
      for (int k=0; k<3; k++) p[k] = ii*a[k]+j*b[k]+l*c[k];
      double pp = p[0]*p[0]+p[1]*p[1]+p[2]*p[2];
      double t = (p[0]*r[0]+p[1]*r[1]+p[2]*r[2]) / (0.5*pp);  // 1 = on the plane
      if (t > worst) worst = t;
      if (t > fill) fill = t;
    }
    if (fill > maxfill) maxfill = fill;
  }
  check(worst < 1.002, tag + " all atoms inside the cell, max plane ratio " + std::to_string(worst));
  check(maxfill > 0.95, tag + " atoms reach the faces " + std::to_string(maxfill));

  // The edges must bound the same region: each vertex has |p.v| <= |p|^2/2.
  double vw = -1e9;
  for (size_t i=0; i<s.vertices.size(); i++)
    for (int ii=-1; ii<=1; ii++) for (int j=-1; j<=1; j++) for (int l=-1; l<=1; l++) {
      if (!ii && !j && !l) continue;
      double p[3];
      for (int k=0; k<3; k++) p[k] = ii*a[k]+j*b[k]+l*c[k];
      double pp = p[0]*p[0]+p[1]*p[1]+p[2]*p[2];
      double t = (p[0]*s.vertices[i][0]+p[1]*s.vertices[i][1]+p[2]*s.vertices[i][2])/(0.5*pp);
      if (t > vw) vw = t;
    }
  check(vw < 1.0001, tag + " vertices inside");
}


// A cell given by its six parameters, as a PDB CRYST1 or the Periodic
// Builder gives it, must reproduce the box vectors GROMACS itself wrote
// (the last line of a Prepare .gro, x10 to Angstrom) and so be recognised.
static void fromParameters()
{
  LatticeDef l(43.923, 43.923, 43.923, 60.0, 60.0, 90.0);
  std::vector<MPoint>* v = l.toVectors();
  const double want[3][3] = {{43.9229, 0, 0}, {0, 43.9229, 0},
                             {21.9615, 21.9615, 31.0582}};
  bool same = true;
  for (int i = 0; i < 3; i++) {
    const double got[3] = {(*v)[i].x(), (*v)[i].y(), (*v)[i].z()};
    for (int k = 0; k < 3; k++)
      if (std::fabs(got[k] - want[i][k]) > 0.01) same = false;
  }
  check(same, "a cell from its parameters has GROMACS's box vectors");
  Vec a = {{(*v)[0].x(), (*v)[0].y(), (*v)[0].z()}};
  Vec b = {{(*v)[1].x(), (*v)[1].y(), (*v)[1].z()}};
  Vec c = {{(*v)[2].x(), (*v)[2].y(), (*v)[2].z()}};
  check(isGromacsCompactBox(a, b, c), "and is drawn as the dodecahedron");
  delete v;

  LatticeDef h(3.0, 3.0, 5.0, 90.0, 90.0, 120.0);
  v = h.toVectors();
  const double la = (*v)[0].length(), lb = (*v)[1].length();
  const double g = std::acos((*v)[0].dotProduct((*v)[1]) / (la * lb)) * 180 / M_PI;
  check(std::fabs(la - 3.0) < 1e-6 && std::fabs(lb - 3.0) < 1e-6 &&
        std::fabs(g - 120.0) < 1e-6, "a hexagonal cell keeps its lengths and gamma");
  delete v;
}

int main(int argc, char** argv)
{
  std::string dir = argc > 1 ? argv[1] : "tests/boxshape/fixtures";
  one(dir, "dodsq", 12, 14);
  one(dir, "dodhex", 12, 14);
  one(dir, "to", 14, 24);
  fromParameters();
  return failures ? 1 : 0;
}
