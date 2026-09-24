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
#include "tdat/SymmetryAnalysis.H"

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

  //  --- a diatomic: no central atom, and none needed -----------------
  //
  //  C2 is one orbit of two equivalent atoms, so there is no central
  //  atom at all -- and the diagram everyone draws for it exists
  //  anyway, one atom's orbitals on each side.  It splits down the
  //  middle.
  //
  //  The columns are the atom's OWN 2s and 2p, not symmetry orbitals:
  //  half the group's operations move that atom onto its partner, so
  //  asking which irreps its orbitals span has no answer.  The symmetry
  //  appears when they combine, which is the middle column.
  {
    double c[] = {
       0.0, 0.0,  0.6210,
       0.0, 0.0, -0.6210 };
    vector<double> coords(c, c + 6);
    const char* e[] = { "C", "C" };
    vector<string> elements(e, e + 2);

    MoColumn left, right;
    string note;
    bool ok = MoFragments::build(coords, elements, "D4H", 0, left, right, note);
    printf("  %-46s %s\n", "C2: builds as one atom a side",
           ok ? "ok" : ("FAIL: " + note).c_str());
    if (!ok) bad++;
    else {
      check("C2: left column is one carbon",  left.title,  "C");
      check("C2: right column is the other",  right.title, "C");
      bool shells = (left.levels.size() == 2 &&
                     left.levels[0].label == "2s" &&
                     left.levels[1].label == "2p" &&
                     left.levels[1].degeneracy == 3 &&
                     left.levels[0].irrep.empty());
      printf("  %-46s %s\n",
             "C2: 2s and 2p, atomic and unlabelled by irrep",
             shells ? "ok" : "FAIL");
      if (!shells) bad++;

      //  And nothing is called non-bonding just because there is
      //  nothing to count against: nitrogen came out with "nb" against
      //  all eight of its levels, including the two holding its triple
      //  bond.
      MoColumn centre;
      const char* cl[] = {"1sig","1siu","1piu","2sig"};
      for (int i = 0; i < 4; i++) {
        MoLevel l; l.label = cl[i]; l.irrep = MoDiagram::canonicalIrrep(cl[i]);
        l.degeneracy = 1; l.energy = -1.0 + 0.3*i;
        centre.levels.push_back(l);
      }
      MoDiagram::classify(left.levels, centre.levels, right.levels);
      bool quiet = true;
      for (size_t i = 0; i < centre.levels.size(); i++) {
        if (centre.levels[i].character != MoLevel::UNKNOWN ||
            centre.levels[i].label.find("nb") != string::npos) quiet = false;
      }
      printf("  %-46s %s\n",
             "C2: nothing is called nb for want of a count",
             quiet ? "ok" : "FAIL");
      if (!quiet) bad++;

      //  The fragment levels say which shell they are, because that is
      //  the only thing a diatomic can be connected on.
      bool shellsKnown = (left.levels[0].shell == 0 &&
                          left.levels[1].shell == 1 &&
                          right.levels[0].shell == 0);
      printf("  %-46s %s\n", "C2: each level knows its shell",
             shellsKnown ? "ok" : "FAIL");
      if (!shellsKnown) bad++;

      //  And connect() uses it.  Give the sigma orbital a composition
      //  that is mostly s and the pi orbital one that is all p, and
      //  each should reach only the level it came from.
      for (size_t i = 0; i < centre.levels.size(); i++) {
        centre.levels[i].shellLeft.assign(3, 0.0);
        centre.levels[i].shellRight.assign(3, 0.0);
      }
      centre.levels[0].shellLeft[0] = centre.levels[0].shellRight[0] = 0.45;
      centre.levels[0].shellLeft[1] = centre.levels[0].shellRight[1] = 0.02;
      centre.levels[2].shellLeft[1] = centre.levels[2].shellRight[1] = 0.50;

      vector<MoConnection> links;
      MoDiagram::connect(left.levels, centre.levels, right.levels, links);

      bool sigmaToS = false, sigmaToP = false, piToS = false, piToP = false;
      for (size_t i = 0; i < links.size(); i++) {
        const int f = links[i].leftLevel;
        if (f < 0) continue;
        const int shell = left.levels[f].shell;
        if (links[i].centreLevel == 0) {
          if (shell == 0) sigmaToS = true;
          if (shell == 1) sigmaToP = true;
        }
        if (links[i].centreLevel == 2) {
          if (shell == 0) piToS = true;
          if (shell == 1) piToP = true;
        }
      }
      const bool byShell = sigmaToS && !sigmaToP && piToP && !piToS;
      printf("  %-46s %s\n",
             "C2: sigma reaches 2s, pi reaches only 2p",
             byShell ? "ok" : "FAIL");
      if (!byShell) bad++;
    }
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
    //  NOT AN AXn MOLECULE, AND IT SAYS SO.
    //
    //  It used to be refused outright.  It is now drawn as the carbon
    //  against everything else -- which is one of the two splits a
    //  person actually uses for methanol, "H4O against C" -- but the
    //  program CHOSE that, so it has to say so: three orbits admit
    //  more than one fragmentation and the reader may want a
    //  different one.
    bool ok = MoFragments::build(coords, elements, "CS", 0, left, right, note);
    const bool saysSo = note.find("guess at the fragments") != string::npos;
    printf("  %-46s %s\n",
           "CH3OH: built on a stated guess, not silently",
           (ok && saysSo) ? "ok"
                          : ("FAIL: " + (ok ? note
                                            : string("refused"))).c_str());
    if (!(ok && saysSo)) bad++;
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

    //  A GENUINE DISAGREEMENT IS A DEFICIT, NOT A SURPLUS.
    //
    //  The fragment model is minimal-valence and a real calculation
    //  is not, so the calculation reporting MORE of an irrep than the
    //  fragments span is the normal case, not an error -- even 3-21G
    //  gives cobalt more virtuals than the model has, and treating
    //  that as a disagreement left CoH6(2+) with no correlation lines
    //  at all.  What cannot be right is the calculation reporting
    //  FEWER than the fragments span: those orbitals have to exist
    //  somewhere, and no exchange of two irreps can conjure them.
    MoColumn surplusL, surplusC, empty;
    for (int i = 0; i < 2; i++) {
      MoLevel l; l.irrep = "A1"; l.degeneracy = 1;
      surplusL.levels.push_back(l);
    }
    for (int i = 0; i < 3; i++) {
      MoLevel l; l.irrep = "A1"; l.degeneracy = 1;
      surplusC.levels.push_back(l);
    }
    string why2;
    const bool accepted = MoDiagram::reconcile(surplusL.levels, empty.levels,
                                               surplusC.levels, why2);
    printf("  %-46s %s\n", "a surplus of virtuals is not a disagreement",
           accepted ? "ok" : ("FAIL: " + why2).c_str());
    if (!accepted) bad++;

    MoColumn shortL, shortC;
    for (int i = 0; i < 3; i++) {
      MoLevel l; l.irrep = "A1"; l.degeneracy = 1;
      shortL.levels.push_back(l);
    }
    for (int i = 0; i < 2; i++) {
      MoLevel l; l.irrep = "A1"; l.degeneracy = 1;
      shortC.levels.push_back(l);
    }
    MoColumn empty2;
    string why3;
    const bool refused = !MoDiagram::reconcile(shortL.levels, empty2.levels,
                                               shortC.levels, why3);
    printf("  %-46s %s\n", "a deficit is reported",
           refused ? "ok" : "FAIL");
    if (!refused) bad++;
  }

  //  --- choosing the fragments yourself ------------------------------
  //
  //  Methanol in Cs is refused by the automatic split: five
  //  symmetry-distinct sets of atoms and no central-atom-plus-
  //  neighbours structure among them.  Saying which sets go where
  //  builds it anyway, which is the whole point -- and the choice is
  //  made by grouping ORBITS, so an impossible fragmentation cannot
  //  be expressed.
  {
    printf("\n  choosing the fragments\n");

    double c[] = {
      -0.0503,  0.6685,  0.0000,    // C
       0.0503, -0.7585,  0.0000,    // O
      -1.0807,  1.0104,  0.0000,    // H, in plane
       0.4400,  1.0967,  0.8900,    // H
       0.4400,  1.0967, -0.8900,    // H
       0.8750, -1.0211,  0.0000 };  // H on O
    vector<double> coords(c, c + 18);
    const char* e[] = { "C", "O", "H", "H", "H", "H" };
    vector<string> elements(e, e + 6);

    MoColumn left, right;
    string note;
    //  Without a choice it falls back to its own guess and says so;
    //  the point of the chooser below is to override that.
    const bool guessed =
        MoFragments::build(coords, elements, "CS", 0, left, right, note) &&
        note.find("guess at the fragments") != string::npos;
    printf("  %-46s %s\n", "CH3OH: guessed, and the guess is stated",
           guessed ? "ok" : "FAIL");
    if (!guessed) bad++;

    vector< vector<int> > orbits;
    string why;
    const bool got = MoFragments::orbitsOf(coords, elements, "CS",
                                           orbits, why);
    printf("  %-46s %zu %s\n", "CH3OH: its orbits can be listed",
           orbits.size(), got ? "ok" : ("FAIL: " + why).c_str());
    if (!got) bad++;
    else {
      //  Put the oxygen on one side and everything else on the other.
      //  Any grouping of the orbits is valid by construction; this one
      //  is just a grouping a person might choose.
      vector<int> side(orbits.size(), 1);
      for (size_t i = 0; i < orbits.size(); i++) {
        if (orbits[i].size() == 1 && elements[orbits[i][0]] == "O") side[i] = 0;
      }

      MoColumn l2, r2;
      string note2;
      vector<int> leftAtoms, rightAtoms;
      const bool built = MoFragments::build(coords, elements, "CS", 0,
                                            l2, r2, note2,
                                            &leftAtoms, &rightAtoms, &side);
      printf("  %-46s %s\n", "CH3OH: and built once they are grouped",
             built ? "ok" : ("FAIL: " + note2).c_str());
      if (!built) bad++;
      else {
        check("CH3OH: the oxygen is its own column", l2.title, "O");
        printf("  %-46s %-14s %s\n",
               "CH3OH: the rest are symmetry orbitals", r2.title.c_str(),
               (r2.title.find("TASOs") != string::npos) ? "ok" : "FAIL");
        if (r2.title.find("TASOs") == string::npos) bad++;

        //  Fourteen valence electrons: C 4, O 6, four H.  The columns
        //  carry them between them however the atoms are grouped.
        double total = 0.0;
        for (size_t i = 0; i < l2.levels.size(); i++)  total += l2.levels[i].occupancy;
        for (size_t i = 0; i < r2.levels.size(); i++) total += r2.levels[i].occupancy;
        checkd("CH3OH: and all fourteen valence electrons", total, 14.0, 1e-9);

        //  A MIXED FRAGMENT BRINGS EACH ELEMENT'S OWN SHELLS.  Taking
        //  the first atom's element for the whole set gave the four
        //  hydrogens a 2p they do not have, and counted fifteen p
        //  functions where a valence picture has three.
        int functions = 0;
        bool hydrogenP = false;
        for (size_t i = 0; i < r2.levels.size(); i++) {
          functions += r2.levels[i].degeneracy;
          if (r2.levels[i].label.find("p H") != string::npos) hydrogenP = true;
        }
        printf("  %-46s %d %s\n",
               "CH3OH: CH4 is eight valence functions", functions,
               (functions == 8) ? "ok" : "FAIL");
        if (functions != 8) bad++;
        printf("  %-46s %s\n", "CH3OH: and no hydrogen 2p among them",
               hydrogenP ? "FAIL" : "ok");
        if (hydrogenP) bad++;

        //  IN ENERGY ORDER, because the aufbau filling walks the list
        //  from the start and the drawing groups neighbours.  Built
        //  one element at a time they came out grouped by element, and
        //  carbon's 2p filled before the hydrogens' 1s below it.
        bool ordered = true;
        for (size_t i = 1; i < r2.levels.size(); i++) {
          if (r2.levels[i].energy < r2.levels[i-1].energy) ordered = false;
        }
        printf("  %-46s %s\n", "CH3OH: the levels are in energy order",
               ordered ? "ok" : "FAIL");
        if (!ordered) bad++;
      }

      //  Everything on one side is not a diagram, and says so.
      vector<int> lopsided(orbits.size(), 1);
      MoColumn l3, r3;
      string note3;
      const bool rejected = !MoFragments::build(coords, elements, "CS", 0,
                                                l3, r3, note3, 0, 0,
                                                &lopsided);
      printf("  %-46s %s\n", "CH3OH: an empty side is refused",
             rejected ? "ok" : "FAIL");
      if (!rejected) bad++;
    }
  }

  //  --- the phase patterns a diagram sketches ------------------------
  {
    printf("\n  phase patterns\n");

    const double d = 0.6276;
    double c[] = { 0,0,0,  d,d,d,  d,-d,-d,  -d,d,-d,  -d,-d,d };
    vector<double> coords(c, c + 15);
    const char* e[] = { "C", "H", "H", "H", "H" };
    vector<string> elements(e, e + 5);

    MoColumn left, right;
    string note;
    MoFragments::build(coords, elements, "TD", 0, left, right, note);

    //  The totally symmetric combination is every hydrogen in phase.
    //  Any other answer means the projection operator, the class
    //  matching or the orbit is wrong.
    bool a1 = false, t2 = false;
    for (size_t i = 0; i < right.levels.size(); i++) {
      const vector<double>& p = right.levels[i].phases;
      if (p.size() != 4) continue;

      bool allSame = true;
      for (size_t k = 1; k < p.size(); k++) {
        if ((p[k] > 0) != (p[0] > 0)) allSame = false;
      }
      if (right.levels[i].irrep == "A1") a1 = allSame;
      if (right.levels[i].irrep == "T2") t2 = !allSame;
    }
    printf("  %-46s %s\n", "CH4: the a1 TASO is all four in phase",
           a1 ? "ok" : "FAIL");
    printf("  %-46s %s\n", "CH4: a t2 TASO is not",
           t2 ? "ok" : "FAIL");
    if (!a1) bad++;
    if (!t2) bad++;

    //  A symmetry orbital is normalised, so its coefficients square to
    //  one -- which is also what makes the circle sizes mean anything.
    double norm = 0.0;
    for (size_t i = 0; i < right.levels.size(); i++) {
      if (right.levels[i].irrep != "A1") continue;
      const vector<double>& p = right.levels[i].phases;
      for (size_t k = 0; k < p.size(); k++) norm += p[k]*p[k];
    }
    checkd("CH4: and it is normalised", norm, 1.0, 1e-9);

    //  Four hydrogens flattened for the sketch: they must not collapse
    //  onto one another, or the picture shows fewer atoms than there
    //  are.
    bool spread = (right.sketchX.size() == 4);
    for (size_t i = 0; spread && i < 4; i++) {
      for (size_t j = i+1; spread && j < 4; j++) {
        if (fabs(right.sketchX[i] - right.sketchX[j]) < 1e-6 &&
            fabs(right.sketchY[i] - right.sketchY[j]) < 1e-6) spread = false;
      }
    }
    printf("  %-46s %s\n", "CH4: the four sketch positions are distinct",
           spread ? "ok" : "FAIL");
    if (!spread) bad++;

    //  A p shell carries no pattern: projectOrbit puts ONE orbital on
    //  each atom, which is what an s shell is, and drawing an s
    //  pattern beside a p level would be a confident lie.
    bool pQuiet = true;
    for (size_t i = 0; i < right.levels.size(); i++) {
      if (right.levels[i].shell == 1 && !right.levels[i].phases.empty()) {
        pQuiet = false;
      }
    }
    printf("  %-46s %s\n", "a p shell claims no pattern",
           pQuiet ? "ok" : "FAIL");
    if (!pQuiet) bad++;
  }

  //  --- p-orbital symmetry orbitals ---------------------------------
  //
  //  The check that says whether the projection is right without
  //  anyone knowing the answer: the number of independent vectors it
  //  returns for an irrep must equal that irrep's multiplicity in the
  //  character reduction, times its dimension.  Two routes to the same
  //  number -- one projecting orbitals and orthogonalising, the other
  //  reducing a character -- and neither is told the other's result.
  {
    printf("\n  p-orbital symmetry orbitals\n");

    //  Nitrite: the compendium works this exact set out by hand and
    //  gets 2a1 + a2 + 2b1 + b2 for the two oxygens' 2p orbitals.
    double c[] = { 0,0,0,  1.0544,0,-0.6717,  -1.0544,0,-0.6717 };
    vector<double> coords(c, c + 9);
    const char* e[] = { "N", "O", "O" };
    vector<string> elements(e, e + 3);

    const CharacterTable *table = CharacterTable::lookup("C2V");
    vector<SymOp> ops;
    MoFragments::symmetryOperations("C2V", ops);

    vector< vector<int> > images;
    vector< vector<int> > classes;
    vector<int> classOfOp;
    bool ready = table != 0 &&
        SymmetryAnalysis::atomImages(coords, elements, ops, 1e-3, images);
    if (ready) {
      SymmetryAnalysis::conjugacyClasses(ops, classes);
      ready = SymmetryAnalysis::matchClasses(ops, classes, *table, classOfOp);
    }
    printf("  %-46s %s\n", "NO2-: the C2v frame is usable",
           ready ? "ok" : "FAIL");
    if (!ready) bad++;
    else {
      vector<int> terminal;
      terminal.push_back(1);
      terminal.push_back(2);

      //  What the character says the p set spans.
      vector< vector<int> > shells(3);
      shells[1].push_back(1);
      shells[2].push_back(1);
      vector<double> chi;
      vector<int> multiplicity;
      SymmetryAnalysis::basisCharacter(shells, images, classOfOp, ops,
                                       (int)table->classes().size(),
                                       false, chi);
      table->reduce(chi, multiplicity);

      const vector<string>& irreps = table->irreps();
      int totalVectors = 0, totalPredicted = 0;
      bool matched = true;
      for (size_t i = 0; i < irreps.size(); i++) {
        vector< vector<double> > vectors;
        SymmetryAnalysis::projectVectorOrbit(terminal, images, classOfOp,
                                             ops, *table, irreps[i],
                                             vectors);
        const int predicted = multiplicity[i]*table->dimension(irreps[i]);
        totalVectors += (int)vectors.size();
        totalPredicted += predicted;
        if ((int)vectors.size() != predicted) {
          matched = false;
          printf("    %-8s projected %zu, the character says %d\n",
                 irreps[i].c_str(), vectors.size(), predicted);
        }
      }
      printf("  %-46s %d and %d %s\n",
             "NO2-: projected vectors match the character",
             totalVectors, totalPredicted, matched ? "ok" : "FAIL");
      if (!matched) bad++;

      //  Six p orbitals on two oxygens, so six vectors in all.
      printf("  %-46s %d %s\n", "NO2-: and there are six of them",
             totalVectors, (totalVectors == 6) ? "ok" : "FAIL");
      if (totalVectors != 6) bad++;

      //  Each is normalised and they are mutually orthogonal, which is
      //  what Gram-Schmidt was for and what makes them drawable.
      vector< vector<double> > all;
      for (size_t i = 0; i < irreps.size(); i++) {
        vector< vector<double> > vectors;
        SymmetryAnalysis::projectVectorOrbit(terminal, images, classOfOp,
                                             ops, *table, irreps[i],
                                             vectors);
        for (size_t k = 0; k < vectors.size(); k++) all.push_back(vectors[k]);
      }
      double worstNorm = 0.0, worstDot = 0.0;
      for (size_t i = 0; i < all.size(); i++) {
        double norm = 0.0;
        for (size_t k = 0; k < all[i].size(); k++) norm += all[i][k]*all[i][k];
        if (fabs(norm - 1.0) > worstNorm) worstNorm = fabs(norm - 1.0);
        for (size_t j = i+1; j < all.size(); j++) {
          double dot = 0.0;
          for (size_t k = 0; k < all[i].size(); k++) dot += all[i][k]*all[j][k];
          if (fabs(dot) > worstDot) worstDot = fabs(dot);
        }
      }
      checkd("NO2-: every vector is normalised", worstNorm, 0.0, 1e-9);
      checkd("NO2-: and they are mutually orthogonal", worstDot, 0.0, 1e-9);

      //  THE DEFINITION, CHECKED DIRECTLY.  A one-dimensional irrep's
      //  symmetry orbital must come back as chi(R) times itself under
      //  every operation.  Nothing else about the projection needs to
      //  be believed if this holds, and no expected answer is
      //  involved.
      //
      //  It also settles arguments.  Reading the output by eye, the
      //  a2 combination looked like it was built from the wrong
      //  component and the whole projection looked wrong; this said
      //  otherwise, exactly, and the fault was in the reading.
      double worstMove = 0.0;
      for (size_t i = 0; i < irreps.size(); i++) {
        if (table->dimension(irreps[i]) != 1) continue;
        vector< vector<double> > vectors;
        SymmetryAnalysis::projectVectorOrbit(terminal, images, classOfOp,
                                             ops, *table, irreps[i],
                                             vectors);
        const vector<double>* chi = table->characters(irreps[i]);
        for (size_t v = 0; v < vectors.size(); v++) {
          for (size_t o = 0; o < ops.size(); o++) {
            vector<double> moved(vectors[v].size(), 0.0);
            for (size_t a = 0; a < terminal.size(); a++) {
              const size_t to =
                  (images[o][terminal[a]] == terminal[0]) ? 0 : 1;
              for (int k = 0; k < 3; k++) {
                for (int kp = 0; kp < 3; kp++) {
                  moved[3*to + kp] += ops[o].m[kp][k]*vectors[v][3*a + k];
                }
              }
            }
            const double want = (*chi)[classOfOp[o]];
            for (size_t k = 0; k < moved.size(); k++) {
              const double d = fabs(moved[k] - want*vectors[v][k]);
              if (d > worstMove) worstMove = d;
            }
          }
        }
      }
      checkd("NO2-: and each transforms with its own character",
             worstMove, 0.0, 1e-9);
    }
  }

  //  --- degenerate sets span an invariant subspace -------------------
  //
  //  The one-dimensional check above does not reach an e or a t: their
  //  members do not come back as multiples of themselves, they mix
  //  into each other.  What is true of ANY irrep is that its set spans
  //  a subspace the group cannot leave -- apply an operation to a
  //  member and the result is still a combination of the set.
  //
  //  This matters because a degenerate irrep is where projection and
  //  Gram-Schmidt are most likely to be subtly wrong, and the counting
  //  check cannot see it: a wrong basis of the right-sized subspace
  //  has exactly the right number of vectors.
  {
    printf("\n  degenerate symmetry orbitals\n");

    const double dd = 0.6276;
    double c[] = { 0,0,0,  dd,dd,dd,  dd,-dd,-dd,  -dd,dd,-dd,  -dd,-dd,dd };
    vector<double> coords(c, c + 15);
    const char* e[] = { "C", "H", "H", "H", "H" };
    vector<string> elements(e, e + 5);

    const CharacterTable *table = CharacterTable::lookup("TD");
    vector<SymOp> ops;
    MoFragments::symmetryOperations("TD", ops);
    vector< vector<int> > images, classes;
    vector<int> classOfOp;
    bool ready = table != 0 &&
        SymmetryAnalysis::atomImages(coords, elements, ops, 1e-3, images);
    if (ready) {
      SymmetryAnalysis::conjugacyClasses(ops, classes);
      ready = SymmetryAnalysis::matchClasses(ops, classes, *table, classOfOp);
    }
    printf("  %-46s %s\n", "CH4: the Td frame is usable", ready ? "ok" : "FAIL");
    if (!ready) bad++;
    else {
      vector<int> orbit;
      for (int a = 1; a <= 4; a++) orbit.push_back(a);

      double worst = 0.0;
      int checked = 0;
      const vector<string>& irreps = table->irreps();
      for (size_t i = 0; i < irreps.size(); i++) {
        vector< vector<double> > vs;
        if (!SymmetryAnalysis::projectOrbit(orbit, images, classOfOp,
                                            *table, irreps[i], vs)) continue;
        if (vs.size() < 2) continue;         // the 1-D check covers these

        for (size_t v = 0; v < vs.size(); v++) {
          for (size_t o = 0; o < ops.size(); o++) {
            //  Move the vector: atom a's coefficient goes to its image.
            vector<double> moved(vs[v].size(), 0.0);
            for (size_t a = 0; a < orbit.size(); a++) {
              int to = -1;
              for (size_t k = 0; k < orbit.size(); k++) {
                if (orbit[k] == images[o][orbit[a]]) to = (int)k;
              }
              if (to < 0) continue;
              moved[to] += vs[v][a];
            }
            //  Take away everything the set spans.  Nothing may be left.
            for (size_t j = 0; j < vs.size(); j++) {
              double dot = 0.0;
              for (size_t k = 0; k < moved.size(); k++) dot += moved[k]*vs[j][k];
              for (size_t k = 0; k < moved.size(); k++) moved[k] -= dot*vs[j][k];
            }
            for (size_t k = 0; k < moved.size(); k++) {
              if (fabs(moved[k]) > worst) worst = fabs(moved[k]);
            }
          }
          checked++;
        }
      }
      printf("  %-46s %d vectors\n",
             "CH4: the t2 set was checked", checked);
      checkd("CH4: no operation takes it out of its own span",
             worst, 0.0, 1e-9);
      if (checked == 0) {
        printf("  %-46s FAIL (nothing degenerate was found)\n",
               "CH4: and there was something to check");
        bad++;
      }
    }
  }

  //  --- how much of an orbital sits on each fragment ----------------
  {
    printf("\n  composition\n");

    //  Three atoms, one function on the first and two on each of the
    //  others: five functions in all.
    int fpa[] = {1, 2, 2};
    vector<int> perAtom(fpa, fpa + 3);
    int one[] = {0};
    int two[] = {1, 2};
    vector<int> first(one, one + 1), rest(two, two + 2);
    const vector<double> noOverlap;

    //  All on the first atom.
    double a[] = {1.0, 0.0, 0.0, 0.0, 0.0};
    vector<double> ca(a, a + 5);
    checkd("entirely on the first atom",
           MoFragments::share(ca, perAtom, first, noOverlap), 1.0, 1e-12);
    checkd("and nothing on the others",
           MoFragments::share(ca, perAtom, rest, noOverlap), 0.0, 1e-12);

    //  Half and half, in an orthonormal basis.
    const double h = 0.5;
    double b[] = {h, h, h, h, h};
    vector<double> cb(b, b + 5);
    checkd("one of five functions is a fifth of it",
           MoFragments::share(cb, perAtom, first, noOverlap), 0.2, 1e-12);
    checkd("the other four are the rest",
           MoFragments::share(cb, perAtom, rest, noOverlap), 0.8, 1e-12);

    //  The shares of a partition must add to one, whatever the
    //  coefficients: that is what makes a cutoff mean anything.
    double c3[] = {0.3, -0.7, 0.1, 0.55, -0.2};
    vector<double> cc(c3, c3 + 5);
    checkd("the two shares add to one",
           MoFragments::share(cc, perAtom, first, noOverlap) +
           MoFragments::share(cc, perAtom, rest, noOverlap), 1.0, 1e-12);

    //  WITH an overlap matrix the populations are Mulliken ones and
    //  still add to one, which is the property the cutoff relies on.
    vector<double> S(25, 0.0);
    for (int i = 0; i < 5; i++) S[i*5 + i] = 1.0;
    S[0*5 + 1] = S[1*5 + 0] = 0.3;      // the two fragments overlap
    S[2*5 + 3] = S[3*5 + 2] = -0.15;
    checkd("with overlap, the shares still add to one",
           MoFragments::share(cc, perAtom, first, S) +
           MoFragments::share(cc, perAtom, rest, S), 1.0, 1e-12);

    //  A mapping that does not account for every function is refused,
    //  because an off-by-one there gives a plausible number instead of
    //  an error.
    int wrong[] = {1, 2};
    vector<int> shortMap(wrong, wrong + 2);
    const bool refused =
        MoFragments::share(cc, shortMap, first, noOverlap) < 0.0;
    printf("  %-46s %s\n", "a mapping that misses functions is refused",
           refused ? "ok" : "FAIL");
    if (!refused) bad++;
  }

  printf("\n  %s\n", bad ? "FAIL" : "PASS");
  return bad ? 1 : 0;
}
