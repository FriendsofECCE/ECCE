//  #325: a Metal (pi) bond joins two atoms directly: no nubs needed, none
//  consumed, no hydrogens touched; the order survives an MVM round trip.
#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>

#include "tdat/Fragment.H"
#include "tdat/TBond.H"

static int failures = 0;

static void check(bool ok, const std::string& what)
{
  printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) failures++;
}

// A flat ring of n carbons with one H each, at height zc; returns the index
// of the first carbon.
static int addRing(Fragment& f, int n, double zc)
{
  int first = f.numAtoms();
  double rc = (n == 6) ? 1.39 : 1.21, rh = rc + 1.08;
  for (int i = 0; i < n; i++) {
    double a = 2 * M_PI * i / n;
    double c[3] = {rc * cos(a), rc * sin(a), zc};
    f.addAtom("C", c);
  }
  for (int i = 0; i < n; i++) {
    double a = 2 * M_PI * i / n;
    double h[3] = {rh * cos(a), rh * sin(a), zc};
    f.addAtom("H", h);
  }
  for (int i = 0; i < n; i++) {
    f.addBond(first + i, first + (i + 1) % n, 1.5);
    f.addBond(first + i, first + n + i, 1.0);
  }
  return first;
}

static int count(Fragment& f, const char* sym)
{
  int n = 0;
  for (int i = 0; i < f.numAtoms(); i++)
    if (f.atomRef(i)->atomicSymbol() == sym) n++;
  return n;
}

int main()
{
  const double fe[3] = {0, 0, 1.6};
  const double nubpos[3] = {0, 0, 3.6};
  DisplayStyle style;

  // Fe with a Nub that must survive, + benzene.
  Fragment f;
  f.addAtom("Fe", fe);
  f.addAtom("Nub", nubpos);
  f.addBond(0, 1, 1.0);
  int ring = addRing(f, 6, 0.0);
  int atoms = f.numAtoms(), hs = count(f, "H"), nubs = count(f, "Nub");
  bool threw = false;
  try {
    for (int i = 0; i < 6; i++)
      f.bondAtoms(f.atomRef(0), f.atomRef(ring + i), TBond::Metal, style);
  } catch (...) { threw = true; }
  check(!threw, "six Fe-C Metal bonds to nubless ring carbons succeed");
  check(f.numAtoms() == atoms, "atom count unchanged");
  check(count(f, "H") == hs, "hydrogen count unchanged");
  check(count(f, "Nub") == nubs, "Fe keeps its nub");
  int metal = 0;
  for (int i = 0; i < 6; i++) {
    TBond* b = f.bond(0, ring + i);
    if (b && b->order() == TBond::Metal) metal++;
  }
  check(metal == 6 && TBond::Metal == 0.5, "six bonds of order 0.5");

  threw = false;
  try { f.bondAtoms(f.atomRef(0), f.atomRef(ring), TBond::Metal, style); }
  catch (...) { threw = true; }
  check(threw, "a second Fe-C bond to the same atom is refused");

  // Picking the Nub stands for its parent.
  Fragment g;
  g.addAtom("Fe", fe);
  g.addAtom("Nub", nubpos);
  g.addBond(0, 1, 1.0);
  int r2 = addRing(g, 6, 0.0);
  g.bondAtoms(g.atomRef(1), g.atomRef(r2), TBond::Metal, style);
  check(g.bond(0, r2) != 0 && g.numAtoms() == 2 + 12 && count(g, "Nub") == 1,
        "a picked nub stands for its parent and is not consumed");

  // An ordinary order still needs nubs.
  threw = false;
  try { g.bondAtoms(g.atomRef(0), g.atomRef(r2 + 1), TBond::Single, style); }
  catch (...) { threw = true; }
  check(threw, "a single bond to a nubless carbon is still refused");

  // Ferrocene from Fe + two Cp.
  Fragment fc;
  fc.addAtom("Fe", fe);
  int c1 = addRing(fc, 5, 0.0), c2 = addRing(fc, 5, 3.3);
  int total = fc.numAtoms();
  for (int i = 0; i < 5; i++) {
    fc.bondAtoms(fc.atomRef(0), fc.atomRef(c1 + i), TBond::Metal, style);
    fc.bondAtoms(fc.atomRef(0), fc.atomRef(c2 + i), TBond::Metal, style);
  }
  check(fc.numAtoms() == total && count(fc, "H") == 10,
        "ferrocene: 21 atoms, 10 H");
  check(fc.numBonds() == 20 + 10, "ferrocene: 30 bonds incl. ten Fe-C");

  // MVM round trip keeps the 0.5 orders.
  std::stringstream ss;
  fc.dumpMVM(ss, true);
  Fragment back;
  bool ok = back.restoreMVM(ss, 1.0, false);
  int back_metal = 0;
  for (int i = 0; ok && i < back.numAtoms(); i++)
    for (int j = i + 1; j < back.numAtoms(); j++) {
      TBond* b = back.bond(i, j);
      if (b && b->order() == TBond::Metal) back_metal++;
    }
  check(ok && back.numAtoms() == total && back_metal == 10,
        "MVM save/reload keeps ten order-0.5 bonds");

  return failures ? 1 : 0;
}
