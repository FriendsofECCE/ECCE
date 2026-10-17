/**
 * @file
 *
 * The fragment-orbital correlation diagram (#301, route B stage 2): the
 * metal's valence AOs on the left, the ligand shell's extended Hueckel
 * orbitals on the right, the parent calculation's MOs in the middle at
 * their own energies, and a line from each MO to every fragment orbital
 * (FO) that makes up at least LINK_MIN of it.
 *
 * WHY A FIT.  An MO's energy is the calculation's; an FO's is extended
 * Hueckel, a different scale.  Placing each FO at the mean energy of
 * the MOs it became would put the 3d and the ligand orbitals wherever
 * the mixing pushed them, which is the opposite of what the picture is
 * for.  So all FOs share ONE linear map (least squares of the MO-mean
 * energy against the EHT energy), which keeps the metal and ligand
 * levels in their EHT order and spacing relative to each other.
 *
 * No wx.  MoDiagramPanel only gathers the inputs and draws the result.
 */

#ifndef FODIAGRAM_HH
#define FODIAGRAM_HH

#include <string>
#include <vector>

#include "tdat/EspField.H"
#include "tdat/FragmentOrbitals.H"
#include "tdat/MoDiagram.H"

using std::string;
using std::vector;

class FoDiagram
{
  public:

    /** The smallest FO share that gets a line. */
    static constexpr double LINK_MIN = 0.10;

    struct Result
    {
      MoColumn left, right;                 // metal AOs, ligand FOs
      vector<MoConnection> links;           // weight = the FO's share
      string note;                          // validity readout
      double slope, intercept;              // E_axis = slope*E_EHT + intercept
      double captureMin, captureMax;        // over the drawn MOs
      Result() : slope(0), intercept(0), captureMin(0), captureMax(0) {}
    };

    /**
     * Is this a complex the construction can describe?  Declines with
     * `why` for no metal, no ligand, or an element without parameters.
     */
    static bool applicable(const vector<string>& symbols,
                           const vector<double>& coordsAng, int charge,
                           string& why);

    /**
     * @param symbols, coordsAng  the parent's atoms in the frame its
     *                            basis functions and MO rows are in
     * @param parentBasis, S, moRows  as FragmentOrbitals::compose()
     * @param moEnergies          one per MO row, Hartree
     * @param centre              the drawn molecular levels.  Each
     *                            gets shareLeft/shareRight and the
     *                            detail text for its tooltip.
     */
    static bool build(const vector<string>& symbols,
                      const vector<double>& coordsAng, int charge,
                      const vector<EspBasisFunction>& parentBasis,
                      const vector<double>& S,
                      const vector< vector<double> >& moRows,
                      const vector<double>& moEnergies,
                      MoColumn& centre, Result& out, string& why);
};

#endif
