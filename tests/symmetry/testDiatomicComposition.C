//  #132 -- the composition fallback, against a REAL homonuclear
//  diatomic (N2, real ORCA MO coefficients from
//  fixtures/g16mo/n2-orca-dinfh.txt -- the same fixture
//  testSubgroupCrossCheck.C already verifies is real and correctly
//  read).
//
//  WHY THIS IS THE CASE THAT MATTERS.  A single N atom spans no irrep
//  of D4H at all -- half the operations swap it onto its partner -- so
//  the fragment columns for a homonuclear diatomic carry NO irrep
//  labels, and MoDiagramPanel::build() clears the centre column's
//  labels too rather than compare against a spelling that means
//  something different (see its "COLUMNS THAT CARRY NO IRREP" comment).
//  Composition -- which SHELL an orbital is built from, from its own
//  coefficients -- is the only thing left that can connect it, which
//  is exactly what MoFragments::composeLevels() supplies and
//  MoDiagram::connect() falls back to.
//
//  This drives that real pipeline (MoFragments::build() for the
//  fragment columns, composeLevels() for the shares, classify()/
//  connect()/placeFragments() for the diagram) on N2's real MOs with
//  NO symmetry labels handed to the centre column -- the scenario a
//  homonuclear diatomic is always in -- and checks the textbook
//  answer: 2sigma_g/2sigma_u connect only to N 2s, and
//  3sigma_g/1pi_u/1pi_g*/3sigma_u* connect only to N 2p.
//
//  Needs a build tree (JCode/XML); no symops/autosym needed (the
//  fragment columns are built directly from the two-atom geometry via
//  the HALVES construction, which needs only the group name to look up
//  its character table and generate its own operations in-process --
//  see run_tests.py).
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#include "tdat/MoFragments.H"
#include "tdat/MoComposition.H"
#include "tdat/MoDiagram.H"
#include "tdat/BasisFlatten.H"
#include "tdat/EspField.H"
#include "tdat/TGBSAngFunc.H"
#include "dsm/TGBSConfig.H"
#include "dsm/TGaussianBasisSet.H"
#include "dsm/ICalcUtils.H"
#include "dsm/JCode.H"
#include "dsm/CodeFactory.H"

static int bad = 0;
static int checks = 0;

static void check(bool ok, const string& what)
{
  checks++;
  printf("  %-70s %s\n", what.c_str(), ok ? "ok" : "FAIL");
  if (!ok) bad++;
}

struct Fixture {
  string group;
  vector<string> elements;
  vector<double> stored;
  string basisText;
  vector<double> energies;
  vector< vector<double> > coefficients;
};

static bool readGeom(ifstream& in, int natoms, vector<string>& elements,
                     vector<double>& coords, bool captureElements)
{
  for (int i = 0; i < natoms; i++) {
    string sym; double x, y, z;
    if (!(in >> sym >> x >> y >> z)) return false;
    if (captureElements) elements.push_back(sym);
    coords.push_back(x); coords.push_back(y); coords.push_back(z);
  }
  return true;
}

//  Same parser testFullOrbitalIrrep.C/testSubgroupCrossCheck.C use,
//  trimmed to what this check needs (no g16labels: the whole point is
//  to run with NONE, which is what a diatomic's fragment columns force
//  in the real panel).
static bool loadFixture(const string& path, int natoms, int nbasis, Fixture& fx)
{
  ifstream in(path.c_str());
  if (!in) return false;
  string tok;
  while (in >> tok) {
    if (!tok.empty() && tok[0] == '#') { string rest; getline(in, rest); continue; }
    if (tok == "group") { in >> fx.group; }
    else if (tok == "subgroup") { string s; in >> s; }
    else if (tok == "stored") {
      if (!readGeom(in, natoms, fx.elements, fx.stored, true)) return false;
    } else if (tok == "probe") {
      vector<string> dummy; vector<double> unused;
      if (!readGeom(in, natoms, dummy, unused, false)) return false;
    } else if (tok == "basis") {
      string line; getline(in, line);
      ostringstream b;
      while (getline(in, line)) {
        b << line << "\n";
        if (line.find("EndNumericalBasis") != string::npos) break;
      }
      fx.basisText = b.str();
    } else if (tok == "energies") {
      string line; getline(in, line); getline(in, line);
      istringstream ls(line); double v;
      while (ls >> v) fx.energies.push_back(v);
    } else if (tok == "g16labels") {
      string line; getline(in, line); getline(in, line);   // discarded
    } else if (tok == "coefficients") {
      string line; getline(in, line);
      for (int r = 0; r < nbasis; r++) {
        if (!getline(in, line)) return false;
        istringstream ls(line);
        vector<double> row; double v;
        while (ls >> v) row.push_back(v);
        if ((int)row.size() != nbasis) return false;
        fx.coefficients.push_back(row);
      }
    }
  }
  return true;
}

