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

   /**
    * How much of this level sits on each fragment, 0 to 1.
    *
    * Symmetry says which orbitals MAY mix; only the calculation says
    * which ones DO.  In C2v every a1 orbital is allowed to mix with
    * every other a1 orbital, so matching on the label alone draws a
    * line from each to all of them and a diagram of anything bigger
    * than water comes out as a web.  These are the shares a population
    * analysis gives, and a level that carries almost nothing of a
    * fragment gets no line to it.
    *
    * Negative when no composition was available, which is not the same
    * as zero and must not be treated as it: the diagram then falls back
    * to symmetry alone rather than deciding everything is unconnected.
    */
   double         shareLeft;
   double         shareRight;

   /**
    * The same, split by the angular momentum it sits in: [0] is s,
    * [1] p, [2] d.  Empty when not computed.
    *
    * Per-atom shares cannot connect a diatomic.  Every orbital of a
    * homonuclear diatomic is exactly half on each atom -- by symmetry,
    * not by accident -- so asking which atom a sigma-g came from has
    * no answer.  Asking which SHELL it came from does: that is the
    * whole content of a diatomic's correlation lines, 2s to the lower
    * sigma pair and 2p to the upper one and the pi orbitals.
    */
   vector<double> shellLeft;
   vector<double> shellRight;

   /**
    * The phase pattern of a symmetry orbital, one per terminal atom.
    *
    * What a hand-drawn diagram sketches beside the level: circles at
    * the terminal atoms, filled or open by sign, sized by weight.  It
    * is what makes a TASO a picture instead of a label -- a1 and t2
    * both say "three orbitals" and neither says which combination.
    *
    * From SymmetryAnalysis::projectOrbit(), so it is the projection
    * operator's own answer.  Empty where there is nothing to sketch:
    * a single atom's own orbital, or a shell the projection does not
    * cover.
    */
   vector<double> phases;

   /**
    * Which shell a FRAGMENT level is, as an angular momentum, or -1.
    *
    * A central atom's 2s and 2p, and a diatomic's, are single shells
    * and carry 0 and 1.  A set of symmetry orbitals built from one
    * shell carries that shell's value too.
    */
   int            shell;

   /**
    * Which shell of THIS COLUMN the level belongs to, or -1.
    *
    * An index into MoColumn::shellKeys, which names each shell by its
    * element as well as its angular momentum.
    *
    * `shell` alone is the angular momentum, and binning the
    * composition by it lumps every s function in a fragment
    * together.  In formaldehyde the terminal fragment is an oxygen
    * and two hydrogens, so oxygen's 2s and the hydrogens' 1s shared
    * one bin, got identical weights, and were dragged towards each
    * other: oxygen's 2s/2p separation came out a third of the free
    * atom's while carbon's stayed full size.  That is not the price
    * of placing a level at the mean of what it became -- it is two
    * different shells being asked one question.
    */
   int            slot;

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
               shareLeft(-1.0), shareRight(-1.0), shell(-1), slot(-1),
               character(UNKNOWN), pairing(-1) {}
};


/** One column of the diagram. */
struct MoColumn
{
   string           title;

   /**
    * This column's shells, named by element and angular momentum.
    *
    * "O:0" is oxygen's s shell.  MoLevel::slot indexes this, and the
    * composition shares are binned the same way, so a fragment level
    * is weighted by how much of an orbital sits in ITS shell rather
    * than in any shell of the same angular momentum.
    */
   vector<string>   shellKeys;

   /** Where to draw this column's atoms, for the phase sketches. */
   vector<double>   sketchX;
   vector<double>   sketchY;

   /**
    * Which Cartesian axis was flattened away making those positions.
    *
    * A p orbital points somewhere, so a sketch of a p symmetry orbital
    * has to say where.  The two axes the atoms spread out in are the
    * page; the third is perpendicular to it, and a p component along
    * that one is the pi case -- drawn as a circle, filled or open,
    * exactly as an s combination is.  The other two are in the page
    * and want an arrow.
    */
   int              sketchNormal;
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

