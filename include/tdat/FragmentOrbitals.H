/**
 * @file
 *
 * Fragment orbitals from extended Hueckel, and the composition of a
 * calculation's molecular orbitals in them (#301, MO diagram route B).
 *
 * WHY.  A ligand-field diagram is read as "this MO is mostly metal d,
 * that one is a ligand group orbital with some d mixed in".  The parent
 * calculation's basis cannot say that directly: its functions sit on
 * atoms, not on a metal and a ligand shell.  So each fragment (the
 * metal's valence AOs, each ligand, or the whole ligand shell) gets
 * extended Hueckel orbitals at the parent geometry, and each parent MO is
 * projected on that fragment-orbital (FO) set.  The MO energies stay the
 * parent's; FO energies are qualitative EHT positions only.
 *
 * The EHT part is a sibling of Huckel (which stays as it is for the
 * existing H-Ne diagram path): s, p and d shells, any n, double-zeta d,
 * parameters from data/client/config/EHTParameters (Alvarez 1993, as in
 * YAeHMOP), weighted Wolfsberg-Helmholz.  Slater functions are fitted by
 * SlaterExpansion, and every overlap, fragment-fragment and
 * fragment-parent, goes through EspField::overlapOf so that one
 * integral code keeps S and S_mix consistent.
 *
 * No wx, and no dependency on a calculation: callers pass the parent's
 * basis functions, overlap matrix and MO coefficient rows.
 */

#ifndef FRAGMENTORBITALS_HH
#define FRAGMENTORBITALS_HH

#include <string>
#include <vector>

#include "tdat/EspField.H"

using std::string;
using std::vector;

class FragmentOrbitals
{
  public:

    enum Kind { METAL, LIGAND, SHELL };

    /** A group of atoms treated as one piece (basic form of #252). */
    struct Fragment
    {
      string      name;           // "Cr", "CO", "NH3 (2)", "ligands"
      vector<int> atoms;          // indices into the parent's atoms
      int         charge;
      int         multiplicity;
      Kind        kind;

      Fragment() : charge(0), multiplicity(1), kind(LIGAND) {}
    };

    /** Which ligand description goes with the metal AOs. */
    enum LigandSet { LIGAND_SHELL, PER_LIGAND };

    /** One valence basis function of a fragment atom. */
    struct Function
    {
      int    atom;          // parent atom index
      string element;
      int    n, l;
      string component;     // "s", "x", "z2", ...
      double hii;           // Hartree
      EspBasisFunction fn;  // normalised, STO-nG
    };

    /** One fragment orbital: coefficients over FoSet::funcs. */
    struct FO
    {
      int            fragment;   // index into FoSet::fragments
      int            index;      // 1-based within its fragment, ascending
      double         energy;     // Hartree (EHT)
      double         occupancy;  // aufbau, 0..2
      vector<double> coef;       // over funcs, c.S.c = 1 within fragment
      string         character;  // "O 2p 62%, C 2s 20%"
    };

    struct FoSet
    {
      vector<Fragment> fragments;   // as used for this set
      vector<Function> funcs;
      vector<FO>       fos;
      vector<double>   overlap;     // funcs x funcs, row major
      double           worstFit;    // lowest STO fit quality used
      FoSet() : worstFit(1.0) {}
    };

    //  ---- parameters ----

    /** Load a parameter file (default: $ECCE_HOME/data/client/config). */
    static bool loadParameters(const string& path = string());

    /** Are there parameters for every element?  `missing` lists the rest. */
    static bool canSolve(const vector<string>& elements, string& missing);

    static bool isTransitionMetal(const string& element);

    //  ---- fragments ----

    /**
     * Metal, ligands and the ligand shell.  Metals are the transition
     * metals; ligands are the connected components left when the metals
     * are removed (covalent-radii bonds, as the diagram's own code).
     * A ligand with an odd electron count is -1, otherwise neutral
     * (Cl-, CN-, OH-; CO, NH3, H2O); the metal takes the rest of the
     * total charge.  Returns false, with `why`, when there is no metal
     * ("not applicable"): existing constructions then apply unchanged.
     *
     * Result order: metals, then ligands, then one SHELL fragment.
     */
    static bool autoFragments(const vector<string>& symbols,
                              const vector<double>& coordsAng,
                              int totalCharge, int multiplicity,
                              vector<Fragment>& out, string& why);

    //  ---- orbitals ----

