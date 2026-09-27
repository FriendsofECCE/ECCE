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
#include <vector>
using std::string;
using std::vector;


class MoComposition
{
  public:

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
};

#endif
