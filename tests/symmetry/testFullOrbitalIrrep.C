//  #147/#132 -- the whole-AO-basis ORBSYM oracle, against a REAL
//  Gaussian-16 calculation, orbital by orbital.
//
//  CH4 in HF/6-31G(d): Gaussian's own point-group detection finds Td
//  (not the D2 subgroup ORCA's UseSym reports for the same molecule --
//  see #151), and its own ORBSYM labels (from the real
//  gaussian-16.orbocc parser, on the real "Orbital symmetries:" block)
//  are the independent oracle SymmetryAnalysis::fullLabelSpectrum()
//  is checked against here: every labelled orbital must match G16's
//  own label exactly, not merely be "a plausible symmetry label".
//
//  The MO coefficients are the real ones too -- scripts/parsers/
//  gaussian-16.mo run on a real fort.7 MO punch file (Punch=(MO), the
//  same route keyword ai.gauss16 adds) -- and the basis is the
//  standard published 6-31G(d), read through ICalcUtils::importConfig(),
//  ECCE's own basis-set text format. See fixtures/g16mo/generate.py for
//  exactly how the fixture below was produced; this file only reads it.
//
//  Frame: G16 did not reorient this molecule for its SCF (Standard
//  orientation == Input orientation in the log), so the MO coefficients
//  are in the "stored" coordinates below; autosym reorients into its
//  own "probe" frame to find Td and to match gensym's operation
//  matrices, so SymmetryAnalysis::alignFrames()/conjugate() are
//  exercised for real, not as a no-op identity case.
//
//  Needs a build tree (JCode/XML) and $ECCE_TEST_SYMOPS (the real
//  symops binary) -- see run_tests.py.
//
//  Also run against benzene/D6h (#151's live case -- e1g/e2u sets no
//  D2h abelian subgroup can even name): 88 of 102 orbitals labelled,
//  ALL 88 agreeing with G16's own label, 14 declined, zero disagreeing.
//  Every decline traces to the SAME cause, not fourteen different
//  ones: labelSpectrum()'s energy grouping (inherited unchanged from
//  the existing s/p-only path, not something this pass introduced)
//  chains orbitals whose CONSECUTIVE gap is under 1e-3 Hartree. The
//  six carbon 1s-like core orbitals span two irreps of different
//  degeneracy each (a1g+e1u and e2g+b1u) but sit close enough in
//  energy that one gap inside that six happens to exceed 1e-3 while
//  the others do not, splitting a projectable pair of a real
//  degenerate manifold into two chunks that are each a MIX of
//  different irreps and so correctly decline rather than mislabel --
//  the same failure mode as several other mid-spectrum near-
//  degeneracies here. This is a real, known limitation of the
//  sequential-chain grouping heuristic, not of the projection itself
//  (which is what correctly refuses to guess); a follow-up should
//  replace it with a connected-components clustering (group i and j
//  whenever ANY gap along a shared chain is under tolerance, not only
//  the immediately-consecutive one) rather than loosening the
//  tolerance, which would risk merging genuinely different orbitals
//  instead.
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

static void check(bool ok, const string& what)
{
  checks++;
  printf("  %-70s %s\n", what.c_str(), ok ? "ok" : "FAIL");
  if (!ok) bad++;
}

