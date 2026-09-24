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

  //  ------------------------------------------------------------------
  //  AN OCTAHEDRAL COMPLEX, AGAINST THE COURSE'S OWN CONSTRUCTION.
  //
  //  The sigma-only ligand field diagram for ML6 is the one every
  //  inorganic course is built around, and its content is a single
  //  sentence: the six ligand sigma donors span a1g + eg + t1u, the
  //  metal offers a1g (4s), t1u (4p) and eg + t2g (3d), so a1g, eg and
  //  t1u each give a bonding and an antibonding combination while t2g
  //  has no partner at all and stays non-bonding at essentially the
  //  metal 3d energy.  The gap from it to eg* is Delta-o.
  //
  //  The spectrum below is a real NWChem B3LYP/3-21G calculation on
  //  [CoH6]3+ in Oh -- cobalt(III), d6, closed shell -- with NWChem's
  //  own symmetry labels.  What is checked is that the engine reaches
  //  that construction from it, because every individual rule here
  //  (core folding, degenerate grouping, reconciliation against a
  //  minimal fragment model) was got wrong at least once on this
  //  molecule and each time the result still LOOKED like a diagram.
  {
    printf("\n  An octahedral complex\n");

    struct Orbital { double energy; double occupancy; const char* label; };
    static const Orbital SPECTRUM[] = {
      { -276.5499000, 2.0, "a1g" },
      {  -33.4469600, 2.0, "a1g" },
      {  -29.0217200, 2.0, "t1u" }, {  -29.0217200, 2.0, "t1u" },
      {  -29.0217200, 2.0, "t1u" },
      {   -4.4698950, 2.0, "a1g" },
      {   -3.1356730, 2.0, "t1u" }, {   -3.1356730, 2.0, "t1u" },
      {   -3.1356730, 2.0, "t1u" },
      {   -1.2108750, 2.0, "a1g" },
      {   -1.0991500, 2.0, "eg"  }, {   -1.0991500, 2.0, "eg"  },
      {   -0.9836387, 2.0, "t2g" }, {   -0.9836387, 2.0, "t2g" },
      {   -0.9836387, 2.0, "t2g" },
      {   -1.0227410, 0.0, "t1u" }, {   -1.0227410, 0.0, "t1u" },
      {   -1.0227410, 0.0, "t1u" },
      {   -0.7208210, 0.0, "eg"  }, {   -0.7208210, 0.0, "eg"  },
      {   -0.5077951, 0.0, "a1g" },
      {   -0.4694974, 0.0, "t1u" }, {   -0.4694974, 0.0, "t1u" },
      {   -0.4694974, 0.0, "t1u" },
      {    0.0,       0.0, 0 }
    };

    vector<double> energies, occupancies;
    vector<string> labels;
    for (int i = 0; SPECTRUM[i].label != 0; i++) {
      energies.push_back(SPECTRUM[i].energy);
      occupancies.push_back(SPECTRUM[i].occupancy);
      labels.push_back(SPECTRUM[i].label);
    }

    map<string,int> dimensions;
    dimensions["A1G"] = 1;
    dimensions["EG"]  = 2;
    dimensions["T1U"] = 3;
    dimensions["T2G"] = 3;

    vector<MoLevel> centre;
    MoDiagram::groupByIrrep(energies, occupancies, labels, dimensions,
                            1.0e-4, centre);

    //  Sorted, whatever order the code printed them in: NWChem put an
    //  empty t1u before an occupied t2g.
    bool ordered = true;
    for (size_t i = 1; i < centre.size(); i++) {
      if (centre[i].energy < centre[i-1].energy) ordered = false;
    }
    check(ordered, "the levels come out in energy order");

    //  A degenerate set is three orbitals at ONE energy, never two
    //  levels that happen to be adjacent in the list.
    bool clean = true;
    for (size_t i = 0; i < centre.size(); i++) {
      for (size_t k = 1; k < centre[i].energies.size(); k++) {
        if (fabs(centre[i].energies[k] - centre[i].energies[0]) > 1.0e-4) {
          clean = false;
        }
      }
      if (centre[i].occupancy > 0.0 &&
          centre[i].occupancy < 2.0*centre[i].degeneracy - 1.0e-9 &&
          centre[i].occupancy != 0.0) {
        //  partial occupancy is legal; a bonding level with NONE is
        //  what the grouping bug produced, and is caught below
      }
    }
    check(clean, "no level mixes orbitals of different energies");

    MoColumn column;
    column.levels = centre;
    MoDiagram::hideBelow(column, MoDiagram::suggestCoreCutoff(column.levels));

    //  Cobalt's core is 1s 2s 2p 3s 3p: nine orbitals.  Taking the
    //  BIGGEST gap instead of the highest big one folded the 1s alone.
    check(column.hiddenCount == 9,
          "the nine core orbitals fold, not just the 1s");

    //  Every level left is valence, and the occupied ones are the
    //  bonding a1g and eg and the non-bonding t2g.
    int t2g = -1, egBonding = -1, egStar = -1, a1gBonding = -1;
    for (size_t i = 0; i < column.levels.size(); i++) {
      const string& irrep = column.levels[i].irrep;
      const bool full = column.levels[i].occupancy > 0.0;
      if (irrep == "T2G" && full)            t2g = (int)i;
      if (irrep == "EG"  && full)            egBonding = (int)i;
      if (irrep == "EG"  && !full && egStar < 0)  egStar = (int)i;
      if (irrep == "A1G" && full)            a1gBonding = (int)i;
    }

    check(t2g >= 0 && egBonding >= 0 && egStar >= 0 && a1gBonding >= 0,
          "a1g and eg bonding, t2g occupied, eg* empty");

    if (t2g >= 0 && egStar >= 0) {
      check(column.levels[t2g].occupancy == 6.0,
            "t2g holds all six d electrons: low spin");
      check(column.levels[egStar].energy > column.levels[t2g].energy,
            "eg* lies above t2g, and the gap is Delta-o");
    }
  }

  printf("\n  %s\n", bad ? "FAIL" : "PASS");
  return bad ? 1 : 0;
}
