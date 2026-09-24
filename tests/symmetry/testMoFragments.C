//  The two outer columns of a correlation diagram, against the answers
//  a textbook gives.
//
//  These are the cases the 5KE195 compendium works through, which is
//  the point: if this disagrees with the course it is meant to support,
//  it is wrong, whatever the code does internally.
//
//  Methane is the one that pins the machinery.  Carbon contributes 2s
//  (a1) and 2p (t2); the four hydrogen 1s orbitals span a1 + t2.  Get
//  the orbit wrong, the class matching wrong, the angular characters
//  wrong or the reduction wrong, and one of those four labels changes.
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
using namespace std;

#include "tdat/MoFragments.H"
#include "tdat/CharacterTable.H"

static int bad = 0;

static void check(const char* what, const string& got, const string& want) {
  bool ok = (got == want);
  printf("  %-46s %-22s %-22s %s\n", what, got.c_str(), want.c_str(),
         ok ? "ok" : "FAIL");
  if (!ok) bad++;
}

static void checkd(const char* what, double got, double want, double tol) {
  bool ok = fabs(got - want) <= tol;
  printf("  %-46s %-22.3f %-22.3f %s\n", what, got, want, ok ? "ok" : "FAIL");
  if (!ok) bad++;
}

/** The irrep names in a column, stripped of shell and multiplicity. */
static string labels(const MoColumn& col) {
  string out;
  for (size_t i = 0; i < col.levels.size(); i++) {
    if (!out.empty()) out += " ";
    out += col.levels[i].label;
  }
  return out;
}

int main(int argc, char** argv) {
  if (argc > 1) CharacterTable::loadFile(argv[1]);

  printf("  %-46s %-22s %-22s\n", "", "got", "expected");

  //  --- methane, Td -------------------------------------------------
  //  C at the origin, H at alternate cube corners: the frame gensym's
  //  Td operations are written in.
  {
    const double d = 0.6276;   // Angstrom, C-H = 1.087
    double c[] = {
       0.0,  0.0,  0.0,
         d,    d,    d,
         d,   -d,   -d,
        -d,    d,   -d,
        -d,   -d,    d };
    vector<double> coords(c, c + 15);

    const char* e[] = { "C", "H", "H", "H", "H" };
    vector<string> elements(e, e + 5);

    MoColumn left, right;
    string note;
    bool ok = MoFragments::build(coords, elements, "TD", left, right, note);
    printf("  %-46s %s\n", "methane builds", ok ? "yes" : note.c_str());
    if (!ok) { bad++; }
    else {
      check("CH4: carbon's valence orbitals",  labels(left),
            "A1  (2s) T2  (2p)");
      check("CH4: the four H 1s orbitals",     labels(right),
            "A1  (1s) T2  (1s)");
      checkd("CH4: C 2p sits above C 2s",
             left.levels[1].energy - left.levels[0].energy, 8.7, 0.05);
      checkd("CH4: H 1s below C 2p",
             right.levels[0].energy, -13.6, 0.01);

      //  The degeneracies, which the labels alone do not carry.
      bool degs = (left.levels[0].degeneracy == 1 &&
                   left.levels[1].degeneracy == 3 &&
                   right.levels[0].degeneracy == 1 &&
                   right.levels[1].degeneracy == 3);
      printf("  %-46s %s\n", "CH4: degeneracies 1,3 and 1,3",
             degs ? "ok" : "FAIL");
      if (!degs) bad++;
    }
  }

  //  --- water, C2v --------------------------------------------------
  //  The other case every course does: O 2s -> a1, O 2p -> a1+b1+b2,
  //  and the two H 1s orbitals -> a1 + b1 (in the convention where the
  //  molecule lies in the xz plane, which is the one the character
  //  table's class names carry).
  {
    double c[] = {
       0.0,  0.0,  0.1173,
       0.0,  0.7572, -0.4692,
       0.0, -0.7572, -0.4692 };
    vector<double> coords(c, c + 9);

    const char* e[] = { "O", "H", "H" };
    vector<string> elements(e, e + 3);

    MoColumn left, right;
    string note;
    bool ok = MoFragments::build(coords, elements, "C2V", left, right, note);
    printf("  %-46s %s\n", "water builds", ok ? "yes" : note.c_str());
    if (!ok) bad++;
    else {
      //  Three p levels, one per irrep, and two TASOs.  The exact
      //  b1/b2 assignment depends which mirror the table calls sv, so
      //  only the count and the totally symmetric member are asserted
      //  here -- the labelling itself is what tests/symmetry's
      //  translations check already pins.
      bool shape = (left.levels.size() == 4 && right.levels.size() == 2);
      printf("  %-46s %s  (%zu and %zu)\n",
             "H2O: 4 oxygen levels, 2 symmetry orbitals",
             shape ? "ok" : "FAIL", left.levels.size(), right.levels.size());
      if (!shape) bad++;

      checkd("H2O: oxygen 2s at -32.4 eV", left.levels[0].energy, -32.4, 0.01);
    }
  }

  //  --- a molecule this kind of diagram does not describe ------------
  //  Ethane has no atom the whole group leaves in place.  The right
  //  answer is to say so, not to nominate one of the carbons.
  {
    double c[] = {
       0.0, 0.0,  0.7680,
       0.0, 0.0, -0.7680 };
    vector<double> coords(c, c + 6);
    const char* e[] = { "C", "C" };
    vector<string> elements(e, e + 2);

    MoColumn left, right;
    string note;
    bool ok = MoFragments::build(coords, elements, "D2H", left, right, note);
    bool refused = !ok && note.find("central atom") != string::npos;
    printf("  %-46s %s\n",
           "C2: no central atom, and it says so",
           refused ? "ok" : ("FAIL: " + note).c_str());
    if (!refused) bad++;
  }

  printf("\n  %s\n", bad ? "FAIL" : "PASS");
  return bad ? 1 : 0;
}
