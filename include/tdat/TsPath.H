/**
 * @file
 *
 * Geometric sanity check of the straight path between the reactant and the
 * product of a transition-state search (#468).  A path that keeps a
 * molecule linear, or passes one atom through another, gives NEB/QST no
 * force to bend it.  No wx here.
 *
 * Structures are flat x,y,z in Angstrom, in the reactant's atom order (the
 * order the AtomMap renumbering produces), already aligned.
 */
#ifndef TSPATH_HH
#define TSPATH_HH

#include <string>
#include <vector>

struct TsPathReport
{
   bool collinear;          // every supplied structure lies on one line
   double closest;          // smallest interatomic distance along the path (A)
   int atomA, atomB;        // the pair that gets that close, -1 when none
   bool passes;             // closest < TsPath::minDistance
   TsPathReport() : collinear(false), closest(1e9), atomA(-1), atomB(-1),
                    passes(false) {}
};

class TsPath
{
 public:
   /** Two atoms closer than this (A) on the path count as one passing through
    *  the other; no bond is that short. */
   static const double minDistance;
   /** A structure is linear when no atom is further than this (A) from the
    *  line through its two most distant atoms. */
   static const double linearTolerance;

   static bool isLinear(const std::vector<double>& xyz);

   /** Linearity of the end points (and of the guess, when given), and the
    *  closest approach along the straight reactant -> product path.  With a
    *  guess the path is the guess's own and only linearity is judged. */
   static TsPathReport check(const std::vector<double>& reactant,
                             const std::vector<double>& product,
                             const std::vector<double>* guess = 0);

   /** The user-facing sentences, empty when the path is fine.  elements and
    *  the report index the same atoms. */
   static std::string warning(const TsPathReport& r,
                              const std::vector<std::string>& elements);
};

#endif
