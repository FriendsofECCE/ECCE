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
#include "tdat/MoDiagram.H"

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

  //  --- a molecule that LOOKS like it has a centre and does not -----
  //
  //  Methanol in Cs.  Three of its atoms lie on the mirror plane, so
  //  "an atom the whole group leaves in place" exists and an earlier
  //  version nominated the oxygen, with the two out-of-plane methyl
  //  hydrogens as its terminal set.  That is a diagram of nothing that
  //  looks exactly like a diagram of methanol, which is worse than no
  //  diagram at all.
  {
    double c[] = {
      -0.0503,  0.6685,  0.0000,    // C
       0.0503, -0.7585,  0.0000,    // O
      -1.0807,  1.0104,  0.0000,    // H, in plane
       0.4400,  1.0967,  0.8900,    // H
       0.4400,  1.0967, -0.8900,    // H
       0.8750, -1.0211,  0.0000 };  // H on O, in plane
    vector<double> coords(c, c + 18);
    const char* e[] = { "C", "O", "H", "H", "H", "H" };
    vector<string> elements(e, e + 6);

    MoColumn left, right;
    string note;
    bool ok = MoFragments::build(coords, elements, "CS", left, right, note);
    bool refused = !ok && note.find("equivalent neighbours") != string::npos;
    printf("  %-46s %s\n",
           "CH3OH: not an AXn molecule, and it says so",
           refused ? "ok" : ("FAIL: " + (ok ? string("built anyway")
                                            : note)).c_str());
    if (!refused) bad++;
  }

  //  --- the spellings the codes actually use ------------------------
  //
  //  An exact string comparison between a code's label and a character
  //  table's is the silent-failure case this whole file exists to
  //  avoid: ORCA writes A" where the table writes A'', and every code
  //  prints a1 where a table prints A1.  Without canonicalisation half
  //  the levels find no partner and the diagram simply comes out with
  //  fewer correlation lines than it should, looking perfectly
  //  reasonable.
  {
    printf("\n  irrep spellings\n");
    check("ORCA's A\" is the table's A''",
          MoDiagram::canonicalIrrep("A\""), MoDiagram::canonicalIrrep("A''"));
    check("a1 matches A1", MoDiagram::canonicalIrrep("a1"),
          MoDiagram::canonicalIrrep("A1"));
    check("t2g matches T2g", MoDiagram::canonicalIrrep("t2g"),
          MoDiagram::canonicalIrrep("T2G"));
    check("surrounding space is ignored",
          MoDiagram::canonicalIrrep("  e' "), MoDiagram::canonicalIrrep("E'"));

    //  And distinct irreps must stay distinct: a normaliser that
    //  collapsed too much would connect levels that cannot mix, which
    //  is worse than connecting none.
    bool distinct =
        MoDiagram::canonicalIrrep("A1") != MoDiagram::canonicalIrrep("A2") &&
        MoDiagram::canonicalIrrep("A'") != MoDiagram::canonicalIrrep("A''") &&
        MoDiagram::canonicalIrrep("Eg") != MoDiagram::canonicalIrrep("Eu");
    printf("  %-46s %s\n", "distinct irreps stay distinct",
           distinct ? "ok" : "FAIL");
    if (!distinct) bad++;
  }

  //  --- connecting a real case --------------------------------------
  //  Methane again, with the centre column labelled the way a code
  //  reports it: lowercase.  Every molecular level must find both a
  //  carbon partner and a hydrogen one, because in Td there is nothing
  //  else for a1 or t2 to be.
  {
    const double d = 0.6276;
    double c[] = { 0,0,0,  d,d,d,  d,-d,-d,  -d,d,-d,  -d,-d,d };
    vector<double> coords(c, c + 15);
    const char* e[] = { "C", "H", "H", "H", "H" };
    vector<string> elements(e, e + 5);

    MoColumn left, right, centre;
    string note;
    MoFragments::build(coords, elements, "TD", left, right, note);

    double energies[] = { -20.0, -12.0, 2.0, 6.0 };
    const char* syms[] = { "a1", "t2", "a1", "t2" };
    vector<double> en(energies, energies + 4), occ(4, 2.0);
    vector<string> sy(syms, syms + 4);
    MoDiagram::group(en, occ, sy, 1e-6, centre.levels);

    vector<MoConnection> links;
    MoDiagram::connect(left.levels, centre.levels, right.levels, links);

    int both = 0;
    for (size_t i = 0; i < links.size(); i++) {
      if (links[i].leftLevel >= 0 && links[i].rightLevel >= 0) both++;
    }
    printf("  %-46s %d of %zu %s\n",
           "CH4: every MO connects to both sides", both, links.size(),
           (both == (int)links.size() && both == 4) ? "ok" : "FAIL");
    if (!(both == (int)links.size() && both == 4)) bad++;
  }

  printf("\n  %s\n", bad ? "FAIL" : "PASS");
  return bad ? 1 : 0;
}
