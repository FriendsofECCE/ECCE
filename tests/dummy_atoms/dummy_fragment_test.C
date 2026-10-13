//  #298: a dummy atom X is not an element -- no basis set, no nuclear charge,
//  not in the formula -- while still counting as an atom (its index is what
//  the constraints and the output tables refer to).
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "tdat/Fragment.H"
#include "tdat/GeomConstraintPick.H"

static int failures = 0;

static void check(bool ok, const std::string& what)
{
  printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) failures++;
}

int main()
{
  Fragment frag;
  const char* tags[] = {"N", "H", "H", "H", "X"};
  const double xyz[] = {0, 0, 0.1,  0, 0.94, -0.3,  0.81, -0.47, -0.3,
                        -0.81, -0.47, -0.3,  0, 0, 1.1};
  for (int i = 0; i < 5; i++) frag.addAtom(tags[i], xyz + 3 * i);

  check(frag.numAtoms() == 5, "X still counts as an atom (numbering)");
  check(frag.uniqueTagStr() == "H N" || frag.uniqueTagStr() == "N H",
        "basis tags omit X: '" + frag.uniqueTagStr() + "'");
  check(frag.nuclearCharge() == 10, "X adds no nuclear charge");
  check(frag.numElectrons() == 10, "X adds no electrons");
  check(frag.formula().find('X') == std::string::npos,
        "formula omits X: '" + frag.formula() + "'");

  // #306: constraint atoms are reordered by the bonding, not the pick order.
  for (int h = 1; h <= 4; h++) frag.addBond(0, h, 1.0);   // N-H1..3, N-X
  std::vector<int> p;
  p.push_back(1); p.push_back(0); p.push_back(2);          // H N H
  GeomConstraintPick r = orderConstraintAtoms(frag, p);
  check(!r.reordered && !r.needsConfirm && r.atoms == p, "angle H-N-H picked centre-in-the-middle is kept");
  p.clear(); p.push_back(0); p.push_back(1); p.push_back(2); // N H H: centre first
  r = orderConstraintAtoms(frag, p);
  check(r.reordered && r.atoms.size() == 3 && r.atoms[1] == 0 && r.atoms[0] == 1 && r.atoms[2] == 2 && !r.needsConfirm,
        "angle picked centre-first is reordered: " + r.description);
  check(r.description == "Angle H2\xE2\x80\x93N1\xE2\x80\x93H3 (centre N1)", "angle description names the centre");
  p.clear(); p.push_back(4); p.push_back(1); p.push_back(0);  // X H N: ends first
  r = orderConstraintAtoms(frag, p);
  check(r.reordered && r.atoms[1] == 0 && !r.needsConfirm, "angle X-H-N becomes X-N-H: " + r.description);
  p.clear(); p.push_back(1); p.push_back(2); p.push_back(3);  // H H H: no centre
  r = orderConstraintAtoms(frag, p);
  check(r.needsConfirm && r.atoms == p && !r.warning.empty(), "angle H-H-H is not bonded: confirm");
  p.clear(); p.push_back(1); p.push_back(2);                  // H H not bonded
  r = orderConstraintAtoms(frag, p);
  check(r.needsConfirm, "bond H-H not bonded: confirm");
  p.clear(); p.push_back(0); p.push_back(4);
  r = orderConstraintAtoms(frag, p);
  check(!r.needsConfirm, "bond N-X (bonded dummy) needs no confirmation");
  Fragment f2;
  for (int i = 0; i < 5; i++) f2.addAtom(tags[i], xyz + 3 * i);
  f2.addBond(0, 1, 1.0); f2.addBond(0, 2, 1.0); f2.addBond(0, 3, 1.0);
  p.clear(); p.push_back(4); p.push_back(0); p.push_back(1);
  r = orderConstraintAtoms(f2, p);
  check(r.needsConfirm && r.atoms == p, "angle with an unbonded dummy keeps the order and asks");
  // torsion H1-N-C-H on a chain
  Fragment f3;
  const char* t3[] = {"H", "N", "C", "O"};
  const double x3[] = {0,0,0, 1,0,0, 2,0,0, 3,0,0};
  for (int i = 0; i < 4; i++) f3.addAtom(t3[i], x3 + 3 * i);
  f3.addBond(0, 1, 1.0); f3.addBond(1, 2, 1.0); f3.addBond(2, 3, 1.0);
  p.clear(); p.push_back(1); p.push_back(0); p.push_back(3); p.push_back(2);
  r = orderConstraintAtoms(f3, p);
  check(r.reordered && !r.needsConfirm && (r.atoms[0] == 0 || r.atoms[0] == 3) && r.atoms[1] != r.atoms[0],
        "torsion picked scrambled becomes a chain: " + r.description);

  // #306: a dummy on a saturated centre, from the picked coordinates.
  {
    std::vector<std::vector<double> > m;
    const double c[4][3] = {{0,0,0}, {0.63,0.63,0.63}, {-0.63,-0.63,0.63}, {-0.63,0.63,-0.63}};
    for (int i = 0; i < 4; i++) m.push_back(std::vector<double>(c[i], c[i] + 3));
    double o[3];
    check(dummyAtomPosition(m, DUMMY_BEYOND, 1.0, o).empty(), "beyond: defined");
    const double r = sqrt(o[0]*o[0] + o[1]*o[1] + o[2]*o[2]);
    check(fabs(r - 1.0) < 1e-9 && o[0] > 0 && o[1] < 0 && o[2] < 0, "beyond: 1 A from the first atom, opposite the rest");
    dummyAtomPosition(m, DUMMY_BETWEEN, 0.5, o);
    check(fabs(o[0] + 0.5/sqrt(3.0)) < 1e-9 && o[1] > 0, "between: towards the rest");
    dummyAtomPosition(m, DUMMY_CENTRE, 1.0, o);
    check(fabs(o[0] + 0.1575) < 1e-9 && fabs(o[1] - 0.1575) < 1e-9 && fabs(o[2] - 0.1575) < 1e-9, "centre: centroid of all picked");
    std::vector<std::vector<double> > two(m.begin(), m.begin() + 1);
    check(!dummyAtomPosition(two, DUMMY_BEYOND, 1.0, o).empty(), "one atom: refused");
    std::vector<std::vector<double> > same(2, m[0]);
    check(!dummyAtomPosition(same, DUMMY_BEYOND, 1.0, o).empty(), "no axis: refused");
  }
  return failures ? 1 : 0;
}
