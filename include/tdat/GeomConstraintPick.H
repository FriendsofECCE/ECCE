#ifndef GEOMCONSTRAINTPICK_HH
#define GEOMCONSTRAINTPICK_HH

#include <string>
#include <vector>

class Fragment;

/**
 * Interprets the atoms a user picked for a bond, angle or torsion constraint.
 *
 * Students pick the atoms in any order; what matters is the bonding.  An
 * angle's centre is the atom bonded to both others and a torsion is a bonded
 * chain, so the picked atoms are reordered when that is unambiguous.  When
 * they are not bonded as required (always the case for an unbonded dummy
 * atom) the pick order is kept and `needsConfirm` is set, so the caller asks
 * before adding.  Nothing here is blocked: the user knows best.
 */
struct GeomConstraintPick
{
  GeomConstraintPick() : reordered(false), needsConfirm(false) { }

  std::vector<int> atoms;     ///< atom indices in the order to constrain
  std::string description;    ///< e.g. "Angle H2-N1-H3 (centre N1)"
  std::string warning;        ///< why confirmation is needed; empty if none
  bool reordered;             ///< atoms differ from the pick order
  bool needsConfirm;          ///< not bonded as required: ask first
};

/** @param picked 2, 3 or 4 atom indices in pick order. */
GeomConstraintPick orderConstraintAtoms(const Fragment& frag,
                                        const std::vector<int>& picked);

/** "N1": the symbol and the 1-based index, as ECCE shows atoms. */
std::string constraintAtomName(const Fragment& frag, int index);

#endif
