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
#include <map>
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
    bool ok = MoFragments::build(coords, elements, "TD", 0, left, right, note);
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
    bool ok = MoFragments::build(coords, elements, "C2V", 0, left, right, note);
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

      //  The columns are named by the atoms they belong to.
      check("H2O: left column is the oxygen",  left.title,  "O");
      check("H2O: right column is the TASOs",  right.title, "2H TASOs");
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
    bool ok = MoFragments::build(coords, elements, "D2H", 0, left, right, note);
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
    bool ok = MoFragments::build(coords, elements, "CS", 0, left, right, note);
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
    MoFragments::build(coords, elements, "TD", 0, left, right, note);

    double energies[] = { -20.0, -12.0, 2.0, 6.0 };
    const char* syms[] = { "a1", "t2", "a1", "t2" };
    vector<double> en(energies, energies + 4), occ(4, 2.0);
    vector<string> sy(syms, syms + 4);
    MoDiagram::group(en, occ, sy, 1e-6, centre.levels);

    vector<MoConnection> links;
    MoDiagram::connect(left.levels, centre.levels, right.levels, links);

    //  One record per fragment level reached, not one per molecular
    //  orbital: an MO mixes with EVERY fragment level of its irrep,
    //  which is what symmetry mixing means and what a single record
    //  per MO could not express.
    MoDiagram::classify(left.levels, centre.levels, right.levels);
    links.clear();
    MoDiagram::connect(left.levels, centre.levels, right.levels, links);

    int reachedLeft = 0, reachedRight = 0;
    for (size_t c = 0; c < centre.levels.size(); c++) {
      bool l = false, r = false;
      for (size_t i = 0; i < links.size(); i++) {
        if (links[i].centreLevel != (int)c) continue;
        if (links[i].leftLevel  >= 0) l = true;
        if (links[i].rightLevel >= 0) r = true;
      }
      if (l) reachedLeft++;
      if (r) reachedRight++;
    }
    const bool allConnect = (reachedLeft == (int)centre.levels.size() &&
                             reachedRight == (int)centre.levels.size());
    printf("  %-46s %d and %d of %zu %s\n",
           "CH4: every MO reaches both sides",
           reachedLeft, reachedRight, centre.levels.size(),
           allConnect ? "ok" : "FAIL");
    if (!allConnect) bad++;
  }

  //  --- everything drawn must be valid UTF-8 -----------------------
  //
  //  wxString(..., wxConvUTF8) returns an EMPTY string for invalid
  //  input rather than something mangled, so a stray byte does not
  //  corrupt a label, it deletes it.  The multiplication sign was
  //  written as the single byte 0xd7, which is not valid UTF-8, and
  //  every label and title carrying it rendered blank.
  {
    const double d = 0.6276;
    double c[] = { 0,0,0,  d,d,d,  d,-d,-d,  -d,d,-d,  -d,-d,d };
    vector<double> coords(c, c + 15);
    const char* e[] = { "C", "H", "H", "H", "H" };
    vector<string> elements(e, e + 5);

    MoColumn left, right;
    string note;
    MoFragments::build(coords, elements, "TD", 0, left, right, note);

    vector<string> texts;
    texts.push_back(left.title);
    texts.push_back(right.title);
    for (size_t i = 0; i < left.levels.size(); i++)
      texts.push_back(left.levels[i].label);
    for (size_t i = 0; i < right.levels.size(); i++)
      texts.push_back(right.levels[i].label);

    bool valid = true;
    for (size_t t = 0; t < texts.size(); t++) {
      const string& u = texts[t];
      for (size_t i = 0; i < u.size(); ) {
        const unsigned char b = (unsigned char)u[i];
        int extra = 0;
        if      (b < 0x80)            extra = 0;
        else if ((b & 0xE0) == 0xC0)  extra = 1;
        else if ((b & 0xF0) == 0xE0)  extra = 2;
        else if ((b & 0xF8) == 0xF0)  extra = 3;
        else { valid = false; break; }
        if (i + extra >= u.size()) { valid = false; break; }
        for (int k = 1; k <= extra; k++) {
          if (((unsigned char)u[i+k] & 0xC0) != 0x80) { valid = false; break; }
        }
        if (!valid) break;
        i += extra + 1;
      }
      if (!valid) {
        printf("  not valid UTF-8: %s\n", u.c_str());
        break;
      }
    }
    printf("  %-46s %s\n", "every drawn string is valid UTF-8",
           valid ? "ok" : "FAIL");
    if (!valid) bad++;
  }

  //  --- against the course's own worked answers ---------------------
  //
  //  Not against what I expect the code to do.  5KE195's workshop 3
  //  works NO2- and CCl4 through by hand and states the results, so
  //  they are an independent standard: if the code disagrees with the
  //  course it is meant to support, the code is wrong.
  {
    printf("\n  against 5KE195 workshop 3\n");

    //  NO2-, bent C2v.  The compendium: "Two terminal atoms, three p
    //  orbitals each: Gamma(sigma+pi) ... reduces to 2a1 + a2 + 2b1 +
    //  b2", and the sigma-only piece is a1 + b1.
    double c[] = { 0,0,0,  1.0544,0,-0.6717,  -1.0544,0,-0.6717 };
    vector<double> coords(c, c + 9);
    const char* e[] = { "N", "O", "O" };
    vector<string> elements(e, e + 3);

    MoColumn left, right;
    string note;
    bool ok = MoFragments::build(coords, elements, "C2V", -1,
                                 left, right, note);
    if (!ok) { printf("  NO2- did not build: %s\n", note.c_str()); bad++; }
    else {
      //  Count orbitals per irrep in the oxygen 2s and 2p sets.
      map<string,int> s2, p2;
      for (size_t i = 0; i < right.levels.size(); i++) {
        const MoLevel& l = right.levels[i];
        const bool isS = l.label.find("(2s)") != string::npos;
        (isS ? s2 : p2)[l.irrep] += l.degeneracy;
      }
      bool sigma = (s2.size() == 2 && s2["A1"] == 1 && s2["B1"] == 1);
      bool all   = (p2.size() == 4 && p2["A1"] == 2 && p2["A2"] == 1 &&
                    p2["B1"] == 2 && p2["B2"] == 1);
      printf("  %-46s %s\n",
             "NO2-: O 2s TASOs are a1 + b1", sigma ? "ok" : "FAIL");
      printf("  %-46s %s\n",
             "NO2-: O 2p TASOs are 2a1 + a2 + 2b1 + b2", all ? "ok" : "FAIL");
      if (!sigma) bad++;
      if (!all) bad++;

      //  18 valence electrons: 5 from N, 6 from each O, 1 for the
      //  charge.  The compendium counts them the same way.
      double total = 0.0;
      for (size_t i = 0; i < left.levels.size(); i++)  total += left.levels[i].occupancy;
      for (size_t i = 0; i < right.levels.size(); i++) total += right.levels[i].occupancy;
      printf("  %-46s %g %s\n", "NO2-: 18 valence electrons", total,
             (total == 18.0) ? "ok" : "FAIL");
      if (total != 18.0) bad++;
    }
  }

  //  --- classification, and the a2 the compendium singles out --------
  {
    printf("\n  bonding, non-bonding, antibonding\n");

    //  Water: for a1 the oxygen brings 2s and 2pz and the hydrogens
    //  one TASO, so one bonding, one antibonding and one left over;
    //  for b1 the oxygen brings 2px and the hydrogens nothing, so the
    //  single b1 can only be non-bonding.  That is the lone pair.
    MoColumn left, centre, right;
    const char* li[] = {"A1","A1","B1","B2"};
    for (int i = 0; i < 4; i++) {
      MoLevel l; l.irrep = li[i]; l.degeneracy = 1; left.levels.push_back(l);
    }
    const char* ri[] = {"A1","B2"};
    for (int i = 0; i < 2; i++) {
      MoLevel l; l.irrep = ri[i]; l.degeneracy = 1; right.levels.push_back(l);
    }
    const char* ci[] = {"A1","B2","A1","B1","A1","B2"};
    const char* cl[] = {"1a1","1b2","2a1","1b1","3a1","2b2"};
    for (int i = 0; i < 6; i++) {
      MoLevel l; l.irrep = ci[i]; l.label = cl[i]; l.degeneracy = 1;
      l.energy = -1.0 + 0.2*i;
      centre.levels.push_back(l);
    }

    MoDiagram::classify(left.levels, centre.levels, right.levels);

    struct { const char* label; int want; } expect[] = {
      {"1a1",  MoLevel::BONDING},     {"1b2",  MoLevel::BONDING},
      {"2a1 nb", MoLevel::NONBONDING}, {"1b1 nb", MoLevel::NONBONDING},
      {"3a1*", MoLevel::ANTIBONDING}, {"2b2*", MoLevel::ANTIBONDING}};
    for (int i = 0; i < 6; i++) {
      const bool got = (centre.levels[i].character == expect[i].want) &&
                       (centre.levels[i].label == expect[i].label);
      printf("  %-46s %-10s %s\n",
             i == 3 ? "H2O: 1b1 is the non-bonding lone pair"
                    : "H2O: level classified and labelled",
             centre.levels[i].label.c_str(), got ? "ok" : "FAIL");
      if (!got) bad++;
    }

    //  A bonding level and its antibonding partner share a number, so
    //  a diagram can colour them together.
    bool paired = (centre.levels[0].pairing == centre.levels[4].pairing &&
                   centre.levels[1].pairing == centre.levels[5].pairing &&
                   centre.levels[0].pairing != centre.levels[1].pairing);
    printf("  %-46s %s\n", "H2O: each bonding level pairs with its own",
           paired ? "ok" : "FAIL");
    if (!paired) bad++;
  }

  //  --- the axis convention ------------------------------------------
  {
    printf("\n  reconciling axis conventions\n");

    //  The compendium says it outright: "Whether you end up writing
    //  a1+b1 or a1+b2 just depends on which in-plane axis you decide
    //  to call x and which y -- either is fine, as long as you're
    //  consistent."  The two sources here are NOT consistent, and
    //  compared as they stand water comes out inside out.
    MoColumn left, centre, right;
    const char* li[] = {"A1","A1","B1","B2"};
    for (int i = 0; i < 4; i++) {
      MoLevel l; l.irrep = li[i]; l.label = string(li[i]) + "  (2p)";
      l.degeneracy = 1; left.levels.push_back(l);
    }
    const char* ri[] = {"A1","B1"};        // the table's frame
    for (int i = 0; i < 2; i++) {
      MoLevel l; l.irrep = ri[i]; l.label = string(ri[i]) + "  (1s)";
      l.degeneracy = 1; right.levels.push_back(l);
    }
    const char* ci[] = {"A1","B2","A1","B1","A1","B2"};   // the code's
    for (int i = 0; i < 6; i++) {
      MoLevel l; l.irrep = ci[i]; l.degeneracy = 1; centre.levels.push_back(l);
    }

    string why;
    bool ok = MoDiagram::reconcile(left.levels, right.levels,
                                   centre.levels, why);
    printf("  %-46s %s\n", "H2O: b1/b2 exchange found",
           ok ? "ok" : ("FAIL: " + why).c_str());
    if (!ok) bad++;
    else {
      //  After the exchange the hydrogens' antisymmetric combination
      //  is b2, matching the code, and the label follows the irrep.
      bool fixed = (right.levels[1].irrep == "B2" &&
                    right.levels[1].label.find("B2") != string::npos);
      printf("  %-46s %s  (%s)\n", "H2O: the label follows the irrep",
             fixed ? "ok" : "FAIL", right.levels[1].label.c_str());
      if (!fixed) bad++;
    }

    //  And a genuine disagreement is reported, not papered over: no
    //  exchange of two irreps can turn 2A1 into 3A1.
    MoColumn l2, c2, r2;
    for (int i = 0; i < 2; i++) {
      MoLevel l; l.irrep = "A1"; l.degeneracy = 1; l2.levels.push_back(l);
    }
    for (int i = 0; i < 3; i++) {
      MoLevel l; l.irrep = "A1"; l.degeneracy = 1; c2.levels.push_back(l);
    }
    string why2;
    bool refused = !MoDiagram::reconcile(l2.levels, r2.levels,
                                         c2.levels, why2);
    printf("  %-46s %s\n", "a real disagreement is reported",
           refused ? "ok" : "FAIL");
    if (!refused) bad++;
  }

  printf("\n  %s\n", bad ? "FAIL" : "PASS");
  return bad ? 1 : 0;
}
