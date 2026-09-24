//  The diagram's layout decisions, which are judgement calls and so
//  worth pinning: what shares a level, what counts as core, what
//  connects to what.
#include <cstdio>
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

  printf("\n  connecting by symmetry\n");
  {
    //  Water: the hydrogen TASOs are a1 + b1; oxygen brings a1 and b1
    //  and b2.  The b2 has no partner and must come out non-bonding.
    //  On .irrep, not .label: the drawn label carries the shell for a
    //  fragment level ("A1  (2s)") and whatever spelling the code uses
    //  for a molecular one, so matching on it would never work.  The
    //  mixed case here is deliberate -- a code reports a1, a character
    //  table says A1, and canonicalIrrep is what reconciles them.
    vector<MoLevel> left(3), centre(3), right(2);
    left[0].irrep   = MoDiagram::canonicalIrrep("A1");
    left[1].irrep   = MoDiagram::canonicalIrrep("B1");
    left[2].irrep   = MoDiagram::canonicalIrrep("B2");
    centre[0].irrep = MoDiagram::canonicalIrrep("a1");
    centre[1].irrep = MoDiagram::canonicalIrrep("b1");
    centre[2].irrep = MoDiagram::canonicalIrrep("b2");
    right[0].irrep  = MoDiagram::canonicalIrrep("A1");
    right[1].irrep  = MoDiagram::canonicalIrrep("B1");

    vector<MoConnection> links;
    MoDiagram::connect(left, centre, right, links);
    check(links.size() == 3, "one connection per molecular orbital");
    check(links[0].leftLevel == 0 && links[0].rightLevel == 0,
          "the a1 MO connects to both sides");
    check(links[2].leftLevel == 2 && links[2].rightLevel == -1,
          "the b2 MO has no TASO partner: non-bonding, as the lone pair is");
  }

  printf("\n  %s\n", bad ? "FAIL" : "PASS");
  return bad ? 1 : 0;
}