//  Which shellKeys index is "N:0" (2s) / "N:1" (2p) in a column.
static int shellSlot(const MoColumn& col, int l)
{
  ostringstream key; key << "N:" << l;
  for (size_t k = 0; k < col.shellKeys.size(); k++) {
    if (col.shellKeys[k] == key.str()) return (int)k;
  }
  return -1;
}

int main(int argc, char** argv)
{
  if (argc < 2) {
    fprintf(stderr, "usage: %s <fixtures-dir>\n", argv[0]);
    return 2;
  }
  const int NATOMS = 2, NBASIS = 28;
  const string path = string(argv[1]) + "/n2-orca-dinfh.txt";

  Fixture fx;
  if (!loadFixture(path, NATOMS, NBASIS, fx)) {
    fprintf(stderr, "could not read fixture %s\n", path.c_str());
    return 2;
  }
  check((int)fx.energies.size() == NBASIS, "fixture: 28 orbital energies read");
  check((int)fx.coefficients.size() == NBASIS, "fixture: 28x28 MO coefficients read");
  check(fx.elements.size() == 2 && fx.elements[0] == "N" && fx.elements[1] == "N",
       "fixture: two nitrogen atoms");

  const JCode* code = CodeFactory::lookup("ORCA");
  check(code != 0, "ORCA JCode loaded");
  if (!code) return 1;

  istringstream gbsIn(fx.basisText);
  TGBSConfig* cfg = ICalcUtils::importConfig(gbsIn);
  check(cfg != 0, "TGBSConfig parsed from the fixture's NumericalBasis text");
  if (!cfg) return 1;

  const bool spherical = fx.basisText.find("spherical") != string::npos;
  TGBSAngFunc* angfunc = code->getAngFunc(spherical ? TGaussianBasisSet::Spherical
                                                    : TGaussianBasisSet::Cartesian);
  check(angfunc != 0, "angular table loaded");
  if (!angfunc) return 1;

  //  Basis functions per atom and each function's angular momentum --
  //  the SAME derivation MoDiagramPanel's functionsPerAtom() falls
  //  back to computing directly, cross-checked here via the flattened
  //  basis the way computeFullGroupLabels() does it (one walk,
  //  trusted, not two that could disagree).
  vector<EspBasisFunction> basis;
  int lengthShellCart[7] = { 1, 3, 6, 10, 15, 21, 28 };
  int lengthShellSph[7]  = { 1, 3, 5, 7, 9, 11, 13 };
  bool flat = BasisFlatten::flatten(fx.elements, fx.stored, cfg, code, angfunc,
                                    angfunc->maxShells(),
                                    spherical ? lengthShellSph : lengthShellCart,
                                    basis);
  check(flat && (int)basis.size() == NBASIS, "basis flattened to 28 functions");
  if (!flat || (int)basis.size() != NBASIS) return 1;

  vector<int> perAtom, shellOf;
  {
    size_t i = 0;
    while (i < basis.size()) {
      size_t j = i;
      while (j < basis.size() &&
             fabs(basis[j].center[0]-basis[i].center[0]) < 1.0e-9 &&
             fabs(basis[j].center[1]-basis[i].center[1]) < 1.0e-9 &&
             fabs(basis[j].center[2]-basis[i].center[2]) < 1.0e-9) j++;
      perAtom.push_back((int)(j - i));
      i = j;
    }
    for (size_t k = 0; k < basis.size(); k++) {
      shellOf.push_back(basis[k].powerX[0]+basis[k].powerY[0]+basis[k].powerZ[0]);
    }
  }
  check(perAtom.size() == 2 && perAtom[0] == 14 && perAtom[1] == 14,
       "14 basis functions on each nitrogen");

  //  --- the fragment columns: two single atoms, no irrep of D4H at all ---
  MoColumn left, right;
  vector<int> leftAtoms, rightAtoms;
  string why;
  bool haveFragments = MoFragments::build(fx.stored, fx.elements, "D4H", 0,
                                          left, right, why, &leftAtoms,
                                          &rightAtoms);
  check(haveFragments, "fragment columns built (two equivalent halves)");
  if (!haveFragments) fprintf(stderr, "why: %s\n", why.c_str());
  check(leftAtoms.size() == 1 && rightAtoms.size() == 1,
       "each half is a single atom -- the diatomic case");
  const int leftS = shellSlot(left, 0), leftP = shellSlot(left, 1);
  const int rightS = shellSlot(right, 0), rightP = shellSlot(right, 1);
  check(leftS >= 0 && leftP >= 0 && rightS >= 0 && rightP >= 0,
       "both columns carry an N 2s shell and an N 2p shell");

  //  --- the molecular column, with NO symmetry labels at all ---
  //
  //  Exactly the state a homonuclear diatomic's fragment columns force
  //  in MoDiagramPanel::build(): centre.levels[].irrep cleared because
  //  neither fragment column can carry one to compare against.
  MoColumn centre;
  map<string,int> noDimensions;
  MoDiagram::groupByIrrep(fx.energies, vector<double>(), vector<string>(),
                          noDimensions, 1.0e-4, centre.levels);
  //  Truly degenerate orbitals (the pi pairs) still group by energy
  //  alone with no labels at all -- that is groupByIrrep()'s own
  //  tolerance grouping, unrelated to what this test checks, so levels
  //  are found by which real orbital they contain (level.orbitals)
  //  rather than assumed to be one per orbital.
  check(centre.levels.size() > 0 && centre.levels.size() <= (size_t)NBASIS,
       "orbitals grouped into drawn levels");

  MoFragments::composeLevels(centre.levels, fx.coefficients, perAtom, shellOf,
                             fx.elements, leftAtoms, rightAtoms, left, right);

  vector<MoConnection> links;
  MoDiagram::classify(left.levels, centre.levels, right.levels, left.fromHalves);
  MoDiagram::connect(left.levels, centre.levels, right.levels, links);
  MoDiagram::placeFragments(centre, left, right, links);

  check(!links.empty(), "composition connected at least one level -- "
                        "the #132 regression (no lines drawn at all)");

  //  --- the textbook answer ---
  //
  //  The fixture's own orbital ORDER (ascending energy, as printed) is
  //  the textbook order: 0,1 = 1sigma_g/1sigma_u (N 1s core); 2,3 =
  //  2sigma_g/2sigma_u (N 2s); 4..9 = 3sigma_g, 1pi_u x2, 1pi_g* x2,
  //  3sigma_u* (N 2p). Which DRAWN level a given real orbital ended up
  //  in is read off level.orbitals rather than assumed, since
  //  groupByIrrep() may have merged a truly degenerate pair (the pi
  //  sets) into one level even with no labels at all.
  auto levelOf = [&](int orbitalIndex) -> int {
    for (size_t L = 0; L < centre.levels.size(); L++) {
      const vector<int>& orbs = centre.levels[L].orbitals;
      for (size_t k = 0; k < orbs.size(); k++) {
        if (orbs[k] == orbitalIndex) return (int)L;
      }
    }
    return -1;
  };

  //  (has an N 2s line, has an N 2p line) for the level containing a
  //  given real orbital -- reading it off the real links, not off
  //  which shell has the bigger tabulated energy, so a wrong
  //  connection shows up as a wrong pair of flags rather than being
  //  silently re-derived away.
  auto shellLines = [&](int orbitalIndex, bool& hasS, bool& hasP) {
    hasS = hasP = false;
    const int centreLevel = levelOf(orbitalIndex);
    if (centreLevel < 0) return;
    for (size_t i = 0; i < links.size(); i++) {
      if (links[i].centreLevel != centreLevel) continue;
      if (links[i].leftLevel == leftS || links[i].rightLevel == rightS) hasS = true;
      if (links[i].leftLevel == leftP || links[i].rightLevel == rightP) hasP = true;
    }
  };

  //  2sigma_g/2sigma_u: N 2s and ONLY N 2s -- nothing pulls a sigma
  //  built from the 2s shell towards 2p at this separation.
  for (int i = 0; i < 2; i++) {
    bool hasS, hasP;
    shellLines(2 + i, hasS, hasP);
    char msg[80];
    snprintf(msg, sizeof(msg), "%s connects to N 2s, and only N 2s",
            i == 0 ? "2sigma_g" : "2sigma_u");
    check(hasS && !hasP, msg);
  }

  //  3sigma_g, 1pi_u x2, 1pi_g* x2, 3sigma_u*: all N 2p-based.  The two
  //  sigma members of this set (3sigma_g, 3sigma_u*) MAY also draw a
  //  weaker N 2s line -- sp mixing between two orbitals of the same
  //  symmetry is real chemistry in N2, not a fault -- but every one of
  //  the six must connect to N 2p.
  const char* names[] = { "3sigma_g", "1pi_u(a)", "1pi_u(b)",
                          "1pi_g*(a)", "1pi_g*(b)", "3sigma_u*" };
  bool allP = true;
  for (int i = 0; i < 6; i++) {
    bool hasS, hasP;
    shellLines(4 + i, hasS, hasP);
    char msg[80];
    snprintf(msg, sizeof(msg), "%s connects to N 2p", names[i]);
    check(hasP, msg);
    if (!hasP) allP = false;
  }
  check(allP, "the whole valence set (3sg,1piu,1pig*,3su*) is N 2p-based");

  //  And the PI orbitals specifically must show NO 2s line at all --
  //  a pi orbital has no s component by symmetry, sp-mixing or not.
  for (int i = 1; i <= 4; i++) {
    bool hasS, hasP;
    shellLines(4 + i, hasS, hasP);
    char msg[80];
    snprintf(msg, sizeof(msg), "%s has no N 2s line (pi has no s component)",
            names[i]);
    check(!hasS, msg);
  }

  printf("\n  %d checks, %d failed\n", checks, bad);
  return bad ? 1 : 0;
}
