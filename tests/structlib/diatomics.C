// Loads every Structure Library entry of Teaching > Diatomics the way the
// Builder does (FragUtil::load, type MVM) and checks atoms, bond and the
// stored distance against the experimental bond lengths (NIST Chemistry
// WebBook, Constants of Diatomic Molecules, Huber & Herzberg 1979, rounded to
// 0.001 A; He2 3.0 A is a convention).  Usage: structlib_diatomics <dir>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "tdat/FragUtil.H"
#include "tdat/Fragment.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"

using namespace std;

static int failures = 0;
static void check(bool ok, const string& what)
{
  cout << (ok ? "ok   " : "FAIL ") << what << endl;
  if (!ok) failures++;
}

struct Entry { const char *name, *a, *b; double dist; int bonds; };

int main(int argc, char **argv)
{
  string dir = argc > 1 ? argv[1] : ".";
  const Entry entries[] = {
    {"C2", "C", "C", 1.243, 1}, {"N2", "N", "N", 1.098, 1},
    {"O2", "O", "O", 1.208, 1}, {"CO", "C", "O", 1.128, 1},
    {"HF", "H", "F", 0.917, 1}, {"He2", "He", "He", 3.0, 0}};
  for (const Entry& e : entries) {
    string path = dir + "/" + e.name + ".mvm";
    ifstream is(path.c_str());
    check(is.good(), "open " + path);
    if (!is.good()) continue;
    Fragment frag;
    FragUtil util(&frag);
    check(util.load(is, "MVM", 1.0, true, 1, " ", " "), string("load ") + e.name);
    check(frag.numAtoms() == 2, string(e.name) + ": two atoms");
    if (frag.numAtoms() != 2) continue;
    check(frag.atomRef(0)->atomicSymbol() == e.a && frag.atomRef(1)->atomicSymbol() == e.b,
          string(e.name) + ": elements");
    const double *c = frag.coordinates();
    double d = sqrt(pow(c[0] - c[3], 2) + pow(c[1] - c[4], 2) + pow(c[2] - c[5], 2));
    check(fabs(d - e.dist) < 1e-4, string(e.name) + ": distance " + to_string(d)
          + " == " + to_string(e.dist));
    check(frag.numBonds() == e.bonds, string(e.name) + ": " + to_string(e.bonds) + " bond");
  }
  return failures ? 1 : 0;
}
