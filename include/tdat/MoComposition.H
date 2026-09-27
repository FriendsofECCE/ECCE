/**
 * @file
 *
 * The full-resolution composition of one molecular orbital: how much
 * of it sits on each atom, broken down further by shell (s, p, d, ...).
 *
 * This is the SAME population analysis MoFragments::share() computes
 * for a whole fragment -- a Mulliken population when an overlap matrix
 * is given, a Lowdin one (plain sum of squares) when it is not -- kept
 * to full atom-by-shell resolution instead of being summed over a
 * fragment's atoms first.  MoFragments::composeLevels() (the MO
 * correlation diagram's fragment/shell shares, #132) sums the entries
 * this returns rather than re-deriving them, so a later "show the AO
 * composition of this MO" feature can call the same function and know
 * it agrees with what the diagram drew.
 */

#ifndef MOCOMPOSITION_HH
#define MOCOMPOSITION_HH

#include <string>
#include <utility>
#include <vector>
using std::string;
using std::vector;


class MoComposition
{
  public:

    /**
     * How much of a molecular level's norm sitting on one atom counts
     * as "this is that atom's lone pair", regardless of construction
     * (#132, settled 2026-09-27) -- a level this localised is
     * non-bonding whichever two columns the diagram correlates it
     * against, so this check runs BEFORE any per-construction bonding/
     * antibonding rule and pre-empts it.
     *
     * Started at 0.90 and to be revisited from observed values
     * (ECCE_DEBUG_MOSYM prints each level's own maximum atom share) --
     * water's 1b1 (a pure oxygen lone pair), ammonia's 3a1, CO's 5-sigma
     * and N2's 3-sigma-g are the reference cases it needs to separate
     * correctly from CH4's and benzene's levels, none of which should
     * ever cross it.
     */
    //  A header-only constant (not defined in the .C file) so that a
    //  caller which only needs the THRESHOLD -- MoDiagram::classify(),
    //  which decides whether a level is localised but does no
    //  composition arithmetic of its own -- is not forced to link the
    //  rest of this class, or MoFragments::shareOfIndices() that
    //  compute() itself calls, in order to read one number.
    static constexpr double LOCALISED_SHARE_THRESHOLD = 0.90;

    /** One atom's share of one shell of one molecular orbital. */
    struct Share
    {
      int    atom;      // index into the caller's atom list
      string element;
      int    shell;     // angular momentum (0=s, 1=p, 2=d, ...), or -1
                         // when `shellOf` was not given -- then this is
                         // the atom's WHOLE share, not split by shell
      double share;      // 0 to 1, this (atom, shell)'s fraction of
                         // the orbital's norm

      Share() : atom(-1), shell(-1), share(0.0) {}
    };

    /**
     * The full breakdown of one molecular orbital.
     *
     * @param coefficients     one per basis function, this orbital's row
     * @param functionsPerAtom basis functions per atom, same order as
     *                         `elements`; must sum to coefficients.size()
     *                         or nothing is returned (see
     *                         MoFragments::share()'s header for why that
     *                         is checked rather than trusted)
     * @param shellOf          the angular momentum of each basis
     *                         function, same length as `coefficients`,
     *                         or empty for a plain per-atom breakdown
     * @param elements         one per atom
     * @param overlap          nbas*nbas, row major, or empty for a
     *                         Lowdin (sum-of-squares) population
     *
     * @return one entry per (atom, shell) actually present, ordered by
     *         atom then by first appearance of that shell; empty if
     *         the arguments do not line up
     */
    static vector<Share> compute(const vector<double>& coefficients,
                                 const vector<int>& functionsPerAtom,
                                 const vector<int>& shellOf,
                                 const vector<string>& elements,
                                 const vector<double>& overlap);

    /**
     * The Mulliken overlap population of one molecular orbital, summed
     * over a given set of bonded atom pairs only:
     *
     *     OP = sum_{(A,B) bonded} sum_{mu in A, nu in B} 2 c_mu c_nu S_{mu,nu}
     *
     * Positive means the orbital reinforces those bonds, negative
     * means it works against them -- the question MoDiagram::classify()
     * asks of a level built from a NO-CENTRAL-ATOM construction (two
     * sets of equivalent atoms, or two halves), where counting which
     * irrep sits in which fragment column asks the wrong question: a
     * C-C bonding orbital in benzene's C6|H6 split has no H partner to
     * share its irrep, and counting alone calls it non-bonding even
     * though it plainly is not (#132).
     *
     * `bonds` is normally MoFragments::covalentBonds() over the WHOLE
     * molecule, not just one fragment's own bonds -- the construction
     * splits the ATOMS into two columns, not the BONDS, and a C-C bond
     * is exactly the interaction a C6|H6 diagram is drawn to show.
     *
     * @param coefficients     one per basis function, this orbital's row
     * @param functionsPerAtom one per atom, in the same order the
     *                         coefficients are
     * @param overlap          nbas*nbas, row major -- required; with no
     *                         overlap matrix a bond's cross term cannot
     *                         be formed at all, so an empty one returns 0
     * @param bonds            which atom pairs to sum over
     *
     * @return the overlap population, or 0 if the arguments do not
     *         line up
     */
    static double overlapPopulation(const vector<double>& coefficients,
                                    const vector<int>& functionsPerAtom,
                                    const vector<double>& overlap,
                                    const vector< std::pair<int,int> >& bonds);
};

#endif
