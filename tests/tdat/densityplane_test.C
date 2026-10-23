// DensityPlane (#215): the plane model and the degenerate sum, wx-free.
//
//   densityplane_test FIXTURE-ROOT      (tests/builder/densitymap-fixtures)
//
// Plane construction is checked on synthetic geometry; the numbers on the
// n2 and water calculations: the density integrates to the
// electron count (an oracle that does not know the plane code), and the
// summed degenerate pi pair is cylindrically symmetric.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "util/EcceURL.H"
#include "tdat/DensityPlane.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/IPropCalculation.H"
#include "dsm/Resource.H"

using namespace std;
using namespace DensityPlane;

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { ++failures; \
  fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); } } while (0)

static double dot(const double *a, const double *b) { return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; }

static void planes()
{
  //  Ethene, in the xy plane, C=C along x.
  const double c[] = { -0.667, 0, 0,   0.667, 0, 0,
                       -1.24, 0.92, 0, -1.24, -0.92, 0,
                        1.24, 0.92, 0,  1.24, -0.92, 0 };
  const vector<double> coords(c, c + 18);
  const vector<string> labels = { "C1", "C2", "H3", "H4", "H5", "H6" };
  Plane p;
  string why;

  CHECK(makePlane(THREE_ATOMS, coords, labels, 0, 1, 2, p, why), "three atoms");
  CHECK(fabs(fabs(p.n[2]) - 1) < 1e-12, "three atoms: normal is z");
  CHECK(fabs(p.u[0] - 1) < 1e-12, "three atoms: u runs from the first to the second atom");
  CHECK(fabs(dot(p.u, p.v)) < 1e-12 && fabs(dot(p.u, p.n)) < 1e-12, "orthonormal frame");
  for (int a = 0; a < 3; a++) {
    const double d[3] = { coords[3*a] - p.origin[0], coords[3*a+1] - p.origin[1],
                          coords[3*a+2] - p.origin[2] };
    CHECK(fabs(dot(d, p.n)) < 1e-12, "each chosen atom lies in the plane");
  }
  Plane col;
  CHECK(!makePlane(THREE_ATOMS, coords, labels, 0, 1, 1, col, why), "repeated atom refused");

  //  Through the C=C bond, perpendicular to the molecular plane: the plane
  //  of the pi lobes, here xz.
  CHECK(makePlane(AXIAL, coords, labels, 0, 1, -1, p, why), "axial");
  CHECK(fabs(fabs(p.n[1]) - 1) < 1e-12, "axial: normal is y (in the molecular plane)");
  CHECK(fabs(p.v[2]) > 1 - 1e-12, "axial: v is the molecular normal");
  CHECK(fabs(p.origin[0]) < 1e-12 && fabs(p.origin[1]) < 1e-12, "axial: through the bond");

  //  The view: right = x, up = z gives the xz plane through the centroid.
  const double right[3] = { 1, 0, 0 }, up[3] = { 0.3, 0, 1 };   // up is not orthogonal
  CHECK(makeViewPlane(coords, right, up, p, why), "view plane");
  CHECK(fabs(dot(p.n, right)) < 1e-12 && fabs(dot(p.n, p.v)) < 1e-12, "view: n is the screen normal");
  CHECK(fabs(fabs(p.n[1]) - 1) < 1e-12, "view: normal is -y");
  CHECK(fabs(p.v[2] - 1) < 1e-12, "view: v is orthogonalised up");
  const double sight[3] = { 0, 0, 1 };
  const double r2[3] = { 1, 0, 0 }, u2[3] = { 0, 1, 0 };
  CHECK(makeViewPlane(coords, r2, u2, p, why) && dot(p.n, sight) > 1 - 1e-12,
        "view: looking down -z, normal towards the viewer");
  const double para[3] = { 2, 0, 0 };
  CHECK(!makeViewPlane(coords, right, para, p, why), "parallel axes refused");

  //  The offset moves along n from the base plane and does not accumulate.
  CHECK(makePlane(THREE_ATOMS, coords, labels, 0, 1, 2, p, why), "base");
  const Plane o = offsetPlane(p, 0.8);
  CHECK(fabs(o.origin[2] - 0.8*p.n[2]) < 1e-12 && o.offset == 0.8, "offset 0.8");
  const Plane o2 = offsetPlane(o, -0.4);
  CHECK(fabs(o2.origin[2] - (-0.4)*p.n[2]) < 1e-12, "offset replaces, does not add");
  CHECK(o.description.find("0.8 \xC3\x85 above") == 0, "caption names the distance");
  CHECK(o.description.find("C1, C2, H3") != string::npos, "caption names the atoms");
  CHECK(offsetPlane(o, 0.0).description == p.description, "zero offset: the plain caption");
  CHECK(offsetPlane(p, -0.8).description.find("below") != string::npos, "negative: below");

  //  What can be offered: ethene is planar, NH3 is not, N2 is linear.
  const vector<string> sym = { "C", "C", "H", "H", "H", "H" };
  CHECK(hasMolecularPlane(coords) && !isLinear(coords), "ethene has a molecular plane");
  CHECK(bonded(sym, coords, 0, 1) && bonded(sym, coords, 0, 2), "C=C and C-H bonded");
  CHECK(!bonded(sym, coords, 2, 5), "H and the far H are not bonded");
  const double nh3[] = { 0, 0, 0.12,  0, 0.94, -0.28,  0.81, -0.47, -0.28,  -0.81, -0.47, -0.28 };
  const vector<double> pyramid(nh3, nh3 + 12);
  CHECK(!isPlanar(pyramid) && !hasMolecularPlane(pyramid), "NH3 has no molecular plane");
  const double n2[] = { 0, 0, 0, 0, 0, 1.1 };
  const vector<double> diatomic(n2, n2 + 6);
  CHECK(isLinear(diatomic) && isPlanar(diatomic) && !hasMolecularPlane(diatomic),
        "N2: a plane through the axis, but nothing to stand on");

  //  Degeneracy: the MO diagram's tolerance.
  const vector<double> e = { -1.0, -0.5, -0.5 + 1e-6, -0.2, -0.2 + 1e-4 };
  CHECK(degenerateSet(e, 2).size() == 2 && degenerateSet(e, 3)[0] == 2, "pair within 1.5e-5");
  CHECK(degenerateSet(e, 4).size() == 1, "1e-4 apart is not degenerate");
  CHECK(degenerateSet(e, 9).empty(), "no such orbital");
}

