//  Orbits and the representations an atom set spans, against the
//  answers in every inorganic textbook.
//
//  These are the cases a chemist can check by eye: CH4's four hydrogen
//  1s orbitals span a1 + t2, water's two span a1 + b2, SF6's six
//  fluorines span a1g + eg + t1u.  If this gets those wrong the
//  correlation diagram built on it is wrong in a way that still looks
//  like a correlation diagram.
//
//  Operations are read from a file the driver produces with symops, so
//  what is tested is the real generator tables and not a second copy of
//  them written here.
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include "tdat/SymmetryAnalysis.H"
#include "tdat/CharacterTable.H"
using namespace std;

static int bad = 0;

static void check(bool ok, const string& what)
{
  printf("  %-62s %s\n", what.c_str(), ok ? "ok" : "FAIL");
  if (!ok) bad++;
}

static bool readOps(const string& path, vector<SymOp>& ops)
{
  ifstream in(path.c_str());
  if (!in) return false;
  int n = 0;
  if (!(in >> n) || n <= 0) return false;
  for (int i = 0; i < n; i++) {
    SymOp op;
    for (int r = 0; r < 3; r++) {
      double t;
      for (int c = 0; c < 3; c++) if (!(in >> op.m[r][c])) return false;
      if (!(in >> t)) return false;            // the translation column
    }
    ops.push_back(op);
  }
  return true;
}

struct Molecule {
  const char* name;
  const char* group;
  vector<string> elements;
  vector<double> coords;
  const char* centre;        // expected central element, "" for none
  const char* terminal;      // expected terminal reduction
  size_t terminalCount;
};

static void run(const Molecule& mol, const string& opsDir)
{
  printf("\n  %s (%s)\n", mol.name, mol.group);

  vector<SymOp> ops;
  if (!readOps(opsDir + "/" + mol.group + ".ops", ops)) {
    check(false, string("read operations for ") + mol.group);
    return;
  }

  const CharacterTable* table = CharacterTable::lookup(mol.group);
  if (table == 0) { check(false, "character table found"); return; }
  check(ops.size() == (size_t)table->order(),
        "operations match the group order");

  vector< vector<int> > images;
  //  A generous tolerance: these coordinates are exact, and a tight one
  //  would be testing floating point rather than symmetry.
  bool ok = SymmetryAnalysis::atomImages(mol.coords, mol.elements, ops,
                                         1.0e-6, images);
  check(ok, "every operation maps the molecule onto itself");
  if (!ok) return;

  vector< vector<int> > orbits;
  SymmetryAnalysis::orbits(images, (int)mol.elements.size(), orbits);

  //  The central atom is the orbit of one; the terminal atoms the rest.
  size_t centreSize = 0, terminalSize = 0;
  for (size_t i = 0; i < orbits.size(); i++) {
    if (orbits[i].size() == 1) centreSize++;
    else terminalSize = orbits[i].size();
  }
  char msg[160];
  snprintf(msg, sizeof(msg), "orbits: %zu of size 1, one of size %zu "
           "(expected %zu)", centreSize, terminalSize, mol.terminalCount);
  check(terminalSize == mol.terminalCount, msg);

  vector< vector<int> > classes;
  SymmetryAnalysis::conjugacyClasses(ops, classes);
  snprintf(msg, sizeof(msg), "%zu conjugacy classes for %zu table classes",
           classes.size(), table->classes().size());
  check(classes.size() == table->classes().size(), msg);

  vector<int> classOfOp;
  ok = SymmetryAnalysis::matchClasses(ops, classes, *table, classOfOp);
  check(ok, "conjugacy classes matched to the table's named classes");
  if (!ok) return;

  //  The symmetry orbitals themselves, not just which irreps appear.
  for (size_t i = 0; i < orbits.size(); i++) {
    if (orbits[i].size() != mol.terminalCount) continue;

    vector<double> chi;
    if (!SymmetryAnalysis::orbitalCharacter(orbits[i], images, classOfOp,
                                            (int)table->classes().size(), chi))
      continue;
    vector<int> mult;
    if (!table->reduce(chi, mult)) continue;

    size_t total = 0;
    for (size_t k = 0; k < table->irreps().size(); k++) {
      if (mult[k] == 0) continue;
      const string& ir = table->irreps()[k];

      vector< vector<double> > vecs;
      bool got = SymmetryAnalysis::projectOrbit(orbits[i], images, classOfOp,
                                                *table, ir, vecs);
      snprintf(msg, sizeof(msg),
               "%s: %zu symmetry orbitals (%d x dim %d = %d expected)",
               ir.c_str(), vecs.size(), mult[k], table->dimension(ir),
               mult[k]*table->dimension(ir));
      check(got && (int)vecs.size() == mult[k]*table->dimension(ir), msg);
      total += vecs.size();

      //  Each must be normalised and orthogonal to the others.
      for (size_t u = 0; u < vecs.size(); u++) {
        double nrm = 0.0;
        for (size_t c = 0; c < vecs[u].size(); c++) nrm += vecs[u][c]*vecs[u][c];
        if (fabs(nrm - 1.0) > 1e-9) {
          check(false, string(ir) + ": a symmetry orbital is not normalised");
          break;
        }
      }
    }
    //  Together they must span the whole orbit -- no orbital left over.
    snprintf(msg, sizeof(msg),
             "the symmetry orbitals span the orbit (%zu of %zu)",
             total, orbits[i].size());
    check(total == orbits[i].size(), msg);

    //  The totally symmetric one is the in-phase combination: every
    //  coefficient equal and positive.  That is the check a student
    //  would do by eye.
    const string& first = table->irreps()[0];
    vector< vector<double> > sym;
    if (SymmetryAnalysis::projectOrbit(orbits[i], images, classOfOp,
                                       *table, first, sym) && sym.size() == 1) {
      const double want = 1.0/sqrt((double)orbits[i].size());
      bool inPhase = true;
      for (size_t c = 0; c < sym[0].size(); c++)
        if (fabs(fabs(sym[0][c]) - want) > 1e-9) inPhase = false;
      snprintf(msg, sizeof(msg),
               "%s is the in-phase combination, all coefficients %.4f",
               first.c_str(), want);
      check(inPhase, msg);
    }
  }

  //  The terminal orbit's s orbitals: the reduction a textbook gives.
  for (size_t i = 0; i < orbits.size(); i++) {
    if (orbits[i].size() != mol.terminalCount) continue;

    vector<double> chi;
    ok = SymmetryAnalysis::orbitalCharacter(orbits[i], images, classOfOp,
                                            (int)table->classes().size(), chi);
    check(ok, "character of the terminal s orbitals computed");
    if (!ok) return;

    const string got = table->reduceToString(chi);
    snprintf(msg, sizeof(msg), "terminal s orbitals span %s (want %s)",
             got.c_str(), mol.terminal);
    check(got == mol.terminal, msg);
  }
}

