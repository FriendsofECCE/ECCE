/**
 * @file
 *
 * The model-first MO correlation diagram (#301).
 *
 * The whole picture is extended Hueckel: the left and right fragments
 * (their own EHT orbitals) at the sides, the molecule's own EHT orbitals
 * ("Model MOs") in the middle, and a line wherever a model MO has at
 * least LINK_MIN of a fragment level in it.  The basis is the same for
 * all three, so the projection of a model MO on the fragment orbitals
 * is exact and the lines carry no fitting.
 *
 * Bonding and antibonding come from the sign of the inter-fragment
 * overlap population of each model MO and are then paired per irrep and
 * degeneracy, so every antibonding level has its bonding partner; a
 * level left over is non-bonding.
 *
 * The real calculation is a column of its own, on its own energy axis.  For each model
 * level the calculated MO (or near-degenerate set) with the largest
 * overlap is found with the mixed STO/Gaussian overlaps and its energy
 * and the overlap value are shown.  The verdict says whether the model
 * order of the valence levels is the calculation's.
 *
 * Energies of the model are Hartree in MoLevel::energy and drawn in eV;
 * the calculation's are text in Hartree.  No wx.
 */

#ifndef MODELDIAGRAM_HH
#define MODELDIAGRAM_HH

#include <string>
#include <vector>

#include "tdat/EspField.H"
#include "tdat/MoDiagram.H"

using std::string;
using std::vector;

class ModelDiagram
{
  public:

    /** The smallest share of a fragment level that gets a line. */
    static constexpr double LINK_MIN = 0.03;
    /** A level this much on the right fragment is ligand-internal (band). */
    static constexpr double INTERNAL_SHARE = 0.80;
    /** A pair needs at least this much of each fragment in both levels. */
    static constexpr double PAIR_FLOOR = 0.05;
    /** Empty levels more than this (eV) above the model HOMO are not drawn:
        extended Hueckel puts sigma* orbitals tens of eV up. */
    static constexpr double WINDOW_EV = 30.0;
    /** Ligand-field view: an empty level above the d levels with at least this
        share on the ligands is ligand-centred (the model's t2g* of Cr(CO)6 is 61 %). */
    static constexpr double LF_LIGAND_SHARE = 0.50;
    /** A fragment character is named in a level's tag from this share up. */
    static constexpr double CHARACTER_MIN = 0.20;
    /** An overlap population this small (electrons) is non-bonding. */
    static constexpr double OP_TOLERANCE = 0.02;
    /** Fragment-orbital make-up two partner levels must share (sum of the smaller shares). */
    static constexpr double PAIR_SIMILAR = 0.5;
    /** Model levels this close (Hartree) are one degenerate level. */
    static constexpr double DEGENERATE = 2.0e-4;
    /** Calculated energies this close (Hartree) are not an inversion. */
    static constexpr double SAME_ENERGY = 1.0e-3;
    /** An overlap below this is a poor match. */
    static constexpr double POOR_OVERLAP = 0.80;

    struct Input
    {
      vector<string> symbols;
      vector<double> coordsAng;       // the frame the parent basis is in
      int  charge, multiplicity;
      vector<int> leftAtoms, rightAtoms;   // together every atom
      int  leftCharge, rightCharge;        // sum = charge
      bool leftMetal;                      // ionic d count, not neutral
      bool showAll;                        // draw the ligand-internal levels too
      bool ligandFieldView;                // fold empty ligand-centred levels above the d levels
      bool piOnly;                         // planar molecule: only the levels that change sign in the plane
      Input() : charge(0), multiplicity(1), leftCharge(0), rightCharge(0),
                leftMetal(false), showAll(false), ligandFieldView(false), piOnly(false) {}
    };

