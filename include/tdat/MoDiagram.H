/**
 * @file
 *
 * The layout of a molecular orbital correlation diagram.
 *
 * Deliberately separate from anything that draws: what to put where is
 * decided here, in terms of levels, labels and connections, and the
 * panel only paints the result.  That keeps the part with the judgement
 * in it -- which orbitals share a level, which count as core, what
 * connects to what -- testable without a display.
 */

#ifndef MODIAGRAM_HH
#define MODIAGRAM_HH

#include <map>
#include <string>
#include <vector>
using std::map;
using std::string;
using std::vector;

/** One drawn level: a set of orbitals sharing an energy and a label. */
struct MoLevel
{
   double         energy;          // Hartree, as drawn
   string         label;           // what to draw
   string         irrep;           // canonical irrep name, for matching
   string         annotation;      // extra text, e.g. a fragment's VOIE

   /** What the level is: see MoDiagram::classify(). */
   enum Character { UNKNOWN, BONDING, NONBONDING, ANTIBONDING };
   Character      character;

   /**
    * Which bonding/antibonding pair this level belongs to, or -1.
    *
    * A bonding level and the antibonding level it is paired with carry
    * the same number, so a diagram can colour them together: seeing
    * which antibonding orbital goes with which bonding one is most of
    * what the picture is for, and it cannot be read off the order.
    */
   int            pairing;
   double         occupancy;       // electrons in the whole level
   int            degeneracy;      // orbitals drawn as this one level
   vector<int>    orbitals;        // their indices, for clicking through

   /**
    * Each orbital's own energy, in the same order as `orbitals`.
    *
    * `energy` is their mean, which is where the level is drawn when
    * they agree.  When they do NOT -- a degenerate set split by finite
    * precision, or by a symmetry the geometry only nearly has -- a
    * diagram draws them at their own heights and brackets them with a
    * brace to say they are one level.  That needs the individual
    * values, so they are kept.
    */
   vector<double> energies;

   MoLevel() : energy(0.0), occupancy(0.0), degeneracy(0),
               character(UNKNOWN), pairing(-1) {}
};


/** One column of the diagram. */
struct MoColumn
{
   string           title;
   vector<MoLevel>  levels;

   /**
    * Orbitals folded away below the energy cutoff.
    *
    * Drawn as a single band rather than as levels: a molecule with
    * thirty core orbitals squeezes its valence region into nothing
    * otherwise.  Kept as a count so the diagram can SAY how many are
    * hidden -- an absence the reader cannot see is worse than a
    * cluttered diagram.
    */
   int    hiddenCount;
   double hiddenMaxEnergy;

   /** Virtuals folded away at the top, counted the same way. */
   int    hiddenAboveCount;

   MoColumn() : hiddenCount(0), hiddenMaxEnergy(0.0), hiddenAboveCount(0) {}
};


/** A line between a fragment level and a molecular orbital. */
struct MoConnection
{
   int leftLevel;     // index into the left column, or -1
   int centreLevel;   // index into the centre column
   int rightLevel;    // index into the right column, or -1
};


class MoDiagram
{
  public:

    MoDiagram();

    /**
     * Group orbitals into drawn levels.
     *
     * Orbitals are one level when they share an irrep label AND lie
     * within tolerance in energy.  The degeneracy that results is
     * reported, but a caller that knows the irrep's dimension should
     * prefer that: an accidental near-degeneracy is not a degenerate
     * set, and grouping by energy alone cannot tell them apart.
     *
     * @param energies     one per orbital, Hartree, ascending
     * @param occupancies  one per orbital
     * @param labels       one per orbital; may be empty, in which case
     *                     grouping is by energy alone and every level
     *                     is labelled unknown
     * @param tolerance    Hartree
     */
    /**
     * An irrep name in one spelling, so two sources can be compared.
     *
     * The codes and the character tables do not agree on how to write
     * these.  ORCA reports A" where the table has A''; codes generally
     * print a1 where a table prints A1.  An exact comparison therefore
     * finds no partner for half the levels and draws no correlation
     * line, with nothing anywhere saying why -- the diagram simply
     * comes out disconnected and plausible.
     *
     * Upper-cased, whitespace removed, and a double quote written as
     * the two single quotes it stands for.
     */
    static string canonicalIrrep(const string& label);

    static bool group(const vector<double>& energies,
                      const vector<double>& occupancies,
                      const vector<string>& labels,
                      double tolerance,
                      vector<MoLevel>& levels);

    /**
     * Group orbitals into levels, using the irreps' own dimensions.
     *
     * DEGENERACY IS A PROPERTY OF THE IRREP, not of how close two
     * energies happen to be.  A t2 level is three orbitals because T2
     * is three dimensional, and it stays three orbitals however far
     * apart a finite-precision calculation puts them -- methane from a
     * semiempirical run spreads its 1t2 over 2e-4 Hartree, which any
     * tolerance tight enough to separate real levels will split.
     *
     * So consecutive orbitals carrying the same label are taken in
     * blocks of that irrep's dimension.  No tolerance is involved, and
     * an accidental near-degeneracy of two different irreps cannot be
     * mistaken for a degenerate set.
     *
     * @param dimensions  irrep name (canonical) to dimension; a label
     *                    that is not in it falls back to the tolerance
     */
    static bool groupByIrrep(const vector<double>& energies,
                             const vector<double>& occupancies,
                             const vector<string>& labels,
                             const map<string,int>& dimensions,
                             double tolerance,
                             vector<MoLevel>& levels);