    /**
     * Build the FO set.  METAL fragments contribute their valence AOs
     * as they are (energy H_ii); LIGAND or SHELL fragments (per
     * `ligands`) contribute their EHT orbitals, occupied and empty.
     */
    static bool build(const vector<string>& symbols,
                      const vector<double>& coordsAng,
                      const vector<Fragment>& fragments, LigandSet ligands,
                      FoSet& out, string& why);

    //  ---- composition ----

    /** FO share of one MO, grouped over degenerate FOs of one fragment. */
    struct LevelShare
    {
      int    fragment;
      string label;        // "Cr 3d", "CO FO5 (occ)"
      string character;    // dominant AO content of a ligand FO
      double energy;       // eV (EHT)
      double share;        // fraction of the MO's norm
    };

    /** The composition of one parent MO. */
    struct MoShares
    {
      double capture;      // sum of the shares, <= 1
      double metal;        // share on METAL FOs, fraction of the norm
      double ligand;       // share on LIGAND/SHELL FOs
      string klass;        // "metal", "metal-centred", "ligand", "mixed", "core"
      double norm;         // c.S.c of the input row
      vector<double>     perFo;   // one per FoSet::fos
      vector<double>     perGroup;  // one per group of groupFos()
      vector<LevelShare> top;     // largest first, shares >= TOP_MIN
      MoShares() : capture(0), metal(0), ligand(0), norm(0) {}
    };

    //  Class thresholds, on shares of the captured part: one side
    //  holding at least SIDE_FRACTION of it decides metal or ligand;
    //  a metal share in [CENTRED_FRACTION, SIDE_FRACTION) is
    //  "metal-centred" (a t2g that borrows 30% from pi* is still read
    //  as the metal's); below CORE_CAPTURE the FO set has not described
    //  the MO at all (1s, 2s of a heavy atom are outside a valence EHT
    //  basis).
    static constexpr double SIDE_FRACTION = 0.80;
    static constexpr double CENTRED_FRACTION = 0.60;
    static constexpr double CORE_CAPTURE  = 0.50;
    static constexpr double TOP_MIN       = 0.02;
    static constexpr double DEGENERATE_EH = 1.0e-4;

    /** The class name for a capture and its metal and ligand parts. */
    static string classOf(double capture, double metal, double ligand);

    /**
     * Degenerate FOs read as one level: within a fragment, and across the
     * same-formula ligands of a per-ligand set.  Indices into set.fos.
     */
    static void groupFos(const FoSet& set, vector< vector<int> >& groups);

    /** Label, character, EHT energy (eV) and fragment of one group;
        `share` is left 0. */
    static LevelShare describeGroup(const FoSet& set,
                                    const vector<int>& members);

    /** H_ii of an element's shell l, eV, from the parameter file. */
    static bool hii(const string& element, int l, double& eV);

    /**
     * Project parent MOs on the FO set.
     *
     * The FO set is orthogonalised symmetrically (S_FF^-1/2) and
     * w_k = |<FO~_k|psi>|^2 with psi normalised in the parent basis, so
     * capture = sum w <= 1 by construction.  `parentBasis` and
     * `parentOverlap` (n x n) must be what the rows are coefficients
     * of, with the geometry frame the FO set was built in; an
     * orthonormalised basis (MOPAC) cannot be used and is declined by
     * the caller (see tools/modiagram/fragorb.C).
     */
    static bool compose(const FoSet& set,
                        const vector<EspBasisFunction>& parentBasis,
                        const vector<double>& parentOverlap,
                        const vector< vector<double> >& moRows,
                        vector<MoShares>& out, string& why,
                        vector< vector<double> >* amplitudes = 0);
    //  `amplitudes`, if given, gets one row per MO with the signed
    //  <FO~_k|psi> the weights are the squares of.

    /**
     * The same projection for fragment orbitals that are already
     * coefficient rows over the parent basis (Gaussian FOs from a
     * fragment calculation), with `foFragment[k]` naming each one's
     * fragment.  Used to check the projection against Multiwfn's CDA;
     * `top` is not filled.
     */
    static bool composeGaussian(const vector<Fragment>& fragments,
                                const vector<int>& foFragment,
                                const vector< vector<double> >& foRows,
                                const vector<double>& parentOverlap,
                                const vector< vector<double> >& moRows,
                                vector<MoShares>& out, string& why);
};

#endif