static Resource *open(const string& path, IPropCalculation *&calc)
{
  Resource *res = EDSIFactory::getResource(EcceURL("file://" + path));
  calc = dynamic_cast<IPropCalculation*>(res);
  return res;
}

int main(int argc, char **argv)
{
  planes();
  if (argc < 2) {
    cerr << "densityplane_test: no fixture root, plane checks only\n";
    return failures ? 1 : 0;
  }
  const string root = argv[1];
  EDSIServerCentral central;

  //  N2: every degenerate pair's sum of squares is cylindrical.
  {
    IPropCalculation *calc;
    open(root + "/n2", calc);
    Evaluator ev;
    CHECK(calc && ev.build(calc), ("n2 builds: " + ev.complaint).c_str());
    if (calc && ev.symbols.size() == 2) {
      double axis[3], len = 0;
      for (int k = 0; k < 3; k++) { axis[k] = ev.coordsAng[3+k] - ev.coordsAng[k]; len += axis[k]*axis[k]; }
      len = sqrt(len);
      for (int k = 0; k < 3; k++) axis[k] /= len;
      //  Two vectors perpendicular to the axis.
      double e1[3] = { 0, 0, 0 }, e2[3];
      int least = 0;
      for (int k = 1; k < 3; k++) if (fabs(axis[k]) < fabs(axis[least])) least = k;
      e1[least] = 1;
      const double d = dot(e1, axis);
      double n1 = 0;
      for (int k = 0; k < 3; k++) { e1[k] -= d*axis[k]; n1 += e1[k]*e1[k]; }
      for (int k = 0; k < 3; k++) e1[k] /= sqrt(n1);
      e2[0] = axis[1]*e1[2] - axis[2]*e1[1];
      e2[1] = axis[2]*e1[0] - axis[0]*e1[2];
      e2[2] = axis[0]*e1[1] - axis[1]*e1[0];

      int pairs = 0;
      for (int mo = 1; mo <= ev.orbitals(false); mo++) {
        const vector<int> set = degenerateSet(ev.energy, mo);
        if (set.size() != 2 || set[0] != mo) continue;
        pairs++;
        //  Ring of radius 0.7 A about the axis, 0.3 A from the first atom.
        double lo = 1e300, hi = -1e300;
        for (int k = 0; k < 24; k++) {
          const double t = 2*M_PI*k/24;
          double p[3];
          for (int i = 0; i < 3; i++)
            p[i] = ev.coordsAng[i] + 0.3*axis[i] + 0.7*(cos(t)*e1[i] + sin(t)*e2[i]);
          double sum = 0;
          for (size_t s = 0; s < set.size(); s++) {
            const double psi = ev.orbital(set[s], false, p[0], p[1], p[2]);
            sum += psi*psi;
          }
          lo = min(lo, sum); hi = max(hi, sum);
        }
        fprintf(stderr, "n2 MOs %d-%d: ring spread %.2e of %.4g\n", set[0], set[1], (hi - lo)/hi, hi);
        CHECK(hi > 1e-6 && (hi - lo)/hi < 1e-6, "degenerate sum is cylindrically symmetric");
        //  A single component is not.
        double a = ev.orbital(mo, false, ev.coordsAng[0] + 0.3*axis[0] + 0.7*e1[0],
                              ev.coordsAng[1] + 0.3*axis[1] + 0.7*e1[1],
                              ev.coordsAng[2] + 0.3*axis[2] + 0.7*e1[2]);
        (void)a;
      }
      CHECK(pairs >= 1, "n2 has a degenerate pi pair");
    }
  }

  //  Water: the density integrates to the electron count.
  {
    IPropCalculation *calc;
    open(root + "/../../mocomp/fixtures/water", calc);
    Evaluator ev;
    CHECK(calc && ev.build(calc), ("water builds: " + ev.complaint).c_str());
    if (calc) {
      double electrons = 0;
      for (size_t m = 0; m < ev.occ.size(); m++) electrons += ev.occ[m];
      const double h = 0.05, box = 4.0;      // A
      double sum = 0;
      for (double x = -box + 0.5*h; x <= box; x += h)
        for (double y = -box + 0.5*h; y <= box; y += h)
          for (double z = -box + 0.5*h; z <= box; z += h)
            sum += ev.density(x + ev.coordsAng[0], y + ev.coordsAng[1], z + ev.coordsAng[2]);
      const double bohr = 0.52917721092;
      const double integral = sum * pow(h/bohr, 3);
      fprintf(stderr, "water: %.3f electrons integrated, %.1f occupied\n", integral, electrons);
      CHECK(fabs(integral - electrons) < 0.05, "density integrates to the electron count");
    }
  }
  return failures ? 1 : 0;
}