    /**
     * Place fragment levels where a correlation diagram puts them.
     *
     * Not at their tabulated energies on the molecular axis: those are
     * free-atom eV values and the molecular levels are whatever the
     * calculation computed, sharing no zero, method or scale.  Drawn on
     * one axis they produced a bonding orbital ABOVE the atomic orbital
     * it is built from, which cannot happen.
     *
     * The levels are placed in the gap between the highest occupied and
     * lowest empty molecular orbital, keeping the order and relative
     * spacing the tabulated energies give -- the more electronegative
     * fragment lower, which is the pedagogical content -- so every
     * bonding level lies below its parents and every antibonding one
     * above, by construction.  Each level keeps its own value as an
     * annotation, so the numbers stay on the diagram.
     */
    static void placeFragments(const MoColumn& centre,
                               MoColumn& left, MoColumn& right);

    /**
     * Fold levels below a cutoff into the column's hidden band.
     *
     * @param cutoff  Hartree; levels below this are hidden
     */
    static void hideBelow(MoColumn& column, double cutoff);

    /**
     * Fold levels ABOVE a cutoff into the column's hidden band.
     *
     * The high virtuals are the diagram's other clutter problem.  They
     * are basis-set artefacts more than chemistry: what a correlation
     * diagram is about is the bonding levels and their antibonding
     * partners, with the odd non-bonding one between.
     */
    static void hideAbove(MoColumn& column, double cutoff);

    /**
     * A cutoff that keeps the antibonding partners and drops the rest.
     *
     * One virtual level is kept for each occupied one, since that is
     * what an occupied level can pair with, plus a couple of spare so a
     * small molecule does not lose its LUMO region to an off-by-one.
     * Returns a value above everything when there is nothing worth
     * folding.
     */
    static double suggestVirtualCutoff(const vector<MoLevel>& levels);

    /**
     * A cutoff that folds away genuine core orbitals and nothing else.
     *
     * A core orbital is OCCUPIED and lies an enormous distance below
     * the valence, so only gaps between two occupied levels count, and
     * only ones bigger than @p minimumGap.  Both conditions are load
     * bearing: the largest gap in a small molecule's spectrum is the
     * HOMO-LUMO gap, and a rule that looked for the largest gap low in
     * the spectrum folded away methane's two BONDING levels and drew
     * the two antibonding ones on their own.
     *
     * @param minimumGap  in the same unit as the energies; the default
     *                    is 1 Hartree, which no valence separation
     *                    approaches and every core sits far beyond
     * @return a value below everything when there is no core, which
     *         hides nothing
     */
    static double suggestCoreCutoff(const vector<MoLevel>& levels,
                                    double minimumGap = 1.0);

    /**
     * Connect fragment levels to molecular orbitals by matching irrep.
     *
     * Symmetry is the whole rule: orbitals of different irreps cannot
     * mix, and orbitals of the same one must be allowed to.  A centre
     * level with no fragment partner of its irrep is non-bonding and
     * gets a connection with -1 on that side.
     */
    /**
     * Mark each molecular level bonding, non-bonding or antibonding.
     *
     * By counting, which is all the symmetry decomposition allows and
     * all it needs.  For one irrep, let the two fragments contribute p
     * and q levels of it.  Those p+q orbitals combine into p+q
     * molecular orbitals of that irrep: min(p,q) bonding, min(p,q)
     * antibonding, and |p-q| left over with nothing on the other side
     * to pair with, which is what non-bonding means.  Lowest in energy
     * are the bonding ones and highest the antibonding ones.
     *
     * Water is the case that shows it working.  For a1 the oxygen
     * brings 2s and 2pz and the hydrogens one TASO, so p=2, q=1: one
     * bonding, one antibonding, one non-bonding -- and water has
     * exactly three a1 orbitals.  For b1 the oxygen brings 2px and the
     * hydrogens nothing at all, so the single b1 is non-bonding, which
     * is the lone pair, arrived at rather than asserted.
     *
     * Each bonding level is paired with the antibonding level it
     * combines into, counting inwards from both ends.
     */
    /**
     * Make the fragments' irrep names agree with the code's.
     *
     * THE TWO SOURCES DO NOT SHARE AN AXIS CONVENTION.  In C2v, which
     * of the two mirror planes is called sigma-v and which sigma-v'
     * decides whether an orbital is b1 or b2, and there is no universal
     * answer: for water the character table's frame puts the two
     * hydrogens' antisymmetric combination in B1 while MOPAC calls the
     * corresponding molecular orbital b2.
     *
     * Left alone this is silent and wrong in the worst way.  The b2
     * molecular orbital finds no fragment partner and is drawn as
     * non-bonding, while the b1 lone pair finds the b1 TASO and is
     * drawn as bonding -- the diagram comes out confidently inside out.
     *
     * It is also detectable, because the two sides must span the same
     * representation: every molecular orbital is a combination of the
     * fragment orbitals, so the multiset of irreps has to match.  When
     * it does not, and exchanging two irreps of equal dimension makes
     * it match, that exchange is a relabelling of the same group under
     * a different choice of axes and is applied.  When nothing makes
     * them match, the disagreement is real and is reported rather than
     * papered over.
     *
     * @param why  filled with an explanation when they cannot be
     *             reconciled
     * @return false when the two sides genuinely disagree
     */
    static bool reconcile(vector<MoLevel>& left, vector<MoLevel>& right,
                          const vector<MoLevel>& centre, string& why);

    static void classify(const vector<MoLevel>& left,
                         vector<MoLevel>& centre,
                         const vector<MoLevel>& right);

    static void connect(const vector<MoLevel>& left,
                        const vector<MoLevel>& centre,
                        const vector<MoLevel>& right,
                        vector<MoConnection>& connections);
};

#endif
