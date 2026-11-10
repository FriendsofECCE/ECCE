// results.h5 written from what ECCE parsed (#690): the Props -> h5 conversion,
// its checks, and that the Props path and the file path give one density.
//
//   results_calc_test fixture <calc folder>...   stored calculations (copies:
//        results.h5 is stored in them); full round trip through the data store
//   results_calc_test orca <orca.mo output> <nb> <xyz> <nelectrons>
//   results_calc_test xtb <molden.input> <xtb.mo output> <nelectrons>
//   results_calc_test emptyspin                  H2+ with an empty beta channel
//   results_calc_test views <calc folder>[=keys]... the vibration and trace
//        properties the views read (ResultsFromCalc::property) against the
//        Props; `keys` (comma-separated) must come from the file
//
// The calculation modes compare Evaluator::density, ::spinDensity and
// every occupied orbital at a grid of points between ECCE_RESULTS_H5=0 (the
// Props) and the stored file, to 1e-8 relative.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>
using namespace std;

#include "util/EcceURL.H"
#include "tdat/BasisFlatten.H"
#include "tdat/DensityPlane.H"
#include "tdat/EspField.H"
#include "tdat/MoAoBasis.H"
#include "tdat/MoAoOrder.H"
#include "dsm/PropFactory.H"
#include "tdat/PropString.H"
#include "tdat/PropTable.H"
#include "tdat/PropTSVecTable.H"
#include "tdat/PropVecTable.H"
#include "tdat/PropVector.H"
#include "tdat/ResultsFromCalc.H"
#include "tdat/TAtm.H"
#include "tdat/TGBSAngFunc.H"
#include "dsm/CodeFactory.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/ICalcUtils.H"
#include "dsm/IPropCalculation.H"
#include "dsm/JCode.H"
#include "dsm/Resource.H"
#include "dsm/TGBSConfig.H"
#include "dsm/TGaussianBasisSet.H"

using DensityPlane::Evaluator;

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { ++failures; \
  printf("FAIL: %s\n", string(msg).c_str()); } else printf("ok:   %s\n", string(msg).c_str()); } while (0)

static long fsize(const string& p) { struct stat st; return stat(p.c_str(), &st) ? -1 : long(st.st_size); }

static vector<double> gridPoints(const vector<double>& xyz)
{
  double lo[3] = {1e9,1e9,1e9}, hi[3] = {-1e9,-1e9,-1e9};
  for (size_t a = 0; a < xyz.size()/3; a++)
    for (int k = 0; k < 3; k++) { lo[k] = min(lo[k], xyz[3*a+k]); hi[k] = max(hi[k], xyz[3*a+k]); }
  vector<double> p;
  const int n = 4;
  for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) for (int k = 0; k < n; k++) {
    // off the symmetry planes so permutations and phases show
    p.push_back(lo[0] - 1.3 + (hi[0]-lo[0]+2.6)*(i + 0.37)/n);
    p.push_back(lo[1] - 1.3 + (hi[1]-lo[1]+2.6)*(j + 0.21)/n);
    p.push_back(lo[2] - 1.3 + (hi[2]-lo[2]+2.6)*(k + 0.43)/n);
  }
  return p;
}

static double worstRel(const vector<double>& a, const vector<double>& b)
{
  double w = 0;
  if (a.size() != b.size()) return 1e30;
  for (size_t i = 0; i < a.size(); i++) {
    const double scale = max(fabs(a[i]), fabs(b[i]));
    if (scale < 1e-13) continue;
    w = max(w, fabs(a[i] - b[i])/scale);
  }
  return w;
}

// ---- stored calculations ----------------------------------------------------