    struct Result
    {
      MoColumn left, right, centre;
      vector<MoConnection> links;
      string summary;                  // tier A, one line
      vector<string> warnings;         // tier B
      string about;                    // tier C
      string verdict;                  // the model-against-calculation line
      bool   verdictOk;                // false: drawn orange
      int    valenceLevels, inversions;
      double overlapMin, overlapMax;
      bool   haveDeltaO;
      bool   lfApplicable;             // a complex: the Ligand-field view means something
      bool   foldedAny;                // ligand-internal levels were folded into bars
      double deltaCalcEv;
      Result() : verdictOk(true), valenceLevels(0), inversions(0),
                 overlapMin(0), overlapMax(0), haveDeltaO(false), lfApplicable(false), foldedAny(false),
                 deltaCalcEv(0) {}
    };

    /**
     * The construction label of one model level (#458): sigma/pi/delta, the
     * parent orbitals and "*" for the out-of-phase combination; "nb" when the
     * other fragment has under NONBONDING_SHARE of it.  Empty where no bond
     * axis exists (two polyatomic fragments).
     */
    struct ConstructLabel
    {
      vector<int> orbitals;     // model orbitals of the level, energy order
      double energy;            // Hartree
      string full;              // "sigma*2p", "sigma(C 2s/O 2s)", "sigma*(B 2p)", "nb (F 2p)"
      string plain;             // the same without the parents: "sigma*", "nb"
      bool   nonbonding, antibonding;
      double otherShare;        // the smaller fragment's share of the level
      double sigmaShare;        // central-atom levels: share of the ligand density with no nodal plane containing the bond; -1 if not given
      string split;             // hover text of that split
      int    nodal;             // nodal planes containing the bond axis: 0 sigma, 1 pi, 2 delta; -1 if not given
      ConstructLabel() : energy(0), nonbonding(false), antibonding(false), otherShare(0), sigmaShare(-1), nodal(-1) {}
    };

    /** A level with less than this on one fragment is non-bonding by construction. */
    static constexpr double NONBONDING_SHARE = 0.05;

    /**
     * The construction labels of the model's levels in energy order, from the
     * extended-Hueckel model alone (no calculation).  Diatomics are named by
     * correlation with the same model run without s-p coupling, so the order
     * of N2's 2sigma_u and 3sigma_g levels does not depend on a composition
     * threshold.
     */
    static bool constructLabels(const Input& in, vector<ConstructLabel>& out, string& why);

    /** A level's energy slope with the bond length (#458). */
    struct BondSlope
    {
      vector<int> orbitals;     // model orbitals of the level, energy order of the unstretched model
      double slope;             // d(energy)/dR in eV per Angstrom
      bool   ok;                // matched in both stretched models
      BondSlope() : slope(0), ok(false) {}
    };

    /** The energy change of a model level with |dE/dR| below this (eV/Angstrom) is "little effect". */
    static constexpr double SLOPE_LITTLE = 0.5;

    /**
     * Central difference of each model level's energy over the bond(s) +-0.02 A:
     * the internuclear distance of a diatomic, every centre-ligand bond together
     * for a central atom with ligands (breathing), otherwise the distance between
     * the fragments with both rigid.  Levels are matched across the three runs by
     * maximum overlap among levels of the same degeneracy.  Rising on stretching
     * means bonding.
     */
    static bool bondSlopes(const Input& in, vector<BondSlope>& out, string& why);

    /** Can extended Hueckel treat every element?  `why` says what not. */
    static bool applicable(const vector<string>& symbols, string& why);

    /**
     * @param parentBasis, S, moRows, moEnergies  the calculation, in the
     *            frame of `in.coordsAng` (rows indexed like ORBENG)
     * @param whole  the calculation's levels (labels, irreps, orbital
     *            indices), before any cut-off
     */
    static bool build(const Input& in,
                      const vector<EspBasisFunction>& parentBasis,
                      const vector<double>& S,
                      const vector< vector<double> >& moRows,
                      const vector<double>& moEnergies,
                      const vector<MoLevel>& whole,
                      Result& out, string& why);
};

#endif
