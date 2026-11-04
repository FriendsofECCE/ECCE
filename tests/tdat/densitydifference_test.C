// Density difference (#726), wx-free.
//
//   densitydifference_test FIXTURE-ROOT   (tests/builder/difference-fixtures)
//
// H2 against its two hydrogen atoms at the same positions, same basis: the
// answer is known in sign (density builds up between the nuclei) and in
// total (the integral is N(complex) - sum N(fragments) = 0).
#include <cmath>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

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

static shared_ptr<Evaluator> load(const string& dir)
{
  Resource *res = EDSIFactory::getResource(EcceURL("file://" + dir));
  IPropCalculation *calc = dynamic_cast<IPropCalculation*>(res);
  shared_ptr<Evaluator> ev(new Evaluator);
  if (!calc || !ev->build(calc)) {
    cerr << dir << ": " << (calc ? ev->complaint : "not a calculation") << endl;
    exit(1);
  }
  return ev;
}

int main(int argc, char **argv)
{
  if (argc < 2) { cerr << "usage: densitydifference_test FIXTURE-ROOT\n"; return 2; }
  string root = argv[1];
  if (root[0] != '/') { char b[4096]; if (getcwd(b, sizeof b)) root = string(b) + "/" + root; }
  EDSIServerCentral central;

  shared_ptr<Evaluator> whole = load(root + "/h2");
  shared_ptr<Evaluator> a = load(root + "/h2-a"), b = load(root + "/h2-b");

  vector<char> used;
  string why;
  CHECK(matchFragment(*whole, *a, used, why), "first hydrogen matches");
  CHECK(matchFragment(*whole, *b, used, why), "second hydrogen matches");
  CHECK(!matchFragment(*whole, *a, used, why) && !why.empty(), "an atom in two fragments is refused");
  vector<char> fresh;
  CHECK(!matchFragment(*a, *whole, fresh, why) && !why.empty(), "a fragment larger than the complex is refused");

  double nWhole, spin, nDiff;
  whole->integrate(0.1, 5.0, nWhole, spin);
  CHECK(fabs(nWhole - 2.0) < 0.02, "H2 integrates to 2 electrons");

  whole->addFragment(a, "H1");
  whole->addFragment(b, "H2");
  CHECK(whole->isDifference(), "difference mode on");
  whole->integrate(0.1, 5.0, nDiff, spin);
  printf("N(H2) = %.4f, integral of the difference = %.5f\n", nWhole, nDiff);
  CHECK(fabs(nDiff) < 0.01, "difference integrates to N(complex) - sum N(fragments) = 0");

  const double mid = whole->density(0, 0, 0);
  const double behind = whole->density(0, 0, 1.2);
  printf("d(rho) in the bond middle %.5f, 0.83 A behind a nucleus %.5f e/bohr^3\n", mid, behind);
  CHECK(mid > 1e-3, "density builds up between the nuclei");
  CHECK(behind < 0, "density is lost behind the nuclei");

  //  The one-pass evaluator used by the profiles agrees with density().
  vector<char> none;
  vector<double> pa, pb, phi;
  double rho;
  whole->evaluateAt(0.1, 0.2, 0.3, none, none, true, pa, pb, phi, rho);
  CHECK(fabs(rho - whole->density(0.1, 0.2, 0.3)) < 1e-12, "evaluateAt matches density in difference mode");

  Plane p;
  vector<string> labels = whole->atomLabels();
  CHECK(makePlane(MOLECULAR, whole->coordsAng, labels, 0, 1, -1, p, why), "plane");
  Map m;
  CHECK(whole->map(p, DENSITY, 0, false, 0.1, 1.5, m), "map");
  CHECK(m.zmin < 0 && m.zmax > 0, "the map has both signs");
  CHECK(m.title.find("difference") != string::npos, "the title says difference");

  whole->clearFragments();
  CHECK(!whole->isDifference() && fabs(whole->density(0, 0, 0) - mid) > 1e-3, "fragments removed");

  if (failures) { fprintf(stderr, "%d failure(s)\n", failures); return 1; }
  printf("ok\n");
  return 0;
}