static void fixture(string path)
{
  //  "failed:<folder>": the orbitals cannot pass the checks (coefficients
  //  stored to 3 digits); the file is written marked and the views skip it.
  bool expectFailed = false;
  if (path.compare(0, 7, "failed:") == 0) { expectFailed = true; path = path.substr(7); }
  printf("== %s%s\n", path.c_str(), expectFailed ? " (expected to fail its checks)" : "");
  Resource *res = EDSIFactory::getResource(EcceURL("file://" + path));
  IPropCalculation *calc = dynamic_cast<IPropCalculation*>(res);
  CHECK(calc != 0, "opens as a calculation");
  if (!calc) return;

  string line;
  setenv("ECCE_RESULTS_H5", "1", 1);
  bool stored = ResultsFromCalc::store(calc, line);
  printf("  %s\n", line.c_str());
  CHECK(stored, "results.h5 stored");
  if (!stored) return;
  if (expectFailed) {
    CHECK(line.find("FAILED") != string::npos, "checks failed and said so: " + line);
    CHECK(ResultsFromCalc::stored(calc) == 0, "a file marked failed is not read");
    setenv("ECCE_RESULTS_H5", "1", 1);
    Evaluator ev;
    CHECK(ev.build(calc) && !ev.fromResults, "the Density Map falls back to the properties");
    return;
  }
  CHECK(line.find("checks passed") != string::npos, "checks passed: " + line);

  string h5 = path + "/Outputs/results.h5";
  const long size = fsize(h5);
  long sources = 0;
  const char *props[] = {"MO", "MOBETA", "ORBENG", "ORBENGBETA", "ORBOCC", "ORBOCCBETA",
                         "TE", "MULLIKEN", "DIPOLE", "EGRADVEC", "VIBFREQ", "VIBIR",
                         "VIB", "VIBRAM", "GEOMTRACE", "TEVEC", "SCANGEOM", "SCANENERGY",
                         "SCANCOORD", "IRCGEOM", "IRCENERGY", "IRCCOORD", "TIMEVEC", "LATTICEVEC"};
  for (size_t i = 0; i < sizeof props/sizeof *props; i++) {
    long s = fsize(path + "/Props/" + props[i]);
    if (s > 0) sources += s;
  }
  printf("  results.h5 %ld bytes, the Props it was made from %ld bytes\n", size, sources);
  std::shared_ptr<const resultsh5::Model> m = ResultsFromCalc::stored(calc);
  CHECK(m != 0, "the stored file is read back unmarked");
  //  4 kB is HDF5's fixed cost for the orbital file; each further group
  //  (gradient, charges, dipole, vibrations, traces) costs about 0.25 kB more, which
  //  only a 3-atom file notices.
  long groups = m ? long(m->gradient.size() + m->charges.size() + m->multipoles.size() + m->vib.size() +
                                m->trace.size()) : 0;
  CHECK(size > 0 && size <= sources + 4096 + 256*groups,
        "results.h5 is not larger than its sources plus 4 kB plus 0.25 kB per extra group (" +
        std::to_string(groups) + ")");

  // The Props path, the file path.
  setenv("ECCE_RESULTS_H5", "0", 1);
  Evaluator props_ev;
  bool a = props_ev.build(calc);
  setenv("ECCE_RESULTS_H5", "1", 1);
  Evaluator file_ev;
  bool b = file_ev.build(calc);
  CHECK(a && !props_ev.fromResults, "Props path builds and reads the Props");
  CHECK(b && file_ev.fromResults, "file path builds and reads results.h5");
  if (!a || !b) return;

  const vector<double> pts = gridPoints(props_ev.coordsAng);
  vector<double> rhoP, rhoF, spinP, spinF, orbP, orbF;
  for (size_t k = 0; k + 2 < pts.size(); k += 3) {
    rhoP.push_back(props_ev.density(pts[k], pts[k+1], pts[k+2]));
    rhoF.push_back(file_ev.density(pts[k], pts[k+1], pts[k+2]));
    spinP.push_back(props_ev.spinDensity(pts[k], pts[k+1], pts[k+2]));
    spinF.push_back(file_ev.spinDensity(pts[k], pts[k+1], pts[k+2]));
    for (int beta = 0; beta < 2; beta++) {
      const Evaluator& e = props_ev;
      for (int mo = 1; mo <= e.orbitals(beta != 0); mo++) {
        if ((beta ? e.occBeta : e.occ)[mo-1] <= 0) continue;
        orbP.push_back(props_ev.orbital(mo, beta != 0, pts[k], pts[k+1], pts[k+2]));
        orbF.push_back(file_ev.orbital(mo, beta != 0, pts[k], pts[k+1], pts[k+2]));
      }
    }
  }
  const double dr = worstRel(rhoP, rhoF), ds = worstRel(spinP, spinF);
  // An orbital changes sign through a node: compare against the orbital scale there.
  double orbWorst = 0, orbScale = 0;
  for (size_t i = 0; i < orbP.size(); i++) orbScale = max(orbScale, fabs(orbP[i]));
  for (size_t i = 0; i < orbP.size() && i < orbF.size(); i++) orbWorst = max(orbWorst, fabs(orbP[i] - orbF[i]));
  printf("  density: %zu points, max relative difference %.2e; spin density %.2e; orbitals max abs %.2e (scale %.2e)\n",
         rhoP.size(), dr, ds, orbWorst, orbScale);
  CHECK(dr <= 1e-8, "density agrees to 1e-8 relative between the two paths");
  CHECK(ds <= 1e-8, "spin density agrees to 1e-8 relative");
  CHECK(orbWorst <= 1e-8*orbScale, "every occupied orbital agrees (1e-8 of the largest)");
  CHECK(props_ev.energy == file_ev.energy && props_ev.occ == file_ev.occ, "orbital energies and occupations agree");

  // marked files are ignored
  resultsh5::Model bad = *m;
  bad.checks = "failed: test";
  string err;
  CHECK(resultsh5::writeFile(h5, bad, err), "a marked file is written");
  ResultsFromCalc::forget(calc);
  Evaluator marked;
  CHECK(marked.build(calc) && !marked.fromResults, "a file marked failed is ignored (Props path)");
}