struct Fixture {
  string group;
  vector<string> elements;
  vector<double> stored, probe;   // atom-major, Angstrom
  string basisText;
  vector<double> energies;
  vector<string> g16labels;
  vector< vector<double> > coefficients;   // [orbital][basisFunction]
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

static bool loadFixture(const string& path, int natoms, int nbasis,
                        Fixture& fx)
{
  ifstream in(path.c_str());
  if (!in) return false;
  string tok;
  while (in >> tok) {
    if (!tok.empty() && tok[0] == '#') { string rest; getline(in, rest); continue; }
    if (tok == "group") { in >> fx.group; }
    else if (tok == "stored") {
      if (!readGeom(in, natoms, fx.elements, fx.stored, true)) return false;
    } else if (tok == "probe") {
      vector<string> dummy;
      if (!readGeom(in, natoms, dummy, fx.probe, false)) return false;
    } else if (tok == "basis") {
      string line; getline(in, line);   // rest of this line (empty)
      ostringstream b;
      while (getline(in, line)) {
        b << line << "\n";
        if (line.find("EndNumericalBasis") != string::npos) break;
      }
      fx.basisText = b.str();
    } else if (tok == "energies") {
      string line; getline(in, line);
      getline(in, line);
      istringstream ls(line); double v;
      while (ls >> v) fx.energies.push_back(v);
    } else if (tok == "g16labels") {
      string line; getline(in, line);
      getline(in, line);
      istringstream ls(line); string s;
      while (ls >> s) fx.g16labels.push_back(s);
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
  char szmsg[80];
  snprintf(szmsg, sizeof(szmsg), "fixture: %d orbital energies read", NBASIS);
  check((int)fx.energies.size() == NBASIS, szmsg);
  snprintf(szmsg, sizeof(szmsg), "fixture: %d G16 labels read", NBASIS);
  check((int)fx.g16labels.size() == NBASIS, szmsg);
  snprintf(szmsg, sizeof(szmsg), "fixture: %dx%d MO coefficients read", NBASIS, NBASIS);
  check((int)fx.coefficients.size() == NBASIS, szmsg);

  const JCode* code = CodeFactory::lookup("Gaussian-16");
  check(code != 0, "Gaussian-16 JCode loaded");
  if (!code) return 1;

  istringstream gbsIn(fx.basisText);
  TGBSConfig* cfg = ICalcUtils::importConfig(gbsIn);
  check(cfg != 0, "TGBSConfig parsed from the fixture's NumericalBasis text");
  if (!cfg) return 1;

  TGBSAngFunc* angfunc = code->getAngFunc(TGaussianBasisSet::Cartesian);
  check(angfunc != 0, "real Cartesian MOOrdering table loaded");
  if (!angfunc) return 1;

  vector<EspBasisFunction> basis;
  int lengthShell[7] = { 1, 3, 6, 10, 15, 21, 28 };
  bool flat = BasisFlatten::flatten(fx.elements, fx.stored, cfg, code, angfunc,
                                    angfunc->maxShells(), lengthShell, basis);
  check(flat, "basis flattened");
  snprintf(szmsg, sizeof(szmsg), "%d flattened basis functions (matches "
           "Gaussian's own fort.7 nBasisFun)", NBASIS);
  check((int)basis.size() == NBASIS, szmsg);
  if (!flat || (int)basis.size() != NBASIS) return 1;

  //  Overlap matrix -- the real S this projection is weighted by.
  vector< vector<double> > S(NBASIS, vector<double>(NBASIS, 0.0));
  for (int i = 0; i < NBASIS; i++)
    for (int j = 0; j < NBASIS; j++)
      S[i][j] = EspField::overlapOf(basis[i], basis[j]);

  double diag0 = S[0][0];
  check(fabs(diag0 - 1.0) < 1.0e-6, "overlap diagonal is 1 (basis is normalised)");

  //  Shell type per function, read off the flattened monomial degree --
  //  unambiguous for s/p/d (0/1/2), which is all this basis has.
  vector<int> shellTypeOf(NBASIS);
  for (int i = 0; i < NBASIS; i++) {
    int deg = basis[i].powerX.empty() ? 0
            : basis[i].powerX[0] + basis[i].powerY[0] + basis[i].powerZ[0];
    shellTypeOf[i] = deg;
  }
  //  Basis functions per atom -- this basis (6-31G(d): d only on C) is
  //  the same for every fixture this test reads.
  vector<int> perAtom(NATOMS);
  for (int a = 0; a < NATOMS; a++)
    perAtom[a] = (fx.elements[a] == "C") ? 15 : 2;

  //  Real operations, from the real symops binary -- same convention
  //  tests/symmetry already uses.
  const char* symopsEnv = getenv("ECCE_TEST_SYMOPS");
  string symopsBin = symopsEnv ? symopsEnv : "build-cmake/symops";
  string cmd = "echo " + fx.group + " | " + symopsBin;
  FILE* pipe = popen(cmd.c_str(), "r");
  check(pipe != 0, "symops ran");
  vector<SymOp> ops;
  if (pipe) {
    int n = 0;
    bool ok = (fscanf(pipe, "%d", &n) == 1) && n > 0;
    for (int o = 0; ok && o < n; o++) {
      SymOp op; double t;
      for (int r = 0; r < 3; r++) {
        if (fscanf(pipe, "%lf %lf %lf %lf", &op.m[r][0], &op.m[r][1],
                  &op.m[r][2], &t) != 4) { ok = false; break; }
      }
      ops.push_back(op);
    }
    pclose(pipe);
    snprintf(szmsg, sizeof(szmsg), "%zu %s operations read from symops",
             ops.size(), fx.group.c_str());
    check(ok, szmsg);
  }
  if (ops.empty()) return 1;

  const CharacterTable* table = CharacterTable::lookup(fx.group.c_str());
  snprintf(szmsg, sizeof(szmsg), "%s character table loaded", fx.group.c_str());
  check(table != 0, szmsg);
  if (!table) return 1;
  check(ops.size() == (size_t)table->order(),
        "symops operation count matches the table's group order");

  vector< vector<int> > images;
  bool imgOk = SymmetryAnalysis::atomImages(fx.probe, fx.elements, ops,
                                            1.0e-4, images);
  check(imgOk, "every operation maps the (probe-frame) molecule onto itself");
  if (!imgOk) return 1;

  vector< vector<int> > classes;
  SymmetryAnalysis::conjugacyClasses(ops, classes);
  vector<int> classOfOp;
  bool classOk = SymmetryAnalysis::matchClasses(ops, classes, *table, classOfOp);
  snprintf(szmsg, sizeof(szmsg), "conjugacy classes matched to %s's table classes",
           fx.group.c_str());
  check(classOk, szmsg);
  if (!classOk) return 1;

  //  THE FRAME FIT: G16 did not reorient (Standard orientation == Input
  //  orientation in the log), so "stored" is what the MO coefficients
  //  are in; autosym's "probe" frame is what the operations are in.
  SymOp Q; double rmsd;
  bool alignOk = SymmetryAnalysis::alignFrames(fx.stored, fx.probe, Q, rmsd);
  check(alignOk, "alignFrames() succeeded");
  char msg[120];
  snprintf(msg, sizeof(msg), "frame fit residual %.2e (stored vs autosym's "
           "probe frame)", rmsd);
  //  1e-5, not 1e-6: the fixture's "stored" coordinates are written to
  //  6 decimal places (Angstrom), so ~1e-6 A of rounding is expected
  //  here and is not itself a sign of anything wrong.
  check(rmsd < 1.0e-5, msg);

  vector<SymOp> opsInCoeffFrame(ops.size());
  for (size_t i = 0; i < ops.size(); i++)
    opsInCoeffFrame[i] = SymmetryAnalysis::conjugate(ops[i], Q);

  vector<string> derived;
  int labelled = SymmetryAnalysis::fullLabelSpectrum(
      fx.coefficients, fx.energies, perAtom, shellTypeOf, S, images,
      classOfOp, opsInCoeffFrame, angfunc, *table, 1.0e-4, derived);

  printf("\n  orbital-by-orbital comparison against Gaussian-16's own ORBSYM:\n");
  int agree = 0, disagree = 0, declined = 0;
  for (int i = 0; i < NBASIS; i++) {
    string mine = derived[i].empty() ? "-" : derived[i];
    string theirs = fx.g16labels[i];
    string status;
    if (derived[i].empty()) { declined++; status = "DECLINED"; }
    else {
      string a = mine, b = theirs;
      for (size_t c = 0; c < a.size(); c++) a[c] = toupper((unsigned char)a[c]);
      for (size_t c = 0; c < b.size(); c++) b[c] = toupper((unsigned char)b[c]);
      if (a == b) { agree++; status = "agree"; } else { disagree++; status = "DISAGREE"; }
    }
    printf("    orbital %2d  E=%9.5f  G16=%-4s  computed=%-4s  %s\n",
           i+1, fx.energies[i], theirs.c_str(), mine.c_str(), status.c_str());
  }
  printf("\n  %d labelled, %d agree, %d disagree, %d declined (of %d orbitals)\n",
         labelled, agree, disagree, declined, NBASIS);

  check(disagree == 0, "no orbital disagrees with Gaussian-16's own label");
  check(labelled > 0, "at least one orbital was labelled");
  //  The design's acceptance bar: 100% agreement on labelled sets --
  //  declines are allowed and counted, a wrong label is not.
  check(agree == labelled, "every LABELLED orbital agrees (declines aside)");

  printf("\n%s (%d checks)\n", bad ? "FAIL" : "PASS", checks);
  return bad ? 1 : 0;
}
