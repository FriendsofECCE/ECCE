//  The diagram's layout decisions, which are judgement calls and so
//  worth pinning: what shares a level, what counts as core, what
//  connects to what.
#include <cstdio>
#include <sstream>
#include <cmath>
#include <string>
#include <vector>
#include "tdat/MoDiagram.H"
#include "tdat/MoComposition.H"
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

  printf("\n  hiding semicore levels (#175)\n");
  {
    //  Cr(CO)6-shaped spectrum: a real core level (Cr 3s-ish, far
    //  below everything), a SEMICORE level sitting well inside the
    //  valence window with nothing separating it by a gap (Cr's 3p,
    //  -2.246 Ha against a t2g valence level at -0.354 -- exactly
    //  suggestCoreCutoff()'s blind spot, since only the biggest gap
    //  between occupied levels is a core/valence boundary and this one
    //  is not it), then genuine valence.
    //  The gap ABOVE the semicore level must stay under 1 Hartree, or
    //  suggestCoreCutoff()'s "highest gap, not biggest" rule picks IT
    //  instead and hides the semicore level as ordinary core -- filled
    //  in with intervening levels the way six CO ligands actually do,
    //  rather than the two-level jump a hand-picked test would have.
    double e[] = {-10.0, -2.246, -1.5, -1.0, -0.354, -0.10};
    double o[] = {  2.0,    2.0,   2.0,  2.0,    2.0,   2.0};
    const char* l[] = {"a1g","t1u","eg1","t2g1","t2g","a1g2"};
    vector<double> E(e, e+6), O(o, o+6);
    vector<string> L(l, l+6);

    MoColumn col;
    MoDiagram::group(E, O, L, 1e-4, col.levels);
    check(col.levels.size() == 6, "six distinct levels before any hiding");

    //  hideBelow() FIRST, on the ORIGINAL spectrum -- the order that
    //  matters (see hideSemicore()'s header): the -10.0 level is deep
    //  enough below the next occupied level (-2.246) to be the biggest
    //  gap, so the ordinary core rule alone already removes it.
    const double cut = MoDiagram::suggestCoreCutoff(col.levels);
    MoDiagram::hideBelow(col, cut);
    check(col.levels.size() == 5 && col.hiddenCount == 1,
          "the real core level (-10.0) is hidden by the ordinary rule");

    //  THEN flag the semicore level against what is LEFT -- index 0 is
    //  now -2.246, the level that survived the gap rule but is still
    //  too localised and too deep to be valence.
    check(fabs(col.levels[0].energy - (-2.246)) < 1e-9,
          "the semicore level is what remains at index 0");
    vector<bool> isSemicore(col.levels.size(), false);
    isSemicore[0] = true;
    MoDiagram::hideSemicore(col, isSemicore);
    check(col.levels.size() == 4 && col.hiddenSemicoreCount == 1,
          "the semicore level is hidden and counted under its own name");
    check(col.hiddenCount == 1,
          "core and semicore both end up hidden, counted separately");
    check(fabs(col.hiddenMaxEnergy - (-2.246)) < 1e-9,
          "hiddenMaxEnergy now reports the SEMICORE energy, not the "
          "deeper core's -- it is the higher (less negative) of the two");
  }
  {
    //  A VALENCE LONE PAIR MUST NOT BE CAUGHT.  Water's O 2a1 (about
    //  -1.0 Ha) is exactly what a caller's Lowdin-share-plus-energy
    //  rule must clear: it can be strongly localised on oxygen, but at
    //  an energy far short of 2x oxygen's tabulated 2s VOIE (-1.19 Ha
    //  -> -2.38 Ha threshold), so it is ordinary valence and stays.
    //  This test only exercises hideSemicore() itself -- the threshold
    //  arithmetic lives in MoDiagramPanel.C and is exercised live by
    //  the water fixture in tests/modiagram, which shows no semicore
    //  line before or after #175.
    double e[] = {-1.35, -1.00, -0.72, -0.57, -0.50};
    double o[] = { 2.0,   2.0,   2.0,   2.0,   2.0};
    const char* l[] = {"a1","a1","b1","a1","b2"};
    vector<double> E(e, e+5), O(o, o+5);
    vector<string> L(l, l+5);

    MoColumn col;
    MoDiagram::group(E, O, L, 1e-4, col.levels);

    vector<bool> isSemicore(col.levels.size(), false);   // nothing flagged
    MoDiagram::hideSemicore(col, isSemicore);
    check(col.levels.size() == 5 && col.hiddenSemicoreCount == 0,
          "a lone pair with no semicore flag is left alone");
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
  //  NO CENTRAL ATOM: the overlap-population rule (#132, settled
  //  2026-09-27).  Modelled on benzene's pi manifold -- four levels
  //  of four distinct irreps, none of which the (single) fragment
  //  column shares with anything on the other side, so the OLD
  //  counting rule marks every one of them "nb" (min(p,q)==0 for all
  //  four).  The OP-based rule instead reads the measured overlap
  //  population directly: a2u and e1g bonding, e2u and b2g
  //  antibonding, exactly the textbook picture.
  {
    printf("\n  no-central-atom construction: classify by overlap "
           "population\n");

    //  A fifth, distinct irrep with an OP indistinguishable from zero
    //  AGAINST THE OTHER FOUR -- there is no meaning to "near zero" in
    //  a window of one, so this is tested alongside real bonding and
    //  antibonding magnitudes, which is the only window the threshold
    //  is ever actually compared against in the real diagram too.
    vector<MoLevel> left(5), right;  // one-sided TASO column, no partner
    vector<MoLevel> centre(5);
    const char* irr[] = { "a2u", "e1g", "e2u", "b2g", "a1g" };
    for (int i = 0; i < 5; i++) {
      left[i].irrep = MoDiagram::canonicalIrrep(irr[i]);
      left[i].degeneracy = 1;
      centre[i].irrep = MoDiagram::canonicalIrrep(irr[i]);
      centre[i].degeneracy = 1;
      centre[i].energy = -0.4 + 0.2*i;   // sorted, as the real spectrum is
    }
    //  Bonding (a2u, e1g) positive, antibonding (e2u, b2g) negative,
    //  scaled so the 5%-of-largest threshold cannot mistake either for
    //  non-bonding; a1g's is three orders of magnitude smaller.
    vector<double> op(5);
    op[0] =  0.62;    // a2u  -- bonding
    op[1] =  0.58;    // e1g  -- bonding
    op[2] = -0.55;    // e2u  -- antibonding
    op[3] = -0.60;    // b2g  -- antibonding
    op[4] =  0.0003;  // a1g  -- negligible next to the other four

    MoDiagram::classify(left, centre, right, false, vector<double>(), op);

    check(centre[0].character == MoLevel::BONDING,
          "a2u (positive OP) comes out bonding, not nb from an empty count");
    check(centre[1].character == MoLevel::BONDING,
          "e1g (positive OP) comes out bonding");
    check(centre[2].character == MoLevel::ANTIBONDING,
          "e2u (negative OP) comes out antibonding");
    check(centre[3].character == MoLevel::ANTIBONDING,
          "b2g (negative OP) comes out antibonding");
    check(centre[4].character == MoLevel::NONBONDING,
          "a1g's OP is negligible next to the other four, and is left "
          "non-bonding rather than guessed either way");
    //  Each of the five appears exactly once in this fragment column --
    //  the full reduction of benzene's carbon 2pz set is a2u+e1g+e2u+b2g,
    //  one SALC of each -- so there is no SAME-irrep partner for any of
    //  them to pair with for colouring; they stay ungrouped (pairing<0),
    //  which is correct and not a sign the OP rule missed anything.
    check(centre[0].pairing < 0 && centre[1].pairing < 0 &&
          centre[2].pairing < 0 && centre[3].pairing < 0,
          "no same-irrep partner exists among these five, so none pair up");
  }

  //  ------------------------------------------------------------------
  //  THE OP THRESHOLD MUST NOT BE SET BY ONE OUTLIER (#132 follow-up,
  //  Cr(CO)6/benzene, live 2026-09-27).  A deep, strongly-bonding sigma
  //  framework level can carry an overlap population several times any
  //  other level's in the same window; a threshold set as a fraction of
  //  the WINDOW'S MAXIMUM then swamps everything weaker sharing that
  //  window -- benzene's occupied pi orbitals measured OP +0.30 to
  //  +0.45, genuinely bonding, and were marked non-bonding because one
  //  unrelated sigma level's OP of magnitude 8 put the 5%-of-maximum bar
  //  at 0.4. The threshold has to be judged against the window's own
  //  typical scale (RMS), not its single largest member.
  {
    printf("\n  no-central-atom construction: the OP threshold survives "
           "one outlier level\n");

    vector<MoLevel> left(4), right;
    vector<MoLevel> centre(4);
    const char* irr[] = { "t1u", "e1g", "e2u", "a1g" };
    for (int i = 0; i < 4; i++) {
      left[i].irrep = MoDiagram::canonicalIrrep(irr[i]);
      left[i].degeneracy = 1;
      centre[i].irrep = MoDiagram::canonicalIrrep(irr[i]);
      centre[i].degeneracy = 1;
      centre[i].energy = -0.6 + 0.2*i;
    }
    vector<double> op(4);
    op[0] =  8.0;     // t1u -- an unrelated, strongly-bonding sigma level
    op[1] =  0.35;    // e1g -- weaker, genuinely bonding pi level
    op[2] = -0.40;    // e2u -- antibonding
    op[3] =  0.0005;  // a1g -- negligible

    MoDiagram::classify(left, centre, right, false, vector<double>(), op);

    check(centre[0].character == MoLevel::BONDING,
          "the outlier's own OP is trivially above any threshold it sets");
    check(centre[1].character == MoLevel::BONDING,
          "a weaker but real bonding OP (+0.35) is not swamped by the "
          "outlier -- this is exactly what the old max-based 5% "
          "threshold got wrong (0.05*8.0 = 0.4 > 0.35)");
    check(centre[2].character == MoLevel::ANTIBONDING,
          "the antibonding level is unaffected");
    check(centre[3].character == MoLevel::NONBONDING,
          "the negligible level is still left non-bonding");
  }

  //  ------------------------------------------------------------------
  //  LOCALISATION PRE-EMPTS EVERYTHING, whichever construction this is
  //  (Andy, 2026-09-27): a level sitting almost entirely on one atom is
  //  that atom's lone pair no matter what it is being correlated
  //  against, so it must come out non-bonding even where the
  //  construction's own rule -- irrep counting here -- would otherwise
  //  call it bonding.
  {
    printf("\n  localisation pre-empts the construction's own rule\n");

    //  TWO centre levels of one irrep, one fragment orbital of that
    //  irrep on each side: the textbook in-phase/out-of-phase split,
    //  which the old counting rule gets right on its own (pairs=1,
    //  lower energy bonding, higher antibonding) -- exactly the case
    //  the localisation check must still be able to override.
    vector<MoLevel> left(1), right(1), centre(2);
    left[0].irrep = right[0].irrep = MoDiagram::canonicalIrrep("a1");
    left[0].degeneracy = right[0].degeneracy = 1;
    for (int i = 0; i < 2; i++) {
      centre[i].irrep = MoDiagram::canonicalIrrep("a1");
      centre[i].degeneracy = 1;
      centre[i].energy = -1.0 + 0.5*i;
    }

    //  Without localisedShare: the plain counting rule calls the lower
    //  (centre[0]) bonding.
    {
      vector<MoLevel> c = centre;
      MoDiagram::classify(left, c, right);
      check(c[0].character == MoLevel::BONDING,
            "sanity: with no localisation data, the old counting rule "
            "still calls this bonding");
    }

    //  With localisedShare at/above threshold: pre-empted to nb.
    //  (Only centre[0] is marked; centre[1]'s fate once its would-be
    //  partner is pre-empted is not this test's question.)
    {
      vector<MoLevel> c = centre;
      vector<double> share(2, 0.0);
      share[0] = MoComposition::LOCALISED_SHARE_THRESHOLD;
      MoDiagram::classify(left, c, right, false, share);
      check(c[0].character == MoLevel::NONBONDING,
            "a level at the localisation threshold is non-bonding "
            "regardless of what the counting rule would have said");
      check(c[0].pairing < 0, "and carries no pairing to draw a line by");
    }

    //  Just under threshold: the construction's own rule still decides.
    {
      vector<MoLevel> c = centre;
      vector<double> share(2, 0.0);
      share[0] = MoComposition::LOCALISED_SHARE_THRESHOLD - 0.05;
      MoDiagram::classify(left, c, right, false, share);
      check(c[0].character == MoLevel::BONDING,
            "just under the threshold, the counting rule still applies");
    }
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

  printf("\n  O2 triplet: merged open-shell occupancy (#132)\n");
  {
    //  Real ORCA ORBENG/ORBOCC/ORBENGBETA/ORBOCCBETA from Andy's live
    //  O2 calculation (users/andy/O2 on niobium), which reports NO
    //  ORBSYM at all -- exactly the case matchAlphaBeta() falls back to
    //  energy-ordered position for.  9 alpha orbitals occupied, 7 beta:
    //  16 electrons, S=1, two unpaired alpha electrons in 1pi-g*.
    double eA[] = {-20.74006,-20.73959,-1.74520,-1.16234,-0.86383,-0.86383,
                   -0.78126,-0.53798,-0.53798, 0.51152};
    double oA[] = {  1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0 };
    double eB[] = {-20.68416,-20.68301,-1.62277,-0.95821,-0.71459,-0.60321,
                   -0.60321, 0.12981, 0.12981};
    double oB[] = {  1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0 };
    vector<double> EA(eA, eA+10), OA(oA, oA+10), EB(eB, eB+9), OB(oB, oB+9);
    vector<string> noLabels;
    map<string,int> noDims;

    vector<MoLevel> levels;
    MoDiagram::groupByIrrep(EA, OA, noLabels, noDims, 1e-4, levels);

    //  Falls back to plain energy grouping, exactly as it does with real
    //  ORBSYM missing -- confirms the levels a diagram would actually
    //  draw, not a hand-picked grouping.
    check(levels.size() == 8,
          "2 core + 2sg + 2su + 1piu(2) + 3sg + 1pig*(2) + LUMO = 8 levels");

    if (levels.size() == 8) {
      //  Indices: 0=1sg,1su core (merged, both -20.7x within 1e-4? they
      //  are 0.00047 Ha apart, so tolerance keeps them separate --
      //  hence 8 levels, not 7).  Levels 2..7 are the valence ladder.
      MoLevel& sigma2g   = levels[2];
      MoLevel& sigma2u   = levels[3];
      MoLevel& piu       = levels[4];
      MoLevel& sigma3g   = levels[5];
      MoLevel& pigStar   = levels[6];

      check(fabs(sigma2g.energy - (-1.74520)) < 1e-4 &&
            sigma2g.degeneracy == 1 && fabs(sigma2g.occupancy-1.0) < 1e-9,
            "2sigma_g: one orbital, alpha occupancy 1");
      check(fabs(sigma2u.energy - (-1.16234)) < 1e-4 &&
            sigma2u.degeneracy == 1 && fabs(sigma2u.occupancy-1.0) < 1e-9,
            "2sigma_u: one orbital, alpha occupancy 1");
      check(piu.degeneracy == 2 && fabs(piu.occupancy-2.0) < 1e-9,
            "1pi_u: two orbitals, alpha occupancy 2 (one each)");
      check(fabs(sigma3g.energy - (-0.78126)) < 1e-4 &&
            sigma3g.degeneracy == 1 && fabs(sigma3g.occupancy-1.0) < 1e-9,
            "3sigma_g: one orbital, alpha occupancy 1");
      check(pigStar.degeneracy == 2 && fabs(pigStar.occupancy-2.0) < 1e-9,
            "1pi_g*: two orbitals, alpha occupancy 2 (one each, unpaired)");

      //  --- matched beta, positional fallback (no labels available) ---
      //
      //  Same fallback matchAlphaBeta() uses in MoDiagramPanel.C, done
      //  by hand here since that function is static to that file: the
      //  k-th alpha orbital's beta count is the k-th beta orbital's
      //  occupancy, by plain index.
      MoLevel* valence[5] = { &sigma2g, &sigma2u, &piu, &sigma3g, &pigStar };
      for (int v = 0; v < 5; v++) {
        MoLevel& lev = *valence[v];
        double betaSum = 0.0;
        bool ok = true;
        for (size_t k = 0; k < lev.orbitals.size(); k++) {
          const int a = lev.orbitals[k];
          if (a < 0 || a >= (int)OB.size()) { ok = false; break; }
          betaSum += OB[a];
        }
        if (ok) lev.occupancyBeta = betaSum;
      }

      check(fabs(sigma2g.occupancyBeta - 1.0) < 1e-9, "2sigma_g: beta 1 (paired)");
      check(fabs(sigma2u.occupancyBeta - 1.0) < 1e-9, "2sigma_u: beta 1 (paired)");
      check(fabs(piu.occupancyBeta - 2.0) < 1e-9,
            "1pi_u: beta 2 -- fully paired despite the label mismatch "
            "the positional fallback cannot see");
      check(fabs(sigma3g.occupancyBeta - 1.0) < 1e-9,
            "3sigma_g: beta 1 -- fully paired");
      check(fabs(pigStar.occupancyBeta - 0.0) < 1e-9,
            "1pi_g*: beta 0 -- the two unpaired electrons are here, "
            "and only here");

      //  --- the per-orbital arrow fill MoDiagramCanvas::drawElectrons()
      //  computes, replicated here since that method is private to a wx
      //  class this test does not link against.  What matters is the
      //  CONTRACT: alpha and beta filled independently, Hund's rule each,
      //  which is what turns 1pi_g*'s (2, 0) into two up-only arrows
      //  rather than one paired orbital and one empty one.
      {
        const int count = pigStar.degeneracy;
        const int totalA = (int)(pigStar.occupancy + 0.5);
        const int totalB = (int)(pigStar.occupancyBeta + 0.5);
        int upOnly = 0, paired = 0, empty = 0;
        for (int d = 0; d < count; d++) {
          const bool up = d < totalA, down = d < totalB;
          if (up && down) paired++;
          else if (up) upOnly++;
          else if (!up && !down) empty++;
        }
        check(upOnly == 2 && paired == 0 && empty == 0,
              "1pi_g* draws as two unpaired up arrows, not one pair");
      }
      {
        const int count = sigma2g.degeneracy;
        const int totalA = (int)(sigma2g.occupancy + 0.5);
        const int totalB = (int)(sigma2g.occupancyBeta + 0.5);
        int paired = 0;
        for (int d = 0; d < count; d++) if (d < totalA && d < totalB) paired++;
        check(paired == 1, "2sigma_g draws as one paired (up+down) arrow");
      }
    }
  }

  printf("\n  placeFragments keeps a fragment's own shell order\n");

  //  A synthetic Cr-like fragment: 3d below 4s below 4p in the free
  //  atom, but the weighted-mean placement below inverts it -- the
  //  shell connected to the lowest molecular orbital is 4p, so a
  //  bare mean would draw 4p at the bottom, under the non-bonding 3d.
  {
    MoColumn centre;
    centre.levels.resize(3);
    centre.levels[0].energy = -1.45;
    centre.levels[1].energy = -0.12;
    centre.levels[2].energy =  0.20;
    for (size_t i = 0; i < 3; i++) centre.levels[i].occupancy = 2.0;

    MoColumn left;
    left.shellKeys.push_back("Cr:0");   // 0: s
    left.shellKeys.push_back("Cr:1");   // 1: p
    left.shellKeys.push_back("Cr:2");   // 2: d
    left.levels.resize(3);

    MoLevel& d = left.levels[0];
    MoLevel& s = left.levels[1];
    MoLevel& p = left.levels[2];
    d.slot = 2; d.energy = -0.26*27.211386; d.moWeight.assign(3, 0.0);
    s.slot = 0; s.energy = -0.24*27.211386; s.moWeight.assign(3, 0.0);
    p.slot = 1; p.energy = -0.13*27.211386; p.moWeight.assign(3, 0.0);

    //  Inverted case: d -> mid MO, s -> top MO, p -> bottom MO.
    d.moWeight[1] = 1.0;
    s.moWeight[2] = 1.0;
    p.moWeight[0] = 1.0;

    MoColumn right;   // empty: this diagram has no right-hand fragment

    vector<MoConnection> connections;
    MoConnection cd; cd.leftLevel = 0; cd.centreLevel = 1; cd.rightLevel = -1;
    MoConnection cs; cs.leftLevel = 1; cs.centreLevel = 2; cs.rightLevel = -1;
    MoConnection cp; cp.leftLevel = 2; cp.centreLevel = 0; cp.rightLevel = -1;
    connections.push_back(cd);
    connections.push_back(cs);
    connections.push_back(cp);

    MoDiagram::placeFragments(centre, left, right, connections);

    check(left.levels[0].energy < left.levels[1].energy,
          "3d ends up below 4s");
    check(left.levels[1].energy < left.levels[2].energy,
          "4s ends up below 4p");
    check(fabs(left.levels[0].energy - (-0.12)) < 1e-9,
          "3d itself is not moved (nothing below it to collide with)");
    check(fabs(left.levels[1].energy - 0.20) < 1e-9,
          "4s itself is not moved either (already above 3d with room)");
    check(left.levels[2].energy > 0.20,
          "4p is pushed up off its raw mean of -1.45");
  }

  //  Same shells, already in the right order and with room to spare:
  //  the reordering step must leave the means it is handed untouched,
  //  bit for bit.
  {
    MoColumn centre;
    centre.levels.resize(3);
    centre.levels[0].energy = -1.45;
    centre.levels[1].energy = -0.12;
    centre.levels[2].energy =  0.20;
    for (size_t i = 0; i < 3; i++) centre.levels[i].occupancy = 2.0;

    MoColumn left;
    left.shellKeys.push_back("Cr:0");
    left.shellKeys.push_back("Cr:1");
    left.shellKeys.push_back("Cr:2");
    left.levels.resize(3);

    MoLevel& d = left.levels[0];
    MoLevel& s = left.levels[1];
    MoLevel& p = left.levels[2];
    d.slot = 2; d.energy = -0.26*27.211386; d.moWeight.assign(3, 0.0);
    s.slot = 0; s.energy = -0.24*27.211386; s.moWeight.assign(3, 0.0);
    p.slot = 1; p.energy = -0.13*27.211386; p.moWeight.assign(3, 0.0);

    //  Already in order: d -> bottom MO, s -> mid MO, p -> top MO.
    d.moWeight[0] = 1.0;
    s.moWeight[1] = 1.0;
    p.moWeight[2] = 1.0;

    MoColumn right;
    vector<MoConnection> connections;
    MoConnection cd; cd.leftLevel = 0; cd.centreLevel = 0; cd.rightLevel = -1;
    MoConnection cs; cs.leftLevel = 1; cs.centreLevel = 1; cs.rightLevel = -1;
    MoConnection cp; cp.leftLevel = 2; cp.centreLevel = 2; cp.rightLevel = -1;
    connections.push_back(cd);
    connections.push_back(cs);
    connections.push_back(cp);

    MoDiagram::placeFragments(centre, left, right, connections);

    check(fabs(left.levels[0].energy - (-1.45)) < 1e-12,
          "3d unchanged, bit-identical to its raw mean");
    check(fabs(left.levels[1].energy - (-0.12)) < 1e-12,
          "4s unchanged, bit-identical to its raw mean");
    check(fabs(left.levels[2].energy - 0.20) < 1e-12,
          "4p unchanged, bit-identical to its raw mean");
  }

  //  Shaped like Cr(CO)6's metal column: 3d split into eg (connected
  //  to nothing) and t2g, 4p connected only to a deep level.  The
  //  unconnected eg must not be averaged into t2g while still in eV,
  //  and no fallback may run after the shell order is fixed.
  {
    MoColumn centre;
    const double mo[5] = { -2.246, -0.354, 0.071, 0.146, 0.348 };
    centre.levels.resize(5);
    for (int i = 0; i < 5; i++) {
      centre.levels[i].energy = mo[i];
      centre.levels[i].occupancy = (i < 2) ? 2.0 : 0.0;
      centre.levels[i].degeneracy = 1;
    }

    MoColumn left;
    left.shellKeys.push_back("Cr:0");
    left.shellKeys.push_back("Cr:1");
    left.shellKeys.push_back("Cr:2");
    left.levels.resize(4);
    MoLevel& eg  = left.levels[0];
    MoLevel& t2g = left.levels[1];
    MoLevel& s   = left.levels[2];
    MoLevel& p   = left.levels[3];
    eg.slot  = 2; eg.shell  = 2; eg.energy  = -7.2;
    t2g.slot = 2; t2g.shell = 2; t2g.energy = -7.2;
    s.slot   = 0; s.shell   = 0; s.energy   = -6.6;
    p.slot   = 1; p.shell   = 1; p.energy   = -3.5;
    for (int i = 0; i < 4; i++) left.levels[i].moWeight.assign(5, 0.0);
    t2g.moWeight[1] = 0.74; t2g.moWeight[3] = 0.31;
    s.moWeight[2] = 0.96;   s.moWeight[4] = 0.76;
    p.moWeight[0] = 0.99;

    MoColumn right;
    vector<MoConnection> connections;
    const int links[5][2] = { {1,1}, {1,3}, {2,2}, {2,4}, {3,0} };
    for (int k = 0; k < 5; k++) {
      MoConnection c;
      c.leftLevel = links[k][0]; c.centreLevel = links[k][1];
      c.rightLevel = -1;
      connections.push_back(c);
    }

    MoDiagram::placeFragments(centre, left, right, connections);

    const double lo = -2.246, hi = 0.348, gap = 0.03*(hi - lo);
    bool inside = true;
    for (int i = 0; i < 3; i++) {
      if (left.levels[i].energy < lo || left.levels[i].energy > hi) {
        inside = false;
      }
    }
    check(inside, "3d and 4s stay inside the molecular range");
    check(fabs(eg.energy - t2g.energy) < 1e-12,
          "eg and t2g of one 3d shell share a row");
    check(t2g.energy + gap <= s.energy + 1e-12 &&
          eg.energy + gap <= s.energy + 1e-12, "all of 3d below 4s by the gap");
    check(s.energy + gap <= p.energy + 1e-12, "4s below 4p by the gap");
  }

  //  ------------------------------------------------------------------
  //  SKELETON: composition beats the counting rule (#183, Cr(CO)6, live
  //  2026-09-28).  Modelled directly on the real orca-crco6 capture's
  //  Eg set: an Eg irrep with one metal (3d) level and one ligand
  //  (sigma) level gives the counting rule p=q=1, pairs=1 -- lowest
  //  energy bonding, highest antibonding, whatever they actually are.
  //  In Cr(CO)6 the three real Eg molecular levels are a CO-internal
  //  combination at the bottom (2Eg, Cr Lowdin share 0.005), the real
  //  sigma-bonding eg in the middle (4Eg, Cr share 0.36), and the
  //  metal-heavy eg* that sets Delta_o at the top (5Eg, Cr share 0.52)
  //  -- so the counting rule picks the CO-internal level as "bonding"
  //  and leaves the real bonding level marked non-bonding.  Composition
  //  (metalShare + metalLigandOP) gets all three right.
  {
    printf("\n  SKELETON: metal-ligand composition, not the counting "
           "rule (Cr(CO)6's Eg set)\n");

    vector<MoLevel> left(1), right(1);
    left[0].irrep = right[0].irrep = MoDiagram::canonicalIrrep("eg");
    left[0].degeneracy = right[0].degeneracy = 2;

    vector<MoLevel> centre(3);
    centre[0].irrep = centre[1].irrep = centre[2].irrep =
        MoDiagram::canonicalIrrep("eg");
    centre[0].degeneracy = centre[1].degeneracy = centre[2].degeneracy = 2;
    centre[0].energy = -1.505;   // "2Eg": CO-internal, near-zero on Cr
    centre[1].energy = -0.613;   // "4Eg": the real sigma-bonding eg
    centre[2].energy =  0.284;   // "5Eg*": metal-heavy, sets Delta_o

    //  Sanity: with no composition, the counting rule picks the energy
    //  extremes -- this is the wrong answer #183 exists to fix, kept
    //  here so a future change to the counting rule itself is visible.
    {
      vector<MoLevel> c = centre;
      MoDiagram::classify(left, c, right);
      check(c[0].character == MoLevel::BONDING,
            "sanity: the plain counting rule calls the CO-internal "
            "extreme bonding -- the bug #183 fixes");
      check(c[1].character == MoLevel::NONBONDING,
            "sanity: and leaves the real bonding level non-bonding");
    }

    vector<double> metalShare(3), metalLigandOP(3);
    metalShare[0] = 0.005;  metalLigandOP[0] =  0.006;  // one-sided: nb
    metalShare[1] = 0.36;   metalLigandOP[1] =  0.25;   // real bonding
    metalShare[2] = 0.52;   metalLigandOP[2] = -0.50;   // real antibonding

    MoDiagram::classify(left, centre, right, false, vector<double>(),
                        vector<double>(), metalShare, metalLigandOP);

    check(centre[0].character == MoLevel::NONBONDING,
          "the CO-internal level (Cr share 0.005, below the 0.10 floor) "
          "is non-bonding, not the energy-extreme \"bonding\" the "
          "counting rule picked");
    check(centre[1].character == MoLevel::BONDING,
          "the real sigma-bonding eg is bonding");
    check(centre[2].character == MoLevel::ANTIBONDING,
          "the metal-heavy eg* is antibonding");
    check(centre[0].pairing < 0,
          "the non-bonding CO-internal level carries no pairing");
    check(centre[1].pairing >= 0 && centre[1].pairing == centre[2].pairing,
          "the real bonding/antibonding pair share a colour -- this is "
          "Delta_o's eg/eg* pair");

    //  connect(): both sides qualify, and the cap does not drop a
    //  qualifying side (#183).  The non-bonding CO-internal level still
    //  carries real (>=10%) share on both fragment columns -- unlike
    //  the ordinary rule, a SKELETON draws both lines for it rather
    //  than picking the side it is "more on".
    left[0].shareLeft = -1.0;  // unused on this side; shares live on centre
    centre[0].shareLeft = 0.30; centre[0].shareRight = 0.70;
    centre[1].shareLeft = 0.36; centre[1].shareRight = 0.64;
    centre[2].shareLeft = 0.52; centre[2].shareRight = 0.48;

    vector<MoConnection> links;
    MoDiagram::connect(left, centre, right, links, 0.10, true);

    bool level0Left = false, level0Right = false;
    for (size_t k = 0; k < links.size(); k++) {
      if (links[k].centreLevel != 0) continue;
      if (links[k].leftLevel  >= 0) level0Left  = true;
      if (links[k].rightLevel >= 0) level0Right = true;
    }
    check(level0Left && level0Right,
          "a SKELETON's non-bonding level still draws to BOTH sides it "
          "has real share on, unlike the ordinary one-sided rule");
  }

  printf("\n  %s\n", bad ? "FAIL" : "PASS");
  return bad ? 1 : 0;
}