// ---- parsed text (orca.mo / xtb.mo output) ----------------------------------

static bool finish(const string& name, ResultsFromCalc::Parsed& in, double nelec)
{
  resultsh5::Model m;
  string why;
  CHECK(ResultsFromCalc::convert(in, m, why), name + ": converted (" + why + ")");
  if (m.mos.empty()) return false;
  const string line = ResultsFromCalc::check(m);
  printf("  %s\n", line.c_str());
  CHECK(m.checks.compare(0, 3, "ok:") == 0, name + ": " + m.checks);
  double occ = 0;
  for (double o : in.occupation[0]) occ += o;
  for (double o : in.occupation[1]) occ += o;
  CHECK(fabs(occ - nelec) < 1e-6, name + ": occupations sum to N");

  // The canonical basis with the file's coefficients against ECCE's flat
  // basis with the parsed ones.
  vector<EspBasisFunction> can;
  string err;
  CHECK(resultsh5::buildAos(m, 0, can, err), "canonical AOs");
  const vector<double> pts = gridPoints(in.coordsAng);
  double worst = 0;
  for (size_t k = 0; k + 2 < pts.size(); k += 3) {
    const double x = pts[k]/0.52917724924, y = pts[k+1]/0.52917724924, z = pts[k+2]/0.52917724924;
    double rhoA = 0, rhoB = 0;
    for (int sp = 0; sp < 2; sp++)
      for (size_t r = 0; r < in.rows[sp].size(); r++) {
        const double o = in.occupation[sp].size() > r ? in.occupation[sp][r] : 0;
        if (o == 0) continue;
        double pa = 0, pb = 0;
        for (size_t i = 0; i < in.basis.size(); i++) pa += in.rows[sp][r][i]*in.basis[i].value(x, y, z);
        const resultsh5::Mos& mo = m.mos[sp];
        for (size_t i = 0; i < can.size(); i++) pb += mo.coef[r*mo.nao + i]*can[i].value(x, y, z);
        rhoA += o*pa*pa;
        rhoB += o*pb*pb;
      }
    const double s = max(fabs(rhoA), fabs(rhoB));
    if (s > 1e-13) worst = max(worst, fabs(rhoA - rhoB)/s);
  }
  printf("  density from the file's AOs against ECCE's flattened basis: max relative difference %.2e\n", worst);
  CHECK(worst <= 1e-8, name + ": density agrees to 1e-8 relative");
  return true;
}

