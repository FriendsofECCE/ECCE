#include <cstdarg>
//  The C++ character table loader.
//
//  run_tests.py already proves the DATA is sound, by orthogonality.
//  What is checked here is that the loader reads it faithfully and that
//  the reduction formula works -- a parser that drops a row, or an
//  off-by-one between classes and counts, would leave a table that is
//  still internally consistent and simply describes a different group.
//
//  The oracle is the same one the data file uses: what (x,y,z) spans is
//  computable from the class NAMES, so it is independent of the numbers
//  being read.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>
#include "tdat/CharacterTable.H"
using namespace std;

static int bad = 0;

static void check(bool ok, const char* fmt, ...)
{
  char msg[512];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(msg, sizeof(msg), fmt, ap);
  va_end(ap);
  printf("  %-64s %s\n", msg, ok ? "ok" : "FAIL");
  if (!ok) bad++;
}

/** Character of the 3-D Cartesian representation, from a class name. */
static double cartesian(const string& cls)
{
  string n = cls.substr(0, cls.find('_'));
  while (!n.empty() && n[n.size()-1] == '\'') n.erase(n.size()-1);

  if (n == "E" || n == "e") return 3.0;
  if (n == "i" || n == "I") return -3.0;
  if ((n[0] == 's' || n[0] == 'S') && !isdigit(n.size() > 1 ? n[1] : 'x')
      && n[0] == 's') return 1.0;

  //  "C5p2" is C5 SQUARED, and "C2(=C4p2)" carries a parenthetical.
  //  Joining every digit in the name turned those into 52-fold and
  //  242-fold rotations, which is how Oh's reduction came out at a
  //  quarter and D5's at nothing at all.
  int order = 2, power = 1;
  {
    size_t paren = n.find('(');
    if (paren != string::npos) n = n.substr(0, paren);
    size_t p = n.find('p');
    string head = (p == string::npos) ? n.substr(1) : n.substr(1, p-1);
    string tail = (p == string::npos) ? ""          : n.substr(p+1);
    if (!head.empty()) order = atoi(head.c_str());
    if (!tail.empty()) power = atoi(tail.c_str());
    if (order < 1) order = 2;
  }
  const double angle = 2.0*M_PI*power/order;
  if (n[0] == 'C') return 1.0 + 2.0*cos(angle);
  if (n[0] == 'S') return -1.0 + 2.0*cos(angle);
  return 1.0;                       // a mirror: sh, sv, sd
}

