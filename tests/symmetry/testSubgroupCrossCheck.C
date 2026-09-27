//  #147/#132 -- the independent, no-gensym-frame cross-check: does
//  SymmetryAnalysis::subgroupLabelSpectrum() reproduce a code's OWN
//  abelian-subgroup labels, built directly from diagonal +/-1
//  operations in the code's OWN (stored) frame?
//
//  ORCA auto-detects the full point group (Td for CH4) but symmetry-
//  adapts orbitals only in the largest ABELIAN subgroup (D2) and prints
//  geometry + MOs in that frame -- no autosym/gensym involved on this
//  side at all, which is the whole point: this check does not share
//  machinery with fullOrbitalIrrep()'s frame-fitting path, so agreement
//  here is real, independent evidence for the stored-frame premise
//  (that a code's own geometry dump IS the frame its MOs are in).
//
//  D2's three C2 classes carry no axis in the table ("classes: E C2 C2
//  C2"), so subgroupLabelSpectrum() tries all six axis assignments and
//  keeps whichever agrees with ORCA's own labels -- verified against
//  real output, not assumed from a textbook (Cotton's convention is
//  not consulted anywhere in this file).
//
//  Needs a build tree (JCode/XML); no symops/autosym needed for this
//  particular check (that's the point) -- see run_tests.py.
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#include "tdat/SymmetryAnalysis.H"
#include "tdat/CharacterTable.H"
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

static string upper(const string& s)
{
  string r = s;
  for (size_t i = 0; i < r.size(); i++) r[i] = toupper((unsigned char)r[i]);
  return r;
}

static void check(bool ok, const string& what)
{
  checks++;
  printf("  %-70s %s\n", what.c_str(), ok ? "ok" : "FAIL");
  if (!ok) bad++;
}

