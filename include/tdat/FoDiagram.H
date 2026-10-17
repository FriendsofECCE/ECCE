/**
 * @file
 *
 * The fragment-orbital correlation diagram (#301, route B): the metal's
 * valence AOs on the left, the ligand shell's extended Hueckel orbitals
 * on the right, the parent calculation's MOs in the middle at their own
 * energies, and a line from each MO to the fragment orbitals (FOs) that
 * make it up.
 *
 * WHERE A FRAGMENT LEVEL SITS.  At the diagonal element of the parent's
 * Fock operator in the Loewdin-orthogonalised FO basis,
 * e_k = <FO~_k|F|FO~_k>, with F = S C eps C^T S known on the span of the
 * reported MOs.  For any orthonormal basis the eigenvalues of a block
 * bracket the diagonal elements it mixes, so a bonding MO lies below
 * and an antibonding one above the FO levels it came from.  A fitted
 * scale of the extended Hueckel energies cannot promise that.
 * <FO~_k|F|FO~_k> = sum_m eps_m |<FO~_k|psi_m>|^2 (psi_m the MOs), the
 * same F that MoLigandField::buildFock() writes out, without forming it.
 *
 * SYMMETRY.  Levels are labelled in the point group of the coordination
 * skeleton (the metal and its donor atoms), because ligands with their
 * own atoms (NH3) break the molecule's group but not the skeleton's.
 * A metal d set therefore splits into its eg and t2g parts.
 *
 * No wx.  MoDiagramPanel only gathers the inputs and draws the result.
 */

#ifndef FODIAGRAM_HH
#define FODIAGRAM_HH

#include <string>
#include <utility>
#include <vector>

#include "tdat/EspField.H"
#include "tdat/FragmentOrbitals.H"
#include "tdat/MoDiagram.H"
#include "tdat/MoPairs.H"

using std::string;
using std::vector;

class FoDiagram
{
  public:

    /** The smallest share that gets a line. */
    static constexpr double LINK_MIN = 0.10;
    /** The smallest metal share an MO needs to belong to a pair. */
    static constexpr double PAIR_FLOOR = 0.05;
    /** A fragment orbital described to less than this is marked. */
    static constexpr double CAPTURE_OK = 0.90;
    /** Non-metal centre: a local label is shown plain from this fit up,
        with "~" from FIT_LABEL_MIN, and not at all below that. */
    static constexpr double FIT_LABEL_OK = 0.90;
    static constexpr double FIT_LABEL_MIN = 0.50;

    /** One fragment level: a set of FOs of one irrep. */
    struct SetInfo
    {
      string label, irrep;
      bool   metal;
      double eFock;        // Hartree, <FO~|F|FO~> averaged over the set
      double eEht;         // Hartree, extended Hueckel (qualitative)
      double capture;      // mean norm of the FOs' projection on the MOs
      int    degeneracy;
      int    column;       // index in left/right, -1 if not drawn
      SetInfo() : metal(false), eFock(0), eEht(0), capture(0),
                  degeneracy(0), column(-1) {}
    };

    /** What the diagram knows of one drawn MO level. */
    struct MoInfo
    {
      string irrep;                          // skeleton irrep, or ""
      int    metalSet, ligandSet;            // indices into sets, or -1
      vector< std::pair<int,double> > top;   // (set, share), largest first
      double fit;      // non-metal centre: how well it fits its local label, else 1
      MoInfo() : metalSet(-1), ligandSet(-1), fit(1.0) {}
    };

    /** A bonding/antibonding pair and the two sets it comes from. */
    struct PairInfo
    {
      int bonding, antibonding;     // indices into the drawn centre levels, -1 if undrawn
      int metalSet, ligandSet;      // indices into sets
      string irrep;
      PairInfo() : bonding(-1), antibonding(-1), metalSet(-1), ligandSet(-1) {}
    };

    /** The pair count and the pair rule are shared (MoPairs.H). */
    typedef MoPairs::PairLevel PairLevel;
    typedef MoPairs::PairCount PairCount;
    static vector<PairCount> countPairs(const vector<PairLevel>& levels)
    { return MoPairs::countPairs(levels); }