static vector<double> block(const string& text, const string& key, vector<int>& size)
{
  vector<double> v;
  size_t p = text.find("key: " + key + "\n");
  if (p == string::npos) return v;
  size_t e = text.find("END\n", p);
  istringstream in(text.substr(p, e - p));
  string line;
  bool inVals = false, inSize = false;
  while (getline(in, line)) {
    if (line == "size:") { inSize = true; continue; }
    if (inSize) { istringstream s(line); int n; while (s >> n) size.push_back(n); inSize = false; continue; }
    if (line == "values:") { inVals = true; continue; }
    if (line == "units:") break;
    if (inVals) { istringstream s(line); double d; while (s >> d) v.push_back(d); }
  }
  return v;
}

static int atomicNumberOf(const string& symbol) { TAtm a(symbol.c_str()); return a.atomicNumber(); }

namespace {
class FakeCalc : public IPropCalculation {
public:
  string marker;
  TProperty* getProperty(const string& name) {
    if (name == "MOAOORDER" && !marker.empty()) {
      PropString *p = (PropString*)PropFactory::getProperty("MOAOORDER");
      if (p) p->value(marker);
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
}

static string slurp(const string& path) { ifstream f(path); stringstream s; s << f.rdbuf(); return s.str(); }

static void orcaBlock(const string& parsed, const string& nbPath, const string& xyzPath, double nelec)
{
  printf("== ORCA %s\n", nbPath.c_str());
  const JCode *code = CodeFactory::lookup("ORCA");
  ifstream nb(nbPath.c_str());
  TGBSConfig *cfg = ICalcUtils::importConfig(nb);
  CHECK(code && cfg, "ORCA registered, basis fixture parsed");
  if (!code || !cfg) return;
  ResultsFromCalc::Parsed in;
  ifstream xyz(xyzPath.c_str());
  string s; double x, y, z;
  while (xyz >> s >> x >> y >> z) {
    in.symbols.push_back(s); in.atomicNumber.push_back(atomicNumberOf(s));
    in.coordsAng.push_back(x); in.coordsAng.push_back(y); in.coordsAng.push_back(z);
  }
  TGBSAngFunc *af = code->getAngFunc(TGaussianBasisSet::Spherical);
  int lenS[7] = {1,3,5,7,9,11,13};
  CHECK(BasisFlatten::flatten(in.symbols, in.coordsAng, cfg, code, af, af->maxShells(), lenS, in.basis),
        "basis flattened");
  vector<int> size;
  string text = slurp(parsed);
  vector<double> mo = block(text, "MO", size);
  CHECK(size.size() == 2 && !mo.empty(), "orca.mo's MO block parsed");
  if (size.size() != 2) return;
  PropTable *t = (PropTable*)PropFactory::getProperty("MO");
  t->values(size[0], size[1], mo);
  FakeCalc fake;
  fake.marker = "angular-momentum";
  PropTable *re = MoAoOrder::reorderToNative(t, &fake, in.symbols, cfg);
  for (int r = 0; r < re->rows(); r++) {
    vector<double> row(re->columns());
    for (int c = 0; c < re->columns(); c++) row[c] = re->value(r, c);
    in.rows[0].push_back(row);
    in.occupation[0].push_back(r < (int)nelec/2 ? 2.0 : 0.0);
  }
  in.code = "ORCA";
  finish("ORCA", in, nelec);
}

static void xtbBlock(const string& moldenPath, const string& parsed, double nelec)
{
  printf("== xtb %s\n", moldenPath.c_str());
  ifstream mf(moldenPath);
  string line;
  ResultsFromCalc::Parsed in;
  bool atoms = false;
  while (getline(mf, line)) {
    if (line.compare(0, 1, "[") == 0) { atoms = (line.compare(0, 6, "[Atoms") == 0); continue; }
    if (!atoms) continue;
    istringstream ss(line);
    string name; int idx, zz; double x, y, z;
    if (!(ss >> name >> idx >> zz >> x >> y >> z)) continue;
    TAtm a((short)zz);
    in.symbols.push_back(a.atomicSymbol()); in.atomicNumber.push_back(zz);
    in.coordsAng.push_back(x*0.52917724924); in.coordsAng.push_back(y*0.52917724924);
    in.coordsAng.push_back(z*0.52917724924);
  }
  string text = slurp(parsed);
  vector<int> gs, ms, os;
  vector<double> gto = block(text, "GTOBASIS", gs), mo = block(text, "MO", ms), occ = block(text, "ORBOCC", os);
  CHECK(!gto.empty() && !mo.empty() && !occ.empty(), "xtb.mo output complete");
  const JCode *code = CodeFactory::lookup("xTB");
  TGBSConfig *cfg = ICalcUtils::gtoBasisConfig(gto);
  CHECK(code && cfg, "xTB registered, GTOBASIS read");
  if (!code || !cfg) return;
  TGBSAngFunc *ang = code->getAngFunc(TGaussianBasisSet::Cartesian);
  int len[7] = {1,3,6,10,15,21,28};
  CHECK(BasisFlatten::flatten(in.symbols, in.coordsAng, cfg, code, ang, ang->maxShells(), len, in.basis), "flattened");
  for (int r = 0; r < ms[0]; r++)
    in.rows[0].push_back(vector<double>(mo.begin() + r*ms[1], mo.begin() + (r+1)*ms[1]));
  in.occupation[0] = occ;
  in.code = "xTB";
  finish("xtb", in, nelec);
}

// ---- a spin channel with no electrons (#943) --------------------------------

// H2+ (doublet, UHF): one alpha electron, none beta.  tr(PS) = 0 for an empty
// channel is exact, so the check must accept it rather than divide by N = 0.
static void emptySpin()
{
  printf("== H2+ with an empty beta channel\n");
  resultsh5::Model m;
  const double R = 2.0;                       // bohr; one s primitive, exponent 1
  m.atomicNumber = {1, 1};
  m.coord = {0, 0, 0, 0, 0, R};
  for (int a = 0; a < 2; a++) {
    resultsh5::Shell sh;
    sh.atom = a;
    sh.exponent = {1.0};
    sh.coef = {1.0};
    m.basis.shells.push_back(sh);
  }
  const double S = exp(-0.5*R*R);             // overlap of the two normalised s
  const double g = 1/sqrt(2 + 2*S), u = 1/sqrt(2 - 2*S);
  for (int spin = 0; spin < 2; spin++) {
    resultsh5::Mos mo;
    mo.spin = spin;
    mo.nmo = mo.nao = 2;
    mo.coef = {g, g, u, -u};
    mo.energy = {-1.1, 0.2};
    mo.occupation = {spin == 0 ? 1.0 : 0.0, 0.0};
    m.mos.push_back(mo);
  }
  m.nelectron[0] = 1;
  m.nelectron[1] = 0;
  const string line = ResultsFromCalc::check(m);
  printf("  %s\n", line.c_str());
  CHECK(m.checks.compare(0, 3, "ok:") == 0, "an empty beta channel passes: " + m.checks);

}

static bool flatValues(TProperty *p, vector<double>& out)
{
  out.clear();
  if (PropVector *v = dynamic_cast<PropVector*>(p)) { out = v->values(); return true; }
  if (PropVecTable *v = dynamic_cast<PropVecTable*>(p)) {
    for (int t = 0; t < v->tables(); t++)
      out.insert(out.end(), v->values(t).begin(), v->values(t).end());
    return true;
  }
  if (PropTSVecTable *v = dynamic_cast<PropTSVecTable*>(p)) {
    for (int t = 0; t < v->tables(); t++)
      out.insert(out.end(), v->values(t).begin(), v->values(t).end());
    return true;
  }
  return false;
}

//  The two paths of every view property: values from results.h5 must equal
//  the parsed ones to 1e-6 of the largest magnitude (the Props hold the
//  code's printed digits; ECCE-QM's own file holds full precision), and
//  with ECCE_RESULTS_H5=0 the view gets the parsed property itself.
static void views(string arg)
{
  string want;
  const size_t eq = arg.find('=');
  if (eq != string::npos) { want = "," + arg.substr(eq + 1) + ","; arg = arg.substr(0, eq); }
  printf("== %s\n", arg.c_str());
  Resource *res = EDSIFactory::getResource(EcceURL("file://" + arg));
  IPropCalculation *calc = dynamic_cast<IPropCalculation*>(res);
  CHECK(calc != 0, "opens as a calculation");
  if (!calc) return;
  const char *keys[] = {"VIBFREQ", "VIBIR", "VIBRAM", "VIB", "SCANGEOM", "SCANENERGY",
                        "SCANCOORD", "IRCGEOM", "IRCENERGY", "IRCCOORD", "GEOMTRACE", "TEVEC"};
  for (const char *key : keys) {
    setenv("ECCE_RESULTS_H5", "1", 1);
    TProperty *parsed = calc->getProperty(key);
    if (parsed == 0) continue;
    ResultsFromCalc::forget(calc);
    TProperty *viewed = ResultsFromCalc::property(calc, key);
    const bool fromFile = viewed != parsed;
    vector<double> a, b;
    flatValues(parsed, a);
    flatValues(viewed, b);
    double scale = 0, diff = 0;
    for (double x : a) scale = max(scale, fabs(x));
    if (a.size() == b.size())
      for (size_t i = 0; i < a.size(); i++) diff = max(diff, fabs(a[i] - b[i]));
    char msg[200];
    snprintf(msg, sizeof msg, "%-10s %-8s n=%zu max|diff| %.2e (scale %.3g, %s)", key,
             fromFile ? "file" : "props", a.size(), diff, scale, parsed->units().c_str());
    CHECK(a.size() == b.size() && diff <= 1e-6*max(scale, 1.0), msg);
    if (want.find("," + string(key) + ",") != string::npos)
      CHECK(fromFile, string(key) + " is read from results.h5");
    setenv("ECCE_RESULTS_H5", "0", 1);
    ResultsFromCalc::forget(calc);
    CHECK(ResultsFromCalc::property(calc, key) == parsed,
          string(key) + ": ECCE_RESULTS_H5=0 gives the parsed property");
  }
  setenv("ECCE_RESULTS_H5", "1", 1);
}

int main(int argc, char **argv)
{
  if (argc < 2) { fprintf(stderr, "usage: see the head of this file\n"); return 2; }
  EDSIServerCentral central;
  const string mode = argv[1];
  if (mode == "fixture")
    for (int i = 2; i < argc; i++) fixture(argv[i]);
  else if (mode == "orca" && argc >= 6) orcaBlock(argv[2], argv[3], argv[4], atof(argv[5]));
  else if (mode == "xtb" && argc >= 5) xtbBlock(argv[2], argv[3], atof(argv[4]));
  else if (mode == "emptyspin") emptySpin();
  else if (mode == "views")
    for (int i = 2; i < argc; i++) views(argv[i]);
  else return 2;
  printf("%s\n", failures ? "FAIL" : "PASS");
  return failures ? 1 : 0;
}