   MoColumn() : hiddenCount(0), hiddenMaxEnergy(0.0),
               hiddenAboveCount(0), sketchNormal(2) {}
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
    /**
     * How many orbitals an irrep holds, read from its Mulliken letter.
     *
     * Used where the character table cannot supply it -- a linear
     * molecule has no finite table.
     */
    static int dimensionFromName(const string& canonical);

    /**
     * The molecule's formula, in Hill order, with its charge.
     *
     * Written as the canvas wants it: digits are subscripts where they
     * stand, and anything after a '^' is a superscript.  "CCl4",
     * "H2O", "NO2^-", "SO4^2-".
     *
     * The charge is the point of it.  A diagram of nitrite and a
     * diagram of nitrogen dioxide differ by one electron and look
     * nearly the same, and the formula is where a reader checks which
     * one they are holding.
     */
    static string formula(const vector<string>& elements, int charge);

    static bool groupByIrrep(const vector<double>& energies,
                             const vector<double>& occupancies,
                             const vector<string>& labels,
                             const map<string,int>& dimensions,
                             double tolerance,
                             vector<MoLevel>& levels);

    /**
     * Place fragment levels at the mean of the orbitals they became.
     *
     * NOT at their tabulated ionisation energies mapped onto the
     * molecular axis.  Those are a free atom's, measured by somebody
     * else, and where they land has no relation to the orbitals the
     * calculation produced: nitrogen's 2s came out above the sigma-u
     * built from it, which cannot happen -- an antibonding combination
     * is above both its parents by definition, as a bonding one is
     * below them.
     *
     * A fragment orbital is spread over the molecular orbitals it
     * combined into, so its energy is their mean weighted by how much
     * of each it makes up.  That is the fragment-orbital energy of a
     * fragment analysis, it comes from the calculation rather than from
     * a table, and it lies inside their range by construction: the
     * bonding partners below and the antibonding ones above, not by
     * arrangement but because that is what the words mean.
     *
     * The tabulated energy is kept as the annotation, where it belongs.
     *
     * @param connections  from connect(), which must therefore be
     *                     called first
     */
    static void placeFragments(const MoColumn& centre,
                               MoColumn& left, MoColumn& right,
                               const vector<MoConnection>& connections);

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

    /**
     * Connect fragment levels to molecular orbitals.
     *
     * Symmetry decides what MAY connect: orbitals of different irreps
     * cannot mix, and orbitals of the same one must be allowed to --
     * water's oxygen 2s and 2pz are both a1 and both mix into every a1
     * orbital, which is the point of the diagram and cannot be drawn
     * by matching each orbital to one partner.
     *
     * COMPOSITION DECIDES WHAT ACTUALLY DOES.  In any molecule past
     * the smallest, symmetry alone permits far more than happens: in a
     * large molecule an orbital has a formal contribution from atoms
     * halfway across it, and drawing all of them buries the two or
     * three interactions the diagram is about.  Where a level carries
     * a composition (MoLevel::shareLeft/shareRight), a fragment it
     * holds less than @p cutoff of gets no line.
     *
     * Where it carries none -- a calculation whose coefficients or
     * basis were not available -- symmetry is used on its own, which
     * is a busier diagram and not a wrong one.
     *
     * @param cutoff  the least share worth drawing, 0 to 1
     */
    /**
     * Mark levels bonding or antibonding from their energies alone.
     *
     * For a calculation that reports no symmetry labels, where the
     * counting rule in classify() has nothing to count.  An orbital
     * below every fragment level it is built from is bonding, one
     * above them all is antibonding, one among them is neither -- which
     * is what the words mean, and needs only the connections.
     *
     * Runs after placeFragments(), so "below its parents" is read
     * against where they actually sit, and leaves alone anything
     * classify() has already decided.
     */
    static void classifyByEnergy(const vector<MoLevel>& left,
                                 vector<MoLevel>& centre,
                                 const vector<MoLevel>& right,
                                 const vector<MoConnection>& connections);

    static void connect(const vector<MoLevel>& left,
                        const vector<MoLevel>& centre,
                        const vector<MoLevel>& right,
                        vector<MoConnection>& connections,
                        double cutoff = 0.05);
};

#endif
