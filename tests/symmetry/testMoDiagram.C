//  The diagram's layout decisions, which are judgement calls and so
//  worth pinning: what shares a level, what counts as core, what
//  connects to what.
#include <cstdio>
#include <sstream>
#include <cmath>
#include <string>
#include <vector>
#include "tdat/MoDiagram.H"
using namespace std;

static int bad = 0;
static void check(bool ok, const string& what)
{
  printf("  %-64s %s\n", what.c_str(), ok ? "ok" : "FAIL");
  if (!ok) bad++;
}

int main()
{
  printf("  grouping orbitals into levels\n");

  //  Methane: a1 below a triply degenerate t2 set, then virtuals.
  {
    double e[] = {-11.2, -0.94, -0.54, -0.54, -0.54, 0.19, 0.28, 0.28, 0.28};
    double o[] = {  2.0,   2.0,   2.0,   2.0,   2.0, 0.0,  0.0,  0.0,  0.0};
    const char* l[] = {"a1","a1","t2","t2","t2","a1","t2","t2","t2"};
    vector<double> E(e, e+9), O(o, o+9);
    vector<string> L(l, l+9);

    vector<MoLevel> levels;
    MoDiagram::group(E, O, L, 1e-4, levels);
    check(levels.size() == 5,
          "CH4's nine orbitals draw as five levels (got "
          + string(1, '0'+(char)levels.size()) + ")");
    if (levels.size() == 5) {
      check(levels[2].degeneracy == 3, "the occupied t2 set is one level of 3");
      check(fabs(levels[2].occupancy - 6.0) < 1e-9,
            "that level holds six electrons");
      check(levels[2].orbitals.size() == 3,
            "it remembers its three orbitals, for clicking through");
    }
  }

  //  An accidental near-degeneracy is NOT a degenerate set.
  {
    double e[] = {-0.5, -0.5};
    double o[] = { 2.0,  2.0};
    const char* l[] = {"a1","b2"};
    vector<double> E(e, e+2), O(o, o+2);
    vector<string> L(l, l+2);
    vector<MoLevel> levels;
    MoDiagram::group(E, O, L, 1e-4, levels);
    check(levels.size() == 2,
          "two irreps at the same energy stay two levels");
  }

  //  Same irrep, different energies: also two levels.
  {
    double e[] = {-0.9, -0.3};
    double o[] = { 2.0,  2.0};
    const char* l[] = {"a1","a1"};
    vector<double> E(e, e+2), O(o, o+2);
    vector<string> L(l, l+2);
    vector<MoLevel> levels;
    MoDiagram::group(E, O, L, 1e-4, levels);
    check(levels.size() == 2, "one irrep at two energies stays two levels");
  }

  printf("\n  hiding the core\n");
  {
    //  Water: oxygen 1s far below everything else.
    double e[] = {-20.5, -1.35, -0.72, -0.57, -0.50};
    double o[] = {  2.0,   2.0,   2.0,   2.0,   2.0};
    const char* l[] = {"a1","a1","b1","a1","b2"};
    vector<double> E(e, e+5), O(o, o+5);
    vector<string> L(l, l+5);

    MoColumn col;
    MoDiagram::group(E, O, L, 1e-4, col.levels);
    const double cut = MoDiagram::suggestCoreCutoff(col.levels);
    check(cut > -20.5 && cut < -1.35,
          "the suggested cutoff falls in the core/valence gap");
    MoDiagram::hideBelow(col, cut);
    check(col.levels.size() == 4 && col.hiddenCount == 1,
          "the 1s is hidden and the four valence levels remain");
  }
  {
    //  A semiempirical calculation has no core at all, and nothing
    //  should be hidden just because something has to be.
    double e[] = {-1.30, -1.05, -0.80, -0.55, -0.30};
    double o[] = {  2.0,   2.0,   2.0,   2.0,   0.0};
    const char* l[] = {"a1","b1","a1","b2","a1"};
    vector<double> E(e, e+5), O(o, o+5);
    vector<string> L(l, l+5);
    MoColumn col;
    MoDiagram::group(E, O, L, 1e-4, col.levels);
    MoDiagram::hideBelow(col, MoDiagram::suggestCoreCutoff(col.levels));
    check(col.hiddenCount == 0,
          "an evenly spaced spectrum has nothing hidden");
  }
  {
    //  METHANE FROM A SEMIEMPIRICAL RUN: the case that went wrong on
    //  screen.  Four levels, a1 and t2 bonding, t2 and a1 antibonding,
    //  and no core orbital anywhere -- MOPAC has no 1s.
    //
    //  The largest gap in this spectrum by far is the HOMO-LUMO gap,
    //  and a rule that looks for the largest gap low in the spectrum
    //  picks it: both BONDING levels were folded away and the diagram
    //  showed the two antibonding ones on their own, over a range of
    //  +6 to +7 eV.
    double e[] = {-0.91999, -0.50466, -0.50465, -0.50447,
                   0.21730,  0.21741,  0.21745,  0.27253};
    double o[] = {2.0, 2.0, 2.0, 2.0, 0.0, 0.0, 0.0, 0.0};
    const char* l[] = {"a1","t2","t2","t2","t2","t2","t2","a1"};
    vector<double> E(e, e+8), O(o, o+8);
    vector<string> L(l, l+8);
    MoColumn col;
    MoDiagram::group(E, O, L, 1e-3, col.levels);
    check(col.levels.size() == 4,
          "methane groups into four levels: a1 t2 t2* a1*");
    MoDiagram::hideBelow(col, MoDiagram::suggestCoreCutoff(col.levels));
    check(col.hiddenCount == 0,
          "methane has no core, so no bonding level is folded away");
    MoDiagram::hideAbove(col, MoDiagram::suggestVirtualCutoff(col.levels));
    check(col.hiddenAboveCount == 0 && col.levels.size() == 4,
          "and both antibonding levels stay: all four are drawn");
  }

  printf("\n  connecting by symmetry\n");
  {
    //  Water: the hydrogen TASOs are a1 + b1; oxygen brings a1 and b1
    //  and b2.  The b2 has no partner and must come out non-bonding.
    //  On .irrep, not .label: the drawn label carries the shell for a
    //  fragment level ("A1  (2s)") and whatever spelling the code uses
    //  for a molecular one, so matching on it would never work.  The
    //  mixed case here is deliberate -- a code reports a1, a character
    //  table says A1, and canonicalIrrep is what reconciles them.
    //  FIVE molecular orbitals, not three.  The fragments bring three
    //  orbitals on one side and two on the other, and five orbitals in
    //  give five out -- a1 and b1 each split into a bonding and an
    //  antibonding, and the lone b2 has nothing to pair with.  The
    //  first version of this fixture listed three, which is not a
    //  possible molecule, and classify() rightly refused to pair
    //  anything in it.
    vector<MoLevel> left(3), centre(5), right(2);
    left[0].irrep   = MoDiagram::canonicalIrrep("A1");
    left[1].irrep   = MoDiagram::canonicalIrrep("B1");
    left[2].irrep   = MoDiagram::canonicalIrrep("B2");
    const char* order[] = {"a1", "b1", "b2", "a1", "b1"};
    for (int i = 0; i < 5; i++) {
      centre[i].irrep = MoDiagram::canonicalIrrep(order[i]);
      centre[i].degeneracy = 1;
      centre[i].energy = -1.0 + 0.3*i;
    }
    for (int i = 0; i < 3; i++) left[i].degeneracy = 1;
    for (int i = 0; i < 2; i++) right[i].degeneracy = 1;
    right[0].irrep  = MoDiagram::canonicalIrrep("A1");
    right[1].irrep  = MoDiagram::canonicalIrrep("B1");

    //  connect() emits one record per fragment level reached, because
    //  an MO mixes with EVERY fragment level of its irrep -- that is
    //  symmetry mixing, and one record per MO cannot express it.
    MoDiagram::classify(left, centre, right);
    vector<MoConnection> links;
    MoDiagram::connect(left, centre, right, links);

    //  What matters is which sides each MO reaches.
    bool reach[5][2];
    for (int i = 0; i < 5; i++) { reach[i][0] = reach[i][1] = false; }
    for (size_t i = 0; i < links.size(); i++) {
      const int c = links[i].centreLevel;
      if (c < 0 || c > 4) continue;
      if (links[i].leftLevel  >= 0) reach[c][0] = true;
      if (links[i].rightLevel >= 0) reach[c][1] = true;
    }
    check(reach[0][0] && reach[0][1], "the a1 MO connects to both sides");
    check(reach[1][0] && reach[1][1], "the b1 MO connects to both sides");
    check(reach[2][0] && !reach[2][1],
          "the b2 MO has no TASO partner: non-bonding, as the lone pair is");
    check(centre[2].character == MoLevel::NONBONDING,
          "and it is classified non-bonding");
    check(centre[0].character == MoLevel::BONDING &&
          centre[4].character == MoLevel::ANTIBONDING,
          "the lowest a1 is bonding and the highest b1 antibonding");
  }

  //  ------------------------------------------------------------------
  //  The formula, which is where a reader checks the charge.
  //
  //  Hill order is an index convention and writes ammonia H3N.  What
  //  belongs on a diagram is IUPAC's element sequence, in which
  //  hydrogen sits between nitrogen and tellurium -- so NH3 and CH4,
  //  but H2O and HCl.  These are the cases that tell the two apart.
  {
    printf("\n  Formulae\n");

    struct Case { const char* atoms; int charge; const char* want; };
    static const Case CASES[] = {
      { "N H H H",        0, "NH3"     },
      { "O H H",          0, "H2O"     },
      { "C H H H H",      0, "CH4"     },
      { "C Cl Cl Cl Cl",  0, "CCl4"    },
      { "N O O",         -1, "NO2^-"   },
      { "H Cl",           0, "HCl"     },
      { "S H H",          0, "H2S"     },
      { "N H H H H",     +1, "NH4^+"   },
      { "S O O O O",     -2, "SO4^2-"  },
      { "P H H H",        0, "PH3"     },
      { 0, 0, 0 }
    };

    for (int i = 0; CASES[i].atoms != 0; i++) {
      vector<string> elements;
      istringstream parse(CASES[i].atoms);
      string symbol;
      while (parse >> symbol) elements.push_back(symbol);

      const string got = MoDiagram::formula(elements, CASES[i].charge);
      string what = string(CASES[i].want);
      if (got != CASES[i].want) what += " (got " + got + ")";
      check(got == CASES[i].want, what);
    }
  }

  printf("\n  %s\n", bad ? "FAIL" : "PASS");
  return bad ? 1 : 0;
}