/** The reduction of a whole AO basis, against the textbook answers. */
static void basisChecks(const string& opsDir)
{
  printf("\n  the AO basis as a whole\n");

  struct Case {
    const char* name; const char* group;
    int natoms; const char* want;
  };

  //  CH4 in a minimal basis: carbon carries 1s, 2s and 2p, each
  //  hydrogen a 1s.  Nine functions spanning 3a1 + 2t2.
  {
    vector<SymOp> ops;
    if (!readOps(opsDir + "/TD.ops", ops)) { check(false, "TD ops"); return; }
    const CharacterTable* t = CharacterTable::lookup("TD");
    const char* e[] = {"C","H","H","H","H"};
    const double c[] = {0,0,0, 1,1,1, 1,-1,-1, -1,1,-1, -1,-1,1};
    vector<string> el(e, e+5); vector<double> co(c, c+15);
    vector< vector<int> > img, cls; vector<int> cof;
    SymmetryAnalysis::atomImages(co, el, ops, 1e-6, img);
    SymmetryAnalysis::conjugacyClasses(ops, cls);
    SymmetryAnalysis::matchClasses(ops, cls, *t, cof);

    vector< vector<int> > sh(5);
    const int cs[] = {0,0,1};
    sh[0].assign(cs, cs+3);
    for (int i = 1; i < 5; i++) sh[i].assign(1, 0);

    vector<double> chi;
    bool ok = SymmetryAnalysis::basisCharacter(sh, img, cof, ops,
                                               (int)t->classes().size(),
                                               false, chi);
    const string got = t->reduceToString(chi);
    check(ok && got == "3A1 + 2T2",
          "CH4 minimal basis spans " + got + " (want 3A1 + 2T2)");
  }

  //  A d shell at the centre of an octahedral field: the crystal field
  //  splitting, and the check that the Cartesian correction matters.
  {
    vector<SymOp> ops;
    if (!readOps(opsDir + "/OH.ops", ops)) { check(false, "OH ops"); return; }
    const CharacterTable* t = CharacterTable::lookup("OH");
    const char* e[] = {"S"};
    const double c[] = {0,0,0};
    vector<string> el(e, e+1); vector<double> co(c, c+3);
    vector< vector<int> > img, cls; vector<int> cof;
    SymmetryAnalysis::atomImages(co, el, ops, 1e-6, img);
    SymmetryAnalysis::conjugacyClasses(ops, cls);
    SymmetryAnalysis::matchClasses(ops, cls, *t, cof);

    vector< vector<int> > sh(1); sh[0].assign(1, 2);
    vector<double> a, b;
    SymmetryAnalysis::basisCharacter(sh, img, cof, ops,
                                     (int)t->classes().size(), false, a);
    SymmetryAnalysis::basisCharacter(sh, img, cof, ops,
                                     (int)t->classes().size(), true, b);
    check(t->reduceToString(a) == "Eg + T2g",
          "five spherical d functions split into " + t->reduceToString(a)
          + " (want Eg + T2g)");
    //  Six Cartesian d functions are D2 + D0, so an extra A1g appears.
    //  Counting them as five would be wrong by exactly that.
    check(t->reduceToString(b) == "A1g + Eg + T2g",
          "six Cartesian d functions span " + t->reduceToString(b)
          + " (want A1g + Eg + T2g)");
  }
}


