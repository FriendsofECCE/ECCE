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
#include "tdat/MoBasisChange.H"
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
  // Optional: a second fixture (NumericalBasis, xyz, electron count) for
  // which C^T S C must be the unit matrix and trace(P S) = N.
  const string nbPath  = argc > 2 ? argv[2] : "fixtures/cro.nb";
  const string xyzPath = argc > 3 ? argv[3] : "fixtures/cro.xyz";
  const int nElec = argc > 4 ? atoi(argv[4]) : 0;
  // The code whose MOOrdering is under test.  Only ORCA's parser writes
  // canonical (angular-momentum sorted) columns; the others store ECCE's
  // native order, so there is nothing to un-permute.
  const string codeName = argc > 5 ? argv[5] : "ORCA";
  const bool unpermute = (codeName == "ORCA");
  // NWChem's ecce_print writes coefficients to 3 decimals; ORCA and the
  // old fixtures are good to 1e-5.
  const double tol = argc > 6 ? atof(argv[6]) : 1.0e-4;
  // Optional: the basis the code itself used (Gaussian's GFInput echo);
  // the MOs are projected from it onto the input basis before checking.
  const string codeBasisPath = argc > 7 ? argv[7] : "";

  const JCode* code = CodeFactory::lookup(codeName.c_str());
  check(code != 0, (codeName + " registered").c_str());
  if (code == 0) { printf("FAIL\n"); return 1; }

  ifstream nb(nbPath.c_str());
  TGBSConfig* cfg = ICalcUtils::importConfig(nb);
  check(cfg != 0, "CrO def2-SVP NumericalBasis fixture parsed");

  ifstream xyz(xyzPath.c_str());
  vector<string> el; vector<double> coordsAng; string s; double x, y, z;
  while (xyz >> s >> x >> y >> z) {
    el.push_back(s);
    coordsAng.push_back(x); coordsAng.push_back(y); coordsAng.push_back(z);
  }
  check(el.size() >= 1, "fixture has atoms");

  TGBSAngFunc* af = code->getAngFunc(TGaussianBasisSet::Spherical);
  int lenS[7] = {1,3,5,7,9,11,13};
  vector<EspBasisFunction> basis;
  bool flattened = cfg && BasisFlatten::flatten(el, coordsAng, cfg, code, af,
                                                af->maxShells(), lenS, basis);
  check(flattened, "basis flattened");

  PropTable *moTable = parseMoText(moPath, "MO");
  check(moTable != 0, "orca.mo's MO block parsed");
  if (moTable == 0 || !flattened) { printf("FAIL\n"); return 1; }

  printf("MO width %d, flattened basis %d\n", moTable->columns(), (int)basis.size());
  check(moTable->columns() == (int)basis.size(),
        "canonical-order MO width matches ECCE's flattened basis width");

  FakeCalc fake;
  fake.p_marker = "angular-momentum";
  PropTable *reordered = unpermute
      ? MoAoOrder::reorderToNative(moTable, &fake, el, cfg) : moTable;
  if (!codeBasisPath.empty()) {
    ifstream cnb(codeBasisPath.c_str());
    TGBSConfig* ccfg = ICalcUtils::importConfig(cnb);
    vector<EspBasisFunction> codeBasis;
    bool ok = ccfg && BasisFlatten::flatten(el, coordsAng, ccfg, code, af,
                                            af->maxShells(), lenS, codeBasis);
    check(ok, "code's own basis flattened");
    if (ok) {
      reordered = MoBasisChange::project(moTable, codeBasis, basis);
      check(reordered != moTable, "MOs projected onto the input basis");
    }
  }
  bool owned = (reordered != moTable);
  if (unpermute)
    check(owned, "MoAoOrder actually reordered (marker recognised)");

  double maxErr = 0;
  int nbad = 0;
  vector<double> S;
  if (nElec > 0) {
    size_t nb = basis.size();
    S.assign(nb*nb, 0.0);
    for (size_t i = 0; i < nb; i++)
      for (size_t j = 0; j < nb; j++)
        S[i*nb+j] = EspField::overlapOf(basis[i], basis[j]);
  }
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
    if (err > tol*10) nbad++;
  }
  printf("max|c^T S c - 1| = %.3e over %d MOs\n", maxErr, reordered->rows());
  if (nElec > 0) {
    // Closed shell: the first nElec/2 MOs are doubly occupied.
    int nocc = nElec/2, nb = (int)basis.size();
    double worstOff = 0, trPS = 0;
    for (int a = 0; a < nocc; a++)
      for (int b = 0; b <= a; b++) {
        double v = 0;
        for (int i = 0; i < nb; i++) {
          double ci = reordered->value(a, i);
          if (ci == 0.0) continue;
          for (int j = 0; j < nb; j++) v += ci * S[(size_t)i*nb+j] * reordered->value(b, j);
        }
        if (a == b) trPS += 2*v;
        else if (fabs(v) > worstOff) worstOff = fabs(v);
      }
    printf("occupied: max|offdiag C^T S C| = %.3e, trace(PS) = %.6f (N = %d)\n",
           worstOff, trPS, nElec);
    check(worstOff < tol, "occupied MOs orthogonal");
    check(fabs(trPS - nElec) < tol*10, "trace(PS) = number of electrons");
  }
  check(maxErr < tol*10, "every MO is normalised (fault A, f/g/h signs)");
  check(nbad == 0, "no MO off by more than 10*tol");

  if (owned) delete reordered;

  printf("%s\n", failures ? "FAIL" : "PASS");
  return failures ? 1 : 0;
}
