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

#include <functional>
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
     * This is compared against a LOWDIN share (see lowdinShares()
     * below), not a Mulliken one (#170). A Mulliken per-atom share is
     * not bounded to [0,1] -- an antibonding virtual can put well over
     * 1.0 on one atom and a compensating negative share on the other
     * (confirmed against ORCA's own Mulliken printout for CO's 6-sigma*)
     * -- so it can cross a "mostly on one atom" threshold for an orbital
     * that plainly is not localised. Lowdin shares are bounded and sum
     * to 1 by construction.
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

    /**
     * A generic symmetric eigensolver (cyclic Jacobi): rotate away the
     * largest off-diagonal element, repeat. Moved here from Huckel.C,
     * which still uses it to build S^-1/2 for its own orthogonalisation
     * -- this class needs the same routine to build S^+1/2 for
     * lowdinShares() below, so it lives where both can call it rather
     * than existing twice.
     *
     * @param a       nxn symmetric matrix; destroyed by the rotation
     * @param values  the n eigenvalues, unordered
     * @param vectors the n eigenvectors, as columns: vectors[i][k] is
     *                component i of eigenvector k
     * @param onSweep optional progress callback, called once per sweep
     *                with a fraction in [0,1] estimated from how far
     *                the off-diagonal norm has fallen towards the
     *                convergence threshold (log scale, since it falls
     *                geometrically) -- purely cosmetic, never affects
     *                the result
     * @return true (kept bool for interface stability; the sweep limit
     *         is generous enough for anything this size that a genuine
     *         non-convergence has not been observed)
     */
    static bool jacobiEigen(vector< vector<double> >& a,
                            vector<double>& values,
                            vector< vector<double> >& vectors,
                            std::function<void(double)> onSweep = nullptr);

    /**
     * S^{1/2}, the symmetric square root of the overlap matrix, built
     * once per diagram and reused for every MO's lowdinShares() below
     * -- Sflat itself does not change per orbital, only c does.
     *
     * @param overlap    nbasis*nbasis, row major (Sflat[i*n+j])
     * @param nbasis     the basis size overlap.size() should equal n*n of
     * @param sqrtOverlap out: nbasis*nbasis, row major
     * @param onSweep optional progress callback, forwarded to jacobiEigen()
     * @return false if overlap does not match nbasis, or if any
     *         eigenvalue of S is <= ~1e-8 (linear dependence -- S^1/2
     *         itself is fine there, but a caller relying on this being
     *         well-conditioned should not trust the result)
     */
    static bool buildSqrtOverlap(const vector<double>& overlap, int nbasis,
                                 vector<double>& sqrtOverlap,
                                 std::function<void(double)> onSweep = nullptr);

    /**
     * One MO's Lowdin (symmetrically-orthogonalised) share per atom:
     * c' = S^{1/2} c, then share_A = sum_{mu on A} c'_mu^2 / sum_mu c'_mu^2.
     * Bounded to [0,1] and sums to 1, unlike a Mulliken share -- see
     * LOCALISED_SHARE_THRESHOLD's comment above for why that matters.
     *
     * @param coefficients one per basis function, this orbital's row
     * @param functionsPerAtom basis functions per atom; must sum to
     *                         coefficients.size()
     * @param sqrtOverlap  nbasis*nbasis, row major, from
     *                     buildSqrtOverlap() above
     * @return one share per atom, or empty if the arguments do not
     *         line up
     */
    static vector<double> lowdinShares(const vector<double>& coefficients,
                                       const vector<int>& functionsPerAtom,
                                       const vector<double>& sqrtOverlap);

    /**
     * c' = S^{1/2} c -- the whole transformed row, not just its
     * per-atom sum.
     *
     * lowdinShares() above applies this internally and discards it;
     * exposed so a caller that needs the full row -- MoFragments::
     * composeLevels()'s per-atom AND per-shell fragment/shell shares,
     * #175 -- can transform each orbital ONCE and hand the result to
     * code that otherwise reads plain c^2 as a Lowdin population (the
     * convention MoFragments::share()/MoComposition::compute() already
     * use whenever no overlap matrix is given): plain c'^2 of the
     * transformed row IS the Lowdin share, so nothing downstream of
     * this needs to change.
     *
     * @param coefficients one per basis function, this orbital's row
     * @param sqrtOverlap  nbasis*nbasis, row major, from
     *                     buildSqrtOverlap() above
     * @return c', or empty if the sizes do not line up
     */
    static vector<double> transform(const vector<double>& coefficients,
                                    const vector<double>& sqrtOverlap);
};

#endif
