//  trace(P.S) = N through ECCE's own C++ for xtb orbitals.
//
//  argv[1] = xtb's molden.input (atoms), argv[2] = what scripts/parsers/xtb.mo
//  printed for it, argv[3] = number of electrons.  The GTOBASIS rows go
//  through ICalcUtils::gtoBasisConfig() and BasisFlatten::flatten() with
//  xTB.edml's real (componentNormalized) Cartesian MOOrdering; the overlap
//  is EspField::overlapOf().  The independent Python check of the same
//  numbers is tests/parsers/xtb_trace_ps_test.py.

#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#include "tdat/BasisFlatten.H"
#include "tdat/EspField.H"
#include "tdat/TAtm.H"
#include "tdat/TGBSAngFunc.H"
#include "dsm/TGBSConfig.H"
#include "dsm/TGaussianBasisSet.H"
#include "dsm/ICalcUtils.H"
#include "dsm/JCode.H"
#include "dsm/CodeFactory.H"

static vector<double> block(const string& text, const string& key,
                            vector<int>& size)
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

int main(int argc, char** argv)
{
  if (argc < 4) return 2;
  ifstream mf(argv[1]);
  string line;
  vector<string> sym;
  vector<double> xyzAng;
  bool atoms = false;
  while (getline(mf, line)) {
    if (line.compare(0, 1, "[") == 0) { atoms = (line.compare(0, 6, "[Atoms") == 0); continue; }
    if (!atoms) continue;
    istringstream s(line);
    string name; int idx, z; double x, y, zz;
    if (!(s >> name >> idx >> z >> x >> y >> zz)) continue;
    TAtm a((short)z);
    sym.push_back(a.atomicSymbol());
    xyzAng.push_back(x*0.52917724924);
    xyzAng.push_back(y*0.52917724924);
    xyzAng.push_back(zz*0.52917724924);
  }
  ifstream pf(argv[2]);
  stringstream ps; ps << pf.rdbuf();
  string text = ps.str();
  const double nel = atof(argv[3]);

  vector<int> gs, ms, os;
  vector<double> gto = block(text, "GTOBASIS", gs);
  vector<double> mo = block(text, "MO", ms);
  vector<double> occ = block(text, "ORBOCC", os);
  if (gto.empty() || mo.empty() || occ.empty()) { printf("FAIL: parser output incomplete\n"); return 1; }

  const JCode* code = CodeFactory::lookup("xTB");
  if (code == 0) { printf("FAIL: xTB JCode (ECCE_HOME?)\n"); return 1; }
  TGBSConfig* cfg = ICalcUtils::gtoBasisConfig(gto);
  if (cfg == 0) { printf("FAIL: gtoBasisConfig\n"); return 1; }
  TGBSAngFunc* ang = code->getAngFunc(TGaussianBasisSet::Cartesian);
  if (ang == 0) { printf("FAIL: cartesian MOOrdering\n"); return 1; }
  int len[7] = { 1, 3, 6, 10, 15, 21, 28 };
  vector<EspBasisFunction> basis;
  if (!BasisFlatten::flatten(sym, xyzAng, cfg, code, ang, ang->maxShells(), len, basis)) {
    printf("FAIL: flatten\n"); return 1;
  }
  const int nmo = ms[0], nbas = ms[1];
  if ((int)basis.size() != nbas) { printf("FAIL: %d functions, MO has %d\n", (int)basis.size(), nbas); return 1; }

  double tr = 0, maxdev = 0;
  vector<vector<double> > S(nbas, vector<double>(nbas));
  for (int i = 0; i < nbas; i++)
    for (int j = 0; j < nbas; j++) S[i][j] = EspField::overlapOf(basis[i], basis[j]);
  for (int m = 0; m < nmo; m++) {
    for (int n = 0; n <= m; n++) {
      double d = 0;
      for (int i = 0; i < nbas; i++)
        for (int j = 0; j < nbas; j++) d += mo[m*nbas+i]*S[i][j]*mo[n*nbas+j];
      maxdev = fmax(maxdev, fabs(d - (m == n ? 1.0 : 0.0)));
      if (m == n) tr += occ[m]*d;
    }
  }
  bool ok = fabs(tr - nel) < 1e-4 && maxdev < 1e-4;
  printf("ECCE C++ overlap: nbas=%d trace(PS)=%.6f (N=%g) max|C'SC-1|=%.1e %s\n",
         nbas, tr, nel, maxdev, ok ? "ok" : "FAIL");
  return ok ? 0 : 1;
}