    /**
     * The electron bookkeeping (ionic convention): the metal's d count
     * n = group number - oxidation state, the ligand fragment orbitals'
     * electrons, and the electrons in the valence MOs (capture >= 50%).
     */
    struct ElectronCount
    {
      int    oxidation, dCount, ligands, unpaired;
      double moElectrons;
      bool   placed;      // the d electrons are drawn
      bool   agree;       // metal + ligands = valence MO electrons
      string why;         // when not placed
      string line;        // the note's line
      string metal;       // symbol of the metal (or centre atom), "" when none
      bool   centreAtom;  // a non-metal centre: neutral pieces, s and p drawn
      ElectronCount() : oxidation(0), dCount(0), ligands(0), unpaired(0),
                        moElectrons(0), placed(false), agree(false),
                        centreAtom(false) {}
    };

    struct Result
    {
      MoColumn left, right;                 // metal sets, ligand sets
      vector<MoConnection> links;           // weight = the set's share
      string note;                          // validity readout (the full legacy text)
      //  The three tiers of the text under the diagram (UTF-8, with
      //  <sub>..</sub> in the point group): `summary` is always shown,
      //  `warnings` only when something is wrong, `about` is the pop-up.
      string summary;
      vector<string> warnings;
      string about;
      vector<SetInfo> sets;                 // every fragment set
      vector<MoInfo> mo;                    // one per drawn centre level
      vector<PairInfo> pairInfo;            // every pair found
      vector<PairCount> pairs;              // per skeleton irrep
      vector<string> pairMessages;          // the mismatches, one line each
      string group;                         // skeleton point group, or ""
      bool   centreAtom;                    // built round a non-metal atom
      string pieces;                        // its pieces, "OH, H, H, H"
      ElectronCount electrons;
      double captureMin, captureMax;        // over the drawn MOs
      Result() : centreAtom(false), captureMin(0), captureMax(0) {}
    };

    /** "Also not shown: 18 core orbitals below -3.49 Hartree.", or "". */
    static string coreSentence(const MoColumn& centre);

    /** A point group as a chemist writes it, "O<sub>h</sub>"; "" for "". */
    static string groupMarkup(const string& group);

    /**
     * The heading formula of a complex, ligands grouped: Cr(CO)6,
     * [Co(NH3)6]^3+, [TiF6]^2-, PtCl2(NH3)2.  Metals first, then the
     * ligand kinds alphabetically; within a ligand the donor atom (the
     * one nearest the metal) first, the rest in Hill order, water as H2O;
     * a monatomic ligand is its bare symbol.  The charge is written
     * "^3+" as MoDiagram::formula() writes it, and a charged complex goes
     * in square brackets.  Empty when the structure is not a complex.
     */
    static string complexFormula(const vector<string>& symbols,
                                 const vector<double>& coordsAng, int charge);

    /**
     * Is this a complex the construction can describe?  Declines with
     * `why` for no metal, no ligand, or an element without parameters.
     */
    static bool applicable(const vector<string>& symbols,
                           const vector<double>& coordsAng, int charge,
                           string& why, int centre = -1);

    /**
     * @param symbols, coordsAng  the parent's atoms in the frame its
     *                            basis functions and MO rows are in
     * @param parentBasis, S, moRows  as FragmentOrbitals::compose();
     *                            ALL the MO rows (the Fock operator is
     *                            known on their span)
     * @param moEnergies          one per MO row, Hartree
     * @param centre              the drawn molecular levels (after the
     *                            panel's cut-offs).  Replaced by the
     *                            levels this diagram draws.
     * @param showAll             draw the ligand-internal and
     *                            low-metal-share levels as well
     * @param whole               every level before the cut-offs, so
     *                            the antibonding partner of a drawn
     *                            bonding level is found and drawn even
     *                            past the cut-off; null = `centre`
     * @param multiplicity        of the molecule: multiplicity - 1 of the
     *                            metal d electrons are drawn unpaired
     * @param pickAtom            >= 0: the atom a single non-metal pick
     *                            names, in the metal's place (#301).  The
     *                            pieces are neutral, its s and p electrons
     *                            are drawn by aufbau and Hund, and each
     *                            orbital's label is those of the local
     *                            skeleton with its fit to it.
     */
    static bool build(const vector<string>& symbols,
                      const vector<double>& coordsAng, int charge,
                      const vector<EspBasisFunction>& parentBasis,
                      const vector<double>& S,
                      const vector< vector<double> >& moRows,
                      const vector<double>& moEnergies,
                      MoColumn& centre, Result& out, string& why,
                      bool showAll = false,
                      const vector<MoLevel>* whole = 0,
                      int multiplicity = 1, int pickAtom = -1);
};

#endif