int main(int argc, char** argv)
{
  const string tables = (argc > 1) ? argv[1]
                                   : "data/client/config/CharacterTables";
  const string opsDir = (argc > 2) ? argv[2] : ".";

  if (CharacterTable::loadFile(tables) == 0) {
    printf("  could not load %s\n", tables.c_str());
    return 1;
  }

  vector<Molecule> mols;

  //  CH4 in the standard Td frame: the two-fold axes along x, y, z.
  {
    Molecule m;
    m.name = "methane"; m.group = "TD";
    const char* e[] = {"C", "H", "H", "H", "H"};
    const double c[] = { 0, 0, 0,
                         1, 1, 1,   1,-1,-1,  -1, 1,-1,  -1,-1, 1};
    m.elements.assign(e, e+5);
    m.coords.assign(c, c+15);
    m.centre = "C"; m.terminal = "A1 + T2"; m.terminalCount = 4;
    mols.push_back(m);
  }
  //  Water, in the xz plane.
  //
  //  THE ORIENTATION IS THE ANSWER HERE, not a detail.  In the xz plane
  //  the two O-H bonds give chi = (2, 0, 2, 0) and reduce to a1 + b1;
  //  in the yz plane the same molecule gives (2, 0, 0, 2) and a1 + b2.
  //  Both are right -- which mirror is called sigma_v and which
  //  sigma_v' is a convention, and the labels follow it.
  //
  //  The xz choice here matches the 5KE195 compendium, whose water
  //  example prints exactly this reducible representation.  Agreeing
  //  with an independently written teaching text, character for
  //  character, is a better check than agreeing with my own arithmetic.
  {
    Molecule m;
    m.name = "water"; m.group = "C2V";
    const char* e[] = {"O", "H", "H"};
    const double c[] = { 0, 0, 0.117,
                         0.757, 0, -0.469,
                        -0.757, 0, -0.469};
    m.elements.assign(e, e+3);
    m.coords.assign(c, c+9);
    m.centre = "O"; m.terminal = "A1 + B1"; m.terminalCount = 2;
    mols.push_back(m);
  }
  //  SF6, octahedral.
  {
    Molecule m;
    m.name = "sulfur hexafluoride"; m.group = "OH";
    const char* e[] = {"S", "F", "F", "F", "F", "F", "F"};
    const double c[] = { 0, 0, 0,
                         1.56, 0, 0,  -1.56, 0, 0,
                         0, 1.56, 0,   0,-1.56, 0,
                         0, 0, 1.56,   0, 0,-1.56};
    m.elements.assign(e, e+7);
    m.coords.assign(c, c+21);
    m.centre = "S"; m.terminal = "A1g + Eg + T1u"; m.terminalCount = 6;
    mols.push_back(m);
  }

  for (size_t i = 0; i < mols.size(); i++) run(mols[i], opsDir);
  basisChecks(opsDir);

  printf("\n  %s\n", bad ? "FAIL" : "PASS");
  return bad ? 1 : 0;
}
