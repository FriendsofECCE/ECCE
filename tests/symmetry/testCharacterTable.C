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

  int order = 2;
  string digits;
  for (size_t i = 0; i < n.size(); i++) if (isdigit(n[i])) digits += n[i];
  if (!digits.empty()) order = atoi(digits.c_str());

  const double angle = 2.0*M_PI/order;
  if (n[0] == 'C') return 1.0 + 2.0*cos(angle);
  if (n[0] == 'S') return -1.0 + 2.0*cos(angle);
  return 1.0;                       // a mirror: sh, sv, sd
}

int main(int argc, char** argv)
{
  const char* path = (argc > 1) ? argv[1]
                                : "data/client/config/CharacterTables";

  const int groups = CharacterTable::loadFile(path);
  check(groups == 13, "loaded 13 groups (got %d)", groups);
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
