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

/** Where a dummy atom goes relative to the picked atoms (#306). */
enum DummyPlacement
{
  DUMMY_CENTRE,    ///< at the centroid of all picked atoms
  DUMMY_BEYOND,    ///< on the axis through the first atom, past it
  DUMMY_BETWEEN    ///< on that axis, between the first atom and the rest
};

/**
 * Position for a dummy atom on a saturated centre, which has no open site.
 * The axis runs from C, the centroid of the atoms after the first, through
 * the first atom A; the point lies `distance` from A, on the far side of A
 * from C (DUMMY_BEYOND) or towards C (DUMMY_BETWEEN).
 * @param xyz the picked atoms' coordinates in pick order (at least two)
 * @param out receives x, y, z
 * @return empty on success, else why no point is defined
 */
std::string dummyAtomPosition(const std::vector<std::vector<double> >& xyz,
                              DummyPlacement how, double distance,
                              double out[3]);

#endif