struct Fixture {
  string group, subgroup;
  vector<string> elements;
  vector<double> stored;
  string basisText;
  vector<double> energies;
  vector<string> reportedLabels;
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

static bool loadFixture(const string& path, int natoms, int nbasis, Fixture& fx)
{
  ifstream in(path.c_str());
  if (!in) return false;
  string tok;
  while (in >> tok) {
    if (!tok.empty() && tok[0] == '#') { string rest; getline(in, rest); continue; }
    if (tok == "group") { in >> fx.group; }
    else if (tok == "subgroup") { in >> fx.subgroup; }
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
      string line; getline(in, line); getline(in, line);
      istringstream ls(line); string s;
      while (ls >> s) fx.reportedLabels.push_back(s);
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

int main(int argc, char** argv)
{
  if (argc < 4) {
    fprintf(stderr, "usage: %s <fixture> <natoms> <nbasis>\n", argv[0]);
    return 2;
  }
  const int NATOMS = atoi(argv[2]);
  const int NBASIS = atoi(argv[3]);

  Fixture fx;
  if (!loadFixture(argv[1], NATOMS, NBASIS, fx)) {
    fprintf(stderr, "could not read fixture %s\n", argv[1]);
    return 2;
  }
  check(!fx.subgroup.empty(), "fixture names the code's own reported subgroup");
  check((int)fx.reportedLabels.size() == NBASIS, "fixture: reported labels read");
  check((int)fx.coefficients.size() == NBASIS, "fixture: MO coefficients read");

  const JCode* code = CodeFactory::lookup("ORCA");
  check(code != 0, "ORCA JCode loaded");
  if (!code) return 1;

  const bool spherical = fx.basisText.find("spherical") != string::npos;
  istringstream gbsIn(fx.basisText);
  TGBSConfig* cfg = ICalcUtils::importConfig(gbsIn);
  check(cfg != 0, "TGBSConfig parsed from the fixture's NumericalBasis text");
  if (!cfg) return 1;

  TGBSAngFunc* angfunc = code->getAngFunc(spherical ? TGaussianBasisSet::Spherical
                                                    : TGaussianBasisSet::Cartesian);
  check(angfunc != 0, "real ORCA MOOrdering table loaded");
  if (!angfunc) return 1;

  vector<EspBasisFunction> basis;
  int lengthShellCart[7] = { 1, 3, 6, 10, 15, 21, 28 };
  int lengthShellSph[7]  = { 1, 3, 5, 7, 9, 11, 13 };
  int *lengthShell = spherical ? lengthShellSph : lengthShellCart;
  bool flat = BasisFlatten::flatten(fx.elements, fx.stored, cfg, code, angfunc,
                                    angfunc->maxShells(), lengthShell, basis);
  check(flat, "basis flattened");
  check((int)basis.size() == NBASIS, "flattened basis size matches ORCA's own nbasis");
  if (!flat || (int)basis.size() != NBASIS) return 1;

  vector< vector<double> > S(NBASIS, vector<double>(NBASIS, 0.0));
  for (int i = 0; i < NBASIS; i++)
    for (int j = 0; j < NBASIS; j++)
      S[i][j] = EspField::overlapOf(basis[i], basis[j]);
  check(fabs(S[0][0] - 1.0) < 1.0e-6, "overlap diagonal is 1 (basis is normalised)");

  vector<int> shellTypeOf(NBASIS);
  for (int i = 0; i < NBASIS; i++) {
    int deg = basis[i].powerX.empty() ? 0
            : basis[i].powerX[0] + basis[i].powerY[0] + basis[i].powerZ[0];
    shellTypeOf[i] = deg;
  }
  vector<int> perAtom(NATOMS);
  for (int a = 0; a < NATOMS; a++) {
    const string& el = fx.elements[a];
    int n = (el == "H") ? 2 : (el == "S") ? 19 : 15;
    if (spherical && el != "H") n -= 1;
    perAtom[a] = n;
  }

  const CharacterTable* subgroupTable = CharacterTable::lookup(fx.subgroup.c_str());
  check(subgroupTable != 0, ("subgroup table " + fx.subgroup + " loaded").c_str());
  if (!subgroupTable) return 1;

  vector<string> derived;
  string axisNote;
  int labelled = SymmetryAnalysis::subgroupLabelSpectrum(
      fx.coefficients, fx.reportedLabels, perAtom, shellTypeOf, S, fx.elements,
      fx.stored, angfunc, *subgroupTable, 1.0e-4, 1.0e-4, derived, axisNote);

  check(labelled >= 0, "subgroupLabelSpectrum() recognised the subgroup name");
  printf("\n  axis convention found: %s\n\n", axisNote.c_str());

  int agree = 0, disagree = 0, declined = 0;
  for (int i = 0; i < NBASIS; i++) {
    string mine = derived[i].empty() ? "-" : derived[i];
    string theirs = fx.reportedLabels[i];
    string status;
    if (derived[i].empty()) { declined++; status = "DECLINED"; }
    else {
      string a = mine, b = theirs;
      for (size_t c = 0; c < a.size(); c++) a[c] = toupper((unsigned char)a[c]);
      for (size_t c = 0; c < b.size(); c++) b[c] = toupper((unsigned char)b[c]);
      if (a == b) { agree++; status = "agree"; } else { disagree++; status = "DISAGREE"; }
    }
    printf("    orbital %2d  E=%9.5f  %s=%-4s  computed=%-4s  %s\n",
           i+1, fx.energies[i], fx.subgroup.c_str(), theirs.c_str(),
           mine.c_str(), status.c_str());
  }
  printf("\n  %d labelled, %d agree, %d disagree, %d declined (of %d orbitals)\n",
         labelled, agree, disagree, declined, NBASIS);

  check(disagree == 0, "no orbital disagrees with the code's own subgroup label");
  check(agree == labelled, "every labelled orbital agrees (declines aside)");
  check(agree > 0, "at least one orbital agreed -- confirms the stored-frame premise");

  //  SUBDUCTION: every full-group label fullLabelSpectrum() assigns
  //  must subduce, via its characters on the SAME physical operations
  //  subgroupLabelSpectrum() just used, to the code's own label for
  //  that orbital -- independent of the full-group computation's own
  //  frame-fitting machinery (this uses the calibrated D2 axis
  //  assignment above, and matches physical operations, not frames).
  if (!fx.group.empty() && fx.group != fx.subgroup) {
    printf("\n  subduction check: %s labels must subduce to %s\n",
           fx.group.c_str(), fx.subgroup.c_str());

    const CharacterTable* fullTable = CharacterTable::lookup(fx.group.c_str());
    check(fullTable != 0, ("full group table " + fx.group + " loaded").c_str());

    const char* symopsEnv = getenv("ECCE_TEST_SYMOPS");
    string symopsBin = symopsEnv ? symopsEnv : "build-cmake/symops";
    string cmd = "echo " + fx.group + " | " + symopsBin;
    FILE* pipe = popen(cmd.c_str(), "r");
    vector<SymOp> fullOps;
    if (pipe) {
      int n = 0;
      bool ok = (fscanf(pipe, "%d", &n) == 1) && n > 0;
      for (int o = 0; ok && o < n; o++) {
        SymOp op; double t;
        for (int r = 0; r < 3; r++)
          if (fscanf(pipe, "%lf %lf %lf %lf", &op.m[r][0], &op.m[r][1],
                    &op.m[r][2], &t) != 4) { ok = false; break; }
        fullOps.push_back(op);
      }
      pclose(pipe);
    }
    check(!fullOps.empty() && fullTable != 0 &&
          fullOps.size() == (size_t)fullTable->order(),
          "full-group operations read from symops");

    //  Re-read the fixture's "probe" line (autosym's standard frame),
    //  ignored above -- needed here to align/conjugate the full-group
    //  operations into the stored frame.
    ifstream in2(argv[1]);
    string tok; vector<double> probe; vector<string> dummyEl;
    while (in2 >> tok) {
      if (!tok.empty() && tok[0] == '#') { string r; getline(in2, r); continue; }
      if (tok == "stored") { vector<string> e; vector<double> c;
        readGeom(in2, NATOMS, e, c, true); continue; }
      if (tok == "probe") { readGeom(in2, NATOMS, dummyEl, probe, false); break; }
    }

    SymOp Q; double rmsd;
    bool alignOk = !probe.empty() &&
        SymmetryAnalysis::alignFrames(fx.stored, probe, Q, rmsd);
    check(alignOk, "full-group frame alignment succeeded");

    vector<SymOp> opsInCoeffFrame(fullOps.size());
    for (size_t i = 0; i < fullOps.size(); i++)
      opsInCoeffFrame[i] = SymmetryAnalysis::conjugate(fullOps[i], Q);

    vector< vector<int> > images;
    bool imgOk = fullTable != 0 && alignOk &&
        SymmetryAnalysis::atomImages(probe, fx.elements, fullOps, 1.0e-4, images);
    vector< vector<int> > classes;
    vector<int> classOfOp;
    bool classOk = imgOk &&
        (SymmetryAnalysis::conjugacyClasses(fullOps, classes), true) &&
        SymmetryAnalysis::matchClasses(fullOps, classes, *fullTable, classOfOp);
    check(classOk, "full-group operations classified");

    vector<string> fullDerived;
    int fullLabelled = -1;
    if (classOk) {
      fullLabelled = SymmetryAnalysis::fullLabelSpectrum(
          fx.coefficients, fx.energies, perAtom, shellTypeOf, S, images,
          classOfOp, opsInCoeffFrame, angfunc, *fullTable, 1.0e-4, fullDerived);
    }
    check(fullLabelled > 0, "full-group labels computed for the subduction check");

    //  Match each physical D2 operation (from subgroupLabelSpectrum's
    //  calibrated axis assignment, read back out the same way it built
    //  them) to the full-group operation IT IS, in the stored frame --
    //  same matrix, found by direct comparison, not by frame-fitting.
    int perm[3];
    {
      // Recover the winning permutation by re-running the same six
      // candidates and checking which yields subgroupTable's own
      // agreement count -- cheap, and avoids threading the choice out
      // of subgroupLabelSpectrum() through another parameter.
      static const int PERMS[6][3] = { {0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0} };
      int best = 0;
      for (int p = 0; p < 6; p++) {
        char tag[8];
        snprintf(tag, sizeof(tag), "%d%d%d", PERMS[p][0], PERMS[p][1], PERMS[p][2]);
        if (axisNote.find(string("(1st table C2)=") +
            (PERMS[p][0]==0?"z":PERMS[p][0]==1?"y":"x")) != string::npos &&
            axisNote.find(string("(2nd)=") +
            (PERMS[p][1]==0?"z":PERMS[p][1]==1?"y":"x")) != string::npos)
          best = p;
      }
      perm[0] = PERMS[best][0]; perm[1] = PERMS[best][1]; perm[2] = PERMS[best][2];
    }
    SymOp c2phys[3] = {
      { {{-1,0,0},{0,-1,0},{0,0,1}} },
      { {{-1,0,0},{0,1,0},{0,0,-1}} },
      { {{1,0,0},{0,-1,0},{0,0,-1}} }
    };
    const SymOp inv = { {{-1,0,0},{0,-1,0},{0,0,-1}} };

    //  The subgroup's own physical operations, same order as its table
    //  columns: E, the 3 (permuted) C2's, and -- for D2H, order 8 --
    //  i and the 3 sigma_k = i*C2_perm[k] (elementwise, diagonal), the
    //  SAME pairing SymmetryAnalysis.C's d2hFamilyOps() uses.
    const int subOrder = subgroupTable->order();
    vector<SymOp> subPhys;
    subPhys.push_back({ {{1,0,0},{0,1,0},{0,0,1}} });
    for (int k = 0; k < 3; k++) subPhys.push_back(c2phys[perm[k]]);
    if (subOrder == 8) {
      subPhys.push_back(inv);
      for (int k = 0; k < 3; k++) {
        const SymOp& c = c2phys[perm[k]];
        SymOp s;
        for (int a=0;a<3;a++) for (int b=0;b<3;b++) s.m[a][b] =
            (a==b) ? inv.m[a][a]*c.m[a][a] : 0.0;
        subPhys.push_back(s);
      }
    }
    check((int)subPhys.size() == subOrder,
          "built the subgroup's own physical operation set (order matches)");

    vector<int> subgroupClassToFullClass(subOrder, -1);
    for (int k = 1; k < subOrder; k++) {
      const SymOp& want = subPhys[k];
      int foundClass = -1;
      for (size_t o = 0; o < opsInCoeffFrame.size() && foundClass < 0; o++) {
        bool same = true;
        for (int a = 0; a < 3 && same; a++)
          for (int b = 0; b < 3 && same; b++)
            if (fabs(opsInCoeffFrame[o].m[a][b] - want.m[a][b]) > 1.0e-3) same = false;
        if (same) foundClass = classOfOp[o];
      }
      subgroupClassToFullClass[k] = foundClass;
    }
    bool matchedAll = true;
    for (int k = 1; k < subOrder; k++)
      if (subgroupClassToFullClass[k] < 0) matchedAll = false;
    check(matchedAll, "every subgroup operation matched to one of the full "
          "group's own operations, in the stored frame (no extra alignment)");

    if (matchedAll && classOk) {
      const vector<string>& subNames = subgroupTable->irreps();
      int subductionOk = 0, subductionChecked = 0;
      for (int i = 0; i < NBASIS; i++) {
        if (fullDerived[i].empty() || derived[i].empty()) continue;
        const vector<double>* chiFull = fullTable->characters(fullDerived[i]);
        if (chiFull == 0) continue;

        //  mult(X) = (1/|subgroup|) * sum_{op in subgroup} chi_full(op's
        //  full class) * chi_X(op) -- the subgroup's own table row for
        //  X, evaluated at E and its other (singleton) classes.
        double bestMult = -1;
        for (size_t x = 0; x < subNames.size(); x++) {
          const vector<double>* chiSub = subgroupTable->characters(subNames[x]);
          if (chiSub == 0 || (int)chiSub->size() != subOrder) continue;
          double sum = (*chiFull)[0]*(*chiSub)[0];   // E, full class 0 always E
          for (int k = 1; k < subOrder; k++)
            sum += (*chiFull)[subgroupClassToFullClass[k]] * (*chiSub)[k];
          const double mult = sum / subOrder;
          if (upper(subNames[x]) == upper(derived[i])) bestMult = mult;
        }
        subductionChecked++;
        const bool ok = bestMult > 0.5;
        if (ok) subductionOk++;
        if (!ok) {
          printf("    orbital %2d: %s does NOT subduce to include %s "
                 "(multiplicity %.3f)\n", i+1, fullDerived[i].c_str(),
                 derived[i].c_str(), bestMult);
        }
      }
      char msg[160];
      snprintf(msg, sizeof(msg), "%d of %d full-group labels subduce to include "
               "the code's own %s label", subductionOk, subductionChecked,
               fx.subgroup.c_str());
      check(subductionOk == subductionChecked, msg);
    }
  }

  printf("\n%s (%d checks)\n", bad ? "FAIL" : "PASS", checks);
  return bad ? 1 : 0;
}