int main(int argc, char** argv)
{
  const char* path = (argc > 1) ? argv[1]
                                : "data/client/config/CharacterTables";

  //  Not a fixed count: the tables are generated from the 5KE195
  //  compendium and the set grows.  What matters is that a useful
  //  number loaded and that the ones checked below are among them.
  const int groups = CharacterTable::loadFile(path);
  check(groups >= 43, "loaded the character tables (got %d groups)", groups);
  if (groups == 0) { printf("\n  FAIL\n"); return 1; }

  //  Case insensitivity: the group name reaches this from the detector,
  //  the .edml files and the codes' own output, which disagree on case.
  check(CharacterTable::lookup("Td") != 0, "lookup is case insensitive (Td)");
  check(CharacterTable::lookup("td") != 0, "lookup is case insensitive (td)");
  check(CharacterTable::lookup("C2v") != 0, "lookup is case insensitive (C2v)");
  check(CharacterTable::lookup("Nonesuch") == 0, "an unknown group returns null");

  const CharacterTable* td = CharacterTable::lookup("TD");
  if (td) {
    check(td->order() == 24, "Td order is 24 (got %d)", td->order());
    check(td->classes().size() == 5, "Td has 5 classes (got %zu)",
          td->classes().size());
    check(td->irreps().size() == 5, "Td has 5 irreps (got %zu)",
          td->irreps().size());
    check(td->dimension("T2") == 3, "Td T2 is three-dimensional (got %d)",
          td->dimension("T2"));
    check(td->dimension("A1") == 1, "Td A1 is one-dimensional (got %d)",
          td->dimension("A1"));
    check(td->characters("Nonesuch") == 0, "an unknown irrep returns null");
  }

  //  The reduction, against the one oracle that does not come from the
  //  numbers being tested.
  struct Case { const char* group; const char* expect; };
  Case cases[] = {
    {"C1", "3A"}, {"CS", "2A' + A''"}, {"CI", "3Au"}, {"C2", "A + 2B"},
    {"C2V", "A1 + B1 + B2"}, {"C3V", "A1 + E"}, {"C4V", "A1 + E"},
    {"C2H", "Au + 2Bu"}, {"D2H", "B1u + B2u + B3u"}, {"D3H", "E' + A2''"},
    {"D4H", "A2u + Eu"}, {"TD", "T2"}, {"OH", "T1u"},
    //  From the wider set the compendium supplies.
    {"D6H", "A2u + E1u"}, {"D3D", "A2u + Eu"}, {"D2D", "B2 + E"},
    {"D5", "A2 + E1"}, {"D4D", "B2 + E1"},
    //  The cyclic, improper-rotation, cubic and icosahedral groups,
    //  where the conjugate pairs are stored as one real E.
    {"C3", "A + E"}, {"C4", "A + E"}, {"C5", "A + E1"}, {"C6", "A + E1"},
    {"C3H", "E' + A''"}, {"C4H", "Au + Eu"}, {"C5H", "E1' + A''"},
    {"C6H", "Au + E1u"}, {"S4", "B + E"}, {"S6", "Au + Eu"},
    {"S8", "B + E1"}, {"T", "T"}, {"TH", "Tu"}, {"O", "T1"},
    {"I", "T1"}, {"IH", "T1u"},
  };
  printf("\n  (x,y,z) reduced through the loaded tables:\n");
  for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
    const CharacterTable* t = CharacterTable::lookup(cases[i].group);
    if (t == 0) { check(false, "%s loaded", cases[i].group); continue; }

    vector<double> chi;
    for (size_t c = 0; c < t->classes().size(); c++) {
      chi.push_back(cartesian(t->classes()[c]));
    }
    const string got = t->reduceToString(chi);
    check(got == cases[i].expect, "%-4s (x,y,z) spans %-18s want %s",
          cases[i].group, got.c_str(), cases[i].expect);
  }

  //  Reducible representations whose decomposition is known without
  //  the table: an orbit of atoms contributes the number it leaves in
  //  place, so a regular orbit gives h at E and 0 elsewhere and each
  //  real E must come out with the multiplicity of one conjugate half.
  struct Rep { const char* group; const char* chi; const char* expect; };
  Rep reps[] = {
    {"C3", "3 0 0", "A + E"},
    {"C3", "6 0 0", "2A + 2E"},
    {"C3", "2 -1 -1", "E"},
    {"C4", "4 0 0 0", "A + B + E"},
    {"C5", "5 0 0 0 0", "A + E1 + E2"},
    {"C6", "6 0 0 0 0 0", "A + B + E1 + E2"},
    {"C3H", "3 0 0 3 0 0", "A' + E'"},
    {"C3H", "6 0 0 0 0 0", "A' + E' + A'' + E''"},
    {"C4H", "8 0 0 0 0 0 0 0", "Ag + Bg + Eg + Au + Bu + Eu"},
    {"C5H", "10 0 0 0 0 0 0 0 0 0", "A' + E1' + E2' + A'' + E1'' + E2''"},
    {"C6H", "6 0 0 0 0 0 0 0 0 6 0 0", "Ag + E2g + Bu + E1u"},
    {"S4", "4 0 0 0", "A + B + E"},
    {"S6", "6 0 0 0 0 0", "Ag + Eg + Au + Eu"},
    {"S8", "8 0 0 0 0 0 0 0", "A + B + E1 + E2 + E3"},
    {"T", "4 1 1 0", "A + T"},
    {"T", "12 0 0 0", "A + E + 3T"},
    {"TH", "6 0 0 2 0 0 0 4", "Ag + Eg + Tu"},
    {"TH", "24 0 0 0 0 0 0 0", "Ag + Eg + 3Tg + Au + Eu + 3Tu"},
    {"O", "6 0 0 2 2", "A1 + E + T1"},
    {"I", "12 2 2 0 0", "A + T1 + T2 + H"},
    {"IH", "12 2 2 0 0 0 0 0 0 4", "Ag + Hg + T1u + T2u"},
  };
  printf("\n  known reducible representations:\n");
  for (size_t i = 0; i < sizeof(reps)/sizeof(reps[0]); i++) {
    const CharacterTable* t = CharacterTable::lookup(reps[i].group);
    if (t == 0) { check(false, "%s loaded", reps[i].group); continue; }
    vector<double> chi;
    istringstream in(reps[i].chi);
    double v;
    while (in >> v) chi.push_back(v);
    const string got = t->reduceToString(chi);
    check(got == reps[i].expect, "%-4s %-24s -> %s", reps[i].group,
          reps[i].chi, got.c_str());
  }

  //  A separable row is flagged, an ordinary one is not.
  const CharacterTable* c3 = CharacterTable::lookup("C3");
  if (c3) {
    check(c3->separable("E") && !c3->separable("A"),
          "C3: E is separable, A is not");
    vector<int> m;
    vector<double> one(3, 0.0);
    one[0] = 1.0;
    check(!c3->reduce(one, m),
          "C3: a lone 1-D character at E is refused (not a real rep)");
  }

  //  A representation must reduce to non-negative integers, and the
  //  totally symmetric one is the simplest check of the formula itself.
  const CharacterTable* oh = CharacterTable::lookup("OH");
  if (oh) {
    vector<double> a1g(oh->classes().size(), 1.0);
    check(oh->reduceToString(a1g) == "A1g",
          "Oh: all-ones reduces to A1g (got %s)",
          oh->reduceToString(a1g).c_str());

    //  Wrong length must be refused, not silently truncated.
    vector<double> tooShort(3, 1.0);
    vector<int> m;
    check(!oh->reduce(tooShort, m), "a wrong-length character vector is refused");

    //  Something that is not a representation of this group at all.
    vector<double> bogus(oh->classes().size(), 1.0);
    bogus[0] = 2.0;
    check(!oh->reduce(bogus, m),
          "a non-representation is refused, not rounded");
  }

  printf("\n  %s\n", bad ? "FAIL" : "PASS");
  return bad ? 1 : 0;
}
