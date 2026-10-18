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
    static constexpr double LINK_MIN = 0.10;
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
      Input() : charge(0), multiplicity(1), leftCharge(0), rightCharge(0),
                leftMetal(false), showAll(false), ligandFieldView(false) {}
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
      double deltaModelEv, deltaCalcEv;
      Result() : verdictOk(true), valenceLevels(0), inversions(0),
                 overlapMin(0), overlapMax(0), haveDeltaO(false), lfApplicable(false), foldedAny(false),
                 deltaModelEv(0), deltaCalcEv(0) {}
    };

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
