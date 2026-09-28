///////////////////////////////////////////////////////////////////////////
// Regression test for issue: ORCA's own def2-SVP shell order for a
// transition metal interleaves a polarization shell among the d shells
// (fault A), and its f(+3)/f(-3) spherical components carry the
// opposite sign from the textbook convention ECCE's MOOrdering used
// (both fixed in ORCA.edml/orca.mo/MoAoOrder). Either bug alone leaves
// every MO with d or f character un-normalised.
//
// fixtures/cro_mo_block.txt is ORCA 6.1.1's real "MOLECULAR ORBITALS"
// block for CrO/def2-SVP (RHF, NoIter). fixtures/cro.nb is the
// corresponding NumericalBasis for Cr+O, extracted from a real stored
// ECCE calculation's basis config (same elements, same def2-SVP).
//
// Runs the real scripts/parsers/orca.mo on the fixture, then MoAoOrder
// + BasisFlatten -- the same path a live ORCA job takes -- and checks
// c^T S c == 1 for every one of the 45 MOs.
///////////////////////////////////////////////////////////////////////////
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#include "tdat/BasisFlatten.H"
#include "tdat/EspField.H"
#include "tdat/TGBSAngFunc.H"
#include "tdat/MoAoOrder.H"
#include "tdat/PropTable.H"
#include "tdat/PropString.H"

#include "dsm/TGBSConfig.H"
#include "dsm/TGaussianBasisSet.H"
#include "dsm/ICalcUtils.H"
#include "dsm/JCode.H"
#include "dsm/CodeFactory.H"
#include "dsm/PropFactory.H"
#include "dsm/IPropCalculation.H"

namespace {

class FakeCalc : public IPropCalculation {
public:
  string p_marker;
  TProperty* getProperty(const string& name) {
    if (name == "MOAOORDER" && !p_marker.empty()) {
      PropString *p = (PropString*)PropFactory::getProperty("MOAOORDER");
      if (p) p->value(p_marker);
      return p;
    }
    return 0;
  }
  vector<string> propertyNames(void) { return vector<string>(); }
  TProperty* updateProperty(const string&, const string&) { return 0; }
  bool putProperty(TProperty*) { return false; }
  bool deleteProperties(void) { return false; }
  void flushPropertyCache(void) { }
  bool fragment(Fragment*) { return false; }
  Fragment* fragment() const { return 0; }
  bool getFragment(Fragment&) { return false; }
  Fragment* getFragmentStep(int) { return 0; }
  Fragment* getFragmentStep(int, PropertyDoc&) { return 0; }
  bool getFragmentStep(Fragment*, int) { return false; }
  long getFragmentModifiedDate() const { return 0; }
  EcceURL getURL() const { return EcceURL(); }
  bool isReadOnly() const { return true; }
  bool promptBeforeSave(Fragment*, string&, string&) { return false; }
};

// Parses scripts/parsers/orca.mo's own "key: MO\nsize:\n...values:...END"
// text format directly -- no round trip through a stored calc.
PropTable* parseMoText(const string& path, const string& key) {
  ifstream in(path);
  string line;
  bool found = false;
  while (getline(in, line)) {
    if (line == "key: " + key) { found = true; break; }
  }
  if (!found) return 0;
  getline(in, line);  // "size:"
  getline(in, line);
  int nmo, nbas;
  { istringstream iss(line); iss >> nmo >> nbas; }
  while (getline(in, line) && line != "values:") { }
  vector<double> vals;
  while ((int)vals.size() < nmo*nbas && getline(in, line)) {
    istringstream iss(line);
    double v;
    while (iss >> v) vals.push_back(v);
  }
  PropTable *t = (PropTable*)PropFactory::getProperty(key);
  if (!t) return 0;
  t->values(nmo, nbas, vals);
  return t;
}

int failures = 0;

void check(bool cond, const char *what) {
  printf("%s: %s\n", cond ? "PASS" : "FAIL", what);
  if (!cond) failures++;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: testCrOMoAoOrder <orca.mo output file>\n");
    return 2;
  }
  const string moPath = argv[1];

  const JCode* code = CodeFactory::lookup("ORCA");
  check(code != 0, "ORCA registered");
  if (code == 0) { printf("FAIL\n"); return 1; }

  ifstream nb("fixtures/cro.nb");
  TGBSConfig* cfg = ICalcUtils::importConfig(nb);
  check(cfg != 0, "CrO def2-SVP NumericalBasis fixture parsed");

  ifstream xyz("fixtures/cro.xyz");
  vector<string> el; vector<double> coordsAng; string s; double x, y, z;
  while (xyz >> s >> x >> y >> z) {
    el.push_back(s);
    coordsAng.push_back(x); coordsAng.push_back(y); coordsAng.push_back(z);
  }
  check(el.size() == 2, "CrO fixture has 2 atoms");

  TGBSAngFunc* af = code->getAngFunc(TGaussianBasisSet::Spherical);
  int lenS[7] = {1,3,5,7,9,11,13};
  vector<EspBasisFunction> basis;
  bool flattened = cfg && BasisFlatten::flatten(el, coordsAng, cfg, code, af,
                                                af->maxShells(), lenS, basis);
  check(flattened, "basis flattened");

  PropTable *moTable = parseMoText(moPath, "MO");
  check(moTable != 0, "orca.mo's MO block parsed");
  if (moTable == 0 || !flattened) { printf("FAIL\n"); return 1; }

  check(moTable->columns() == (int)basis.size(),
        "canonical-order MO width matches ECCE's flattened basis width");

  FakeCalc fake;
  fake.p_marker = "angular-momentum";
  PropTable *reordered = MoAoOrder::reorderToNative(moTable, &fake, el, cfg);
  bool owned = (reordered != moTable);
  check(owned, "MoAoOrder actually reordered (marker recognised)");

  double maxErr = 0;
  int nbad = 0;
  for (int m = 0; m < reordered->rows(); m++) {
    double n = 0;
    for (size_t i = 0; i < basis.size(); i++) {
      double ci = reordered->value(m, i);
      if (ci == 0.0) continue;
      for (size_t j = 0; j < basis.size(); j++) {
        double cj = reordered->value(m, j);
        if (cj == 0.0) continue;
        n += ci * cj * EspField::overlapOf(basis[i], basis[j]);
      }
    }
    double err = fabs(n - 1.0);
    if (err > maxErr) maxErr = err;
    if (err > 1.0e-3) nbad++;
  }
  printf("max|c^T S c - 1| = %.3e over %d MOs\n", maxErr, reordered->rows());
  check(maxErr < 1.0e-4, "every CrO MO is normalised (fault A + f(+-3) sign)");
  check(nbad == 0, "no MO off by more than 1e-3");

  if (owned) delete reordered;

  printf("%s\n", failures ? "FAIL" : "PASS");
  return failures ? 1 : 0;
}
