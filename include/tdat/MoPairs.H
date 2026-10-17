/**
 * @file
 *
 * One pair rule for every MO diagram construction (#301).
 *
 * Per symmetry label, n fragment orbitals give n MOs.  A pair is a
 * bonding level below both parents and its antibonding partner (*) above
 * both; every drawn level without a partner is non-bonding (nb), its
 * mixing shown in the hover composition only.  There is no third class.
 *
 * pairByParents() finds the pairs of a fragment-orbital diagram, where
 * the parents and every level's share of them are known; settle() applies
 * the rule to the other constructions' levels, after their own
 * classification; countPairs() is the readout both use.  No wx.
 */

#ifndef MOPAIRS_HH
#define MOPAIRS_HH

#include <string>
#include <utility>
#include <vector>

#include "tdat/MoDiagram.H"

using std::string;
using std::vector;

class MoPairs
{
  public:
    /** One level in the pair count. */
    struct PairLevel
    {
      string irrep;
      int    degeneracy;
      MoLevel::Character character;
      int    shown;        // 1 drawn, 0 not drawn: cut-off, -1 not drawn: filtered
      PairLevel() : degeneracy(1), character(MoLevel::UNKNOWN), shown(1) {}
    };

    struct PairCount
    {
      string irrep;
      int bonding, antibonding, nonbonding;   // drawn MOs, by degeneracy
      int hiddenByCutoff, hiddenByFilter;     // undrawn bonding/antibonding
      bool ok;
      string message;                         // empty when ok
      PairCount() : bonding(0), antibonding(0), nonbonding(0),
                    hiddenByCutoff(0), hiddenByFilter(0), ok(true) {}
    };

    /**
     * Orbitals are conserved: per irrep, drawn bonding MOs equal drawn
     * antibonding MOs.  A mismatch is a message, not an error; MOs
     * left undrawn count as missing partners and the message says why.
     */
    static vector<PairCount> countPairs(const vector<PairLevel>& levels);

    /** A fragment set a level can be made of. */
    struct Parent
    {
      string irrep;
      double energy;       // Hartree
      bool   centre;       // the centre atom's (metal's) side
      Parent() : energy(0.0), centre(false) {}
    };

    /** An MO level, with its share of every parent. */
    struct Level
    {
      string irrep;
      int    degeneracy;
      double energy;
      bool   valence;
      vector<double> share;     // per parent
      Level() : degeneracy(1), energy(0.0), valence(false) {}
    };

    struct Pair
    {
      size_t bonding, antibonding;      // indices into the levels
      int centreParent, ligandParent;   // indices into the parents
    };

    /**
     * The pairs: two levels of one irrep and degeneracy that mix the same
     * centre parent, the lower below that parent's energy and the higher
     * above it, and share a ligand parent of that irrep (at least
     * pairFloor in both, linkMin in one of them).  Taken best first, so a
     * ligand-internal level with a little centre in it does not displace
     * the pair that carries the interaction.
     */
    static vector<Pair> pairByParents(const vector<Parent>& parents,
                                      const vector<Level>& levels,
                                      bool haveSym, double pairFloor,
                                      double linkMin);

    /**
     * Apply the rule to levels a construction has classified: MIXED
     * becomes non-bonding; a bonding or antibonding level whose partner
     * is not there (same pairing number, other character, same
     * degeneracy, the bonding one lower) becomes non-bonding; labels are
     * rewritten to match (`*`, `nb`).  The messages (one per irrep) say
     * which levels were left without a partner.
     */
    static void settle(vector<MoLevel>& levels, vector<string>* messages = 0);
};

#endif
