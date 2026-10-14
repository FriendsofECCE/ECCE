//  #325: a Metal (pi) bond joins two atoms directly: no nubs needed, none
//  consumed, no hydrogens touched; the order survives an MVM round trip.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

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

  // Piano stool: tetrahedral Fe, three P, benzene on the fourth nub.
  {
    Fragment ps;
    const double a = 2.2 / sqrt(3.0);
    ps.addAtom("Fe", fe);
    const double pp[3][3] = {{ a, a, 1.6 - a}, {-a, -a, 1.6 - a}, {-a, a, 1.6 + a}};
    for (int i = 0; i < 3; i++) { ps.addAtom("P", pp[i]); ps.addBond(0, 1 + i, 1.0); }
    const double nb[3] = {a, -a, 1.6 + a};
    ps.addAtom("Nub", nb); ps.addBond(0, 4, 1.0);
    // A flat ring far away, tilted about x.
    int first = addRing(ps, 6, 0.0);
    for (int i = first; i < ps.numAtoms(); i++) {
      const double *c = ps.atomRef(i)->coordinates();
      double ct = cos(0.7), st = sin(0.7);
      double n3[3] = {c[0] + 9.0, c[1] * ct - c[2] * st + 5.0, c[1] * st + c[2] * ct - 3.0};
      ps.atomRef(i)->coordinates(n3);
    }
    int before = ps.numAtoms(), hs0 = count(ps, "H");
    std::vector<TAtm*> pick(1, ps.atomRef(first + 2));
    std::string msg;
    threw = false;
    try { ps.bondPiLigand(ps.atomRef(4), pick, style, msg); } catch (...) { threw = true; }
    check(!threw, "piano stool: benzene attaches at the Fe nub");
    printf("     message: %s\n", msg.c_str());
    check(msg.find("eta6") == 0, "message names eta6");
    check(ps.numAtoms() == before - 1 && count(ps, "H") == hs0 && count(ps, "Nub") == 0,
          "only the metal nub is consumed");
    int nm = 0;
    double cen[3] = {0, 0, 0};
    for (int i = 0; i < ps.numAtoms(); i++) {
      TBond* b = ps.bond(0, i);
      if (b && b->order() == TBond::Metal) {
        nm++;
        for (int k = 0; k < 3; k++) cen[k] += ps.atomRef(i)->coordinates()[k] / 6;
      }
    }
    check(nm == 6, "six Metal bonds");
    double d[3] = {cen[0] - fe[0], cen[1] - fe[1], cen[2] - fe[2]};
    double dl = sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
    printf("     Fe-centroid %.3f\n", dl);
    check(fabs(dl - 1.7) < 0.01, "Fe-centroid 1.7 A");
    double dirn[3] = {a, -a, a};
    double cosang = (d[0]*dirn[0] + d[1]*dirn[1] + d[2]*dirn[2]) / (dl * 2.2);
    check(cosang > 0.999, "centroid lies on the nub direction");
    double maxdev = 0;
    for (int i = 0; i < ps.numAtoms(); i++) {
      TBond* b = ps.bond(0, i);
      if (!b || b->order() != TBond::Metal) continue;
      const double *c = ps.atomRef(i)->coordinates();
      double r[3] = {c[0] - fe[0] - d[0], c[1] - fe[1] - d[1], c[2] - fe[2] - d[2]};
      maxdev = std::max(maxdev, fabs(r[0]*d[0] + r[1]*d[1] + r[2]*d[2]) / dl);
    }
    printf("     max out-of-plane deviation %.4f\n", maxdev);
    check(maxdev < 0.01, "ring plane perpendicular to the Fe-centroid line");
    check(ps.atomRef(1)->coordinates()[0] == pp[0][0], "the P atoms did not move");
  }

  // Ethene on an equatorial nub of a trigonal-bipyramidal Fe.
  {
    Fragment tb;
    tb.addAtom("Fe", fe);
    const double ax[2][3] = {{0, 0, 1.6 + 1.8}, {0, 0, 1.6 - 1.8}};
    const double eq[3][3] = {{1.8, 0, 1.6}, {-0.9, 1.56, 1.6}, {-0.9, -1.56, 1.6}};
    for (int i = 0; i < 2; i++) { tb.addAtom("C", ax[i]); tb.addBond(0, 1 + i, 1.0); }
    tb.addAtom("C", eq[1]); tb.addBond(0, 3, 1.0);
    tb.addAtom("C", eq[2]); tb.addBond(0, 4, 1.0);
    tb.addAtom("Nub", eq[0]); tb.addBond(0, 5, 1.0);
    int e0 = tb.numAtoms();
    const double ec[6][3] = {{5, 5, 0.67}, {5, 5, -0.67}, {5.9, 5, 1.2}, {4.1, 5, 1.2},
                             {5.9, 5, -1.2}, {4.1, 5, -1.2}};
    const char* es[6] = {"C", "C", "H", "H", "H", "H"};
    for (int i = 0; i < 6; i++) tb.addAtom(es[i], ec[i]);
    tb.addBond(e0, e0 + 1, 2.0);
    for (int i = 2; i < 6; i++) tb.addBond(e0 + (i < 4 ? 0 : 1), e0 + i, 1.0);
    std::vector<TAtm*> pick(1, tb.atomRef(e0));
    std::string msg;
    threw = false;
    try { tb.bondPiLigand(tb.atomRef(5), pick, style, msg); } catch (...) { threw = true; }
    check(!threw && msg.find("eta2") == 0, "ethene attaches at the equatorial nub: " + msg);
    // The nub (index 5) is deleted, so the ethene carbons are now 5 and 6.
    int c0 = e0 - 1;
    check(tb.bond(0, c0) != 0 && tb.bond(0, c0 + 1) != 0 &&
          tb.bond(0, c0)->order() == TBond::Metal, "two Metal bonds");
    const double *c1 = tb.atomRef(c0)->coordinates(), *c2 = tb.atomRef(c0 + 1)->coordinates();
    double mid[3] = {(c1[0]+c2[0])/2 - fe[0], (c1[1]+c2[1])/2 - fe[1], (c1[2]+c2[2])/2 - fe[2]};
    double ml = sqrt(mid[0]*mid[0] + mid[1]*mid[1] + mid[2]*mid[2]);
    double cc[3] = {c2[0]-c1[0], c2[1]-c1[1], c2[2]-c1[2]};
    double cl = sqrt(cc[0]*cc[0] + cc[1]*cc[1] + cc[2]*cc[2]);
    double cs = (cc[0]*mid[0] + cc[1]*mid[1] + cc[2]*mid[2]) / (cl * ml);
    printf("     Fe-midpoint %.3f, cos(C=C, M-mid) %.4f, C=C %.3f\n", ml, cs, cl);
    check(fabs(ml - 2.0) < 0.01, "Fe-midpoint 2.0 A");
    check(fabs(cs) < 0.01, "C=C perpendicular to the M-midpoint line");
    check(mid[0] > 1.9, "midpoint lies along the nub direction (+x)");
    check(count(tb, "Nub") == 0, "the nub is gone");
  }

  // Structure Library ligands arrive with a nub on each donor atom.
  {
    const char* home = getenv("ECCE_HOME");
    std::string dir = std::string(home ? home : ".") + "/data/client/StructureLibrary/Ligands/";
    Fragment cp;
    std::ifstream in((dir + "Cyclopentadienyl.mvm").c_str());
    check(in.good() && cp.restoreMVM(in, 1.0, false), "Cyclopentadienyl.mvm loads");
    Fragment m;
    m.addAtom("Fe", fe);
    const double up[3] = {0, 0, 3.6};
    m.addAtom("Nub", up);
    m.addBond(0, 1, 1.0);
    int off = m.numAtoms();
    for (int i = 0; i < cp.numAtoms(); i++) {
      double c[3] = {cp.atomRef(i)->coordinates()[0] + 6, cp.atomRef(i)->coordinates()[1],
                     cp.atomRef(i)->coordinates()[2] - 4};
      m.addAtom(cp.atomRef(i)->atomicSymbol().c_str(), c);
    }
    for (int i = 0; i < cp.numAtoms(); i++)
      for (int j = i + 1; j < cp.numAtoms(); j++) {
        TBond* b = cp.bond(i, j);
        if (b) m.addBond(off + i, off + j, b->order());
      }
    std::vector<TAtm*> pick(1, m.atomRef(off));   // a ring carbon
    std::string msg;
    threw = false;
    try { m.bondPiLigand(m.atomRef(1), pick, style, msg); } catch (...) { threw = true; }
    printf("     %s\n", msg.c_str());
    int nm = 0;
    for (int i = 1; i < m.numAtoms(); i++) {
      TBond* b = m.bond(0, i);
      if (b && b->order() == TBond::Metal) nm++;
    }
    printf("     threw=%d nm=%d atoms=%d\n", (int)threw, nm, m.numAtoms());
    check(!threw && msg.find("eta5") == 0 && nm == 5, "Cp attaches eta5 with five Metal bonds");
    check(count(m, "Nub") == 0 && count(m, "H") == 5 && m.numAtoms() == 1 + 10,
          "no nubs left on the metal or the Cp; its hydrogens stay");

    // A sigma donor still goes through the nub path: Fe nub + pyridine N nub.
    Fragment py;
    std::ifstream pin((dir + "Pyridine.mvm").c_str());
    check(pin.good() && py.restoreMVM(pin, 1.0, false), "Pyridine.mvm loads");
    Fragment s;
    s.addAtom("Fe", fe);
    s.addAtom("Nub", up);
    s.addBond(0, 1, 1.0);
    int noff = s.numAtoms(), nn = -1, nnub = -1;
    for (int i = 0; i < py.numAtoms(); i++) {
      s.addAtom(py.atomRef(i)->atomicSymbol().c_str(), py.atomRef(i)->coordinates());
      if (py.atomRef(i)->atomicSymbol() == "N") nn = noff + i;
    }
    for (int i = 0; i < py.numAtoms(); i++)
      for (int j = i + 1; j < py.numAtoms(); j++) {
        TBond* b = py.bond(i, j);
        if (b) s.addBond(noff + i, noff + j, b->order());
      }
    for (int i = noff; i < s.numAtoms(); i++)
      if (s.atomRef(i)->atomicSymbol() == "Nub") nnub = i;
    int natoms = s.numAtoms();
    threw = false;
    try { s.bondAtoms(s.atomRef(0), s.atomRef(nn), TBond::Single, style); }
    catch (...) { threw = true; }
    int nafter = -1;
    for (int i = 0; i < s.numAtoms(); i++)
      if (s.atomRef(i)->atomicSymbol() == "N") nafter = i;
    check(!threw && nn >= 0 && nnub >= 0 && s.numAtoms() == natoms - 2 && nafter > 0 &&
          s.bond(0, nafter) != 0 && s.bond(0, nafter)->order() == TBond::Single,
          "pyridine bonds Fe-N through the nubs as before (both nubs consumed)");
  }

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
