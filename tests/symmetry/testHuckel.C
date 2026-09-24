//  Extended Huckel, against what the method must give.
//
//  There is no reference implementation here to diff against, and
//  comparing energies to a table of somebody else's numbers would
//  check the parameters rather than the code.  What CAN be checked is
//  what the method is obliged to produce whatever the parameters are:
//  degeneracies the geometry forces, and a non-bonding orbital left
//  exactly where its atom put it.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
using namespace std;

#include "tdat/Huckel.H"
#include "tdat/MoFragments.H"

static int bad = 0;

static void check(const char* what, bool ok, const char* detail = "")
{
  printf("  %-52s %s %s\n", what, ok ? "ok" : "FAIL", detail);
  if (!ok) bad++;
}

int main(void)
{
  printf("\n  Extended Huckel\n");

  //  --- methane: Td forces a threefold degeneracy -------------------
  {
    const double d = 0.6276;
    const double c[] = { 0,0,0,  d,d,d,  d,-d,-d,  -d,d,-d,  -d,-d,d };
    vector<double> coords(c, c + 15);
    const char* e[] = { "C", "H", "H", "H", "H" };
    vector<string> elements(e, e + 5);

    vector<double> energies, occupancies;
    vector< vector<double> > coefficients;
    vector<int> perAtom, shellOf;
    string why;

    const bool ok = Huckel::solve(coords, elements, 0, energies,
                                  occupancies, coefficients, perAtom,
                                  shellOf, why);
    check("methane solves", ok, why.c_str());
    if (ok) {
      check("eight valence functions", energies.size() == 8);

      //  A t2 set is three orbitals at ONE energy.  Nothing tells the
      //  engine that; it falls out of the geometry, so it is the
      //  sharpest check there is that the overlaps and the solver are
      //  right.
      const bool bonding = energies.size() == 8 &&
          fabs(energies[1] - energies[2]) < 1.0e-9 &&
          fabs(energies[2] - energies[3]) < 1.0e-9;
      check("the occupied t2 set is exactly threefold", bonding);

      const bool anti = energies.size() == 8 &&
          fabs(energies[4] - energies[5]) < 1.0e-9 &&
          fabs(energies[5] - energies[6]) < 1.0e-9;
      check("and so is the empty one", anti);

      //  a1 below t2 below t2* below a1*, which is the diagram.
      const bool order = energies.size() == 8 &&
          energies[0] < energies[1] && energies[3] < energies[4] &&
          energies[6] < energies[7];
      check("a1 below t2, t2* below a1*", order);

      double electrons = 0.0;
      for (size_t i = 0; i < occupancies.size(); i++) {
        electrons += occupancies[i];
      }
      check("eight valence electrons, paired", fabs(electrons - 8.0) < 1e-9);
    }
  }

  //  --- water: the lone pair stays where oxygen put it ---------------
  {
    const double c[] = { 0.0, 0.0, 0.1173,  0.0, 0.7572, -0.4692,
                         0.0, -0.7572, -0.4692 };
    vector<double> coords(c, c + 9);
    const char* e[] = { "O", "H", "H" };
    vector<string> elements(e, e + 3);

    vector<double> energies, occupancies;
    vector< vector<double> > coefficients;
    vector<int> perAtom, shellOf;
    string why;

    const bool ok = Huckel::solve(coords, elements, 0, energies,
                                  occupancies, coefficients, perAtom,
                                  shellOf, why);
    check("water solves", ok, why.c_str());

    if (ok) {
      //  ONE OF WATER'S ORBITALS IS PURE OXYGEN 2p: the out-of-plane
      //  lone pair, which has no hydrogen combination of its symmetry
      //  to mix with.  An orbital that does not mix cannot move, so it
      //  must come out at oxygen's own 2p energy -- exactly, not
      //  nearly.  Nothing in the method arranges that; it is what the
      //  method means.
      double p2 = 0.0;
      MoFragments::valenceEnergy("O", 1, p2);
      p2 /= 27.211386245988;

      bool found = false;
      for (size_t i = 0; i < energies.size(); i++) {
        if (fabs(energies[i] - p2) < 1.0e-9) found = true;
      }
      char detail[64];
      snprintf(detail, sizeof(detail), "(O 2p = %.4f Ha)", p2);
      check("the lone pair sits exactly at oxygen's 2p", found, detail);

      double electrons = 0.0;
      for (size_t i = 0; i < occupancies.size(); i++) {
        electrons += occupancies[i];
      }
      check("eight valence electrons", fabs(electrons - 8.0) < 1e-9);
    }
  }

  //  --- and it declines rather than guessing -------------------------
  {
    const double c[] = { 0,0,0,  1.8,0,0 };
    vector<double> coords(c, c + 6);
    const char* e[] = { "C", "Cl" };
    vector<string> elements(e, e + 2);

    string missing;
    const bool no = !Huckel::canSolve(elements, missing);
    check("past the second row it declines", no, missing.c_str());
  }

  printf("\n  %s\n", bad ? "FAIL" : "PASS");
  return bad ? 1 : 0;
}
