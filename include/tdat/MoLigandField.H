/**
 * @file
 *
 * A fragment-orbital ligand-field MODEL for an ML6-style complex
 * (#162 follow-up, Cr(CO)6 as the reference case) -- four columns:
 * the metal's own valence orbitals, the metal-ligand INTERACTION
 * model (bonding / metal non-bonding / antibonding), the ligand
 * orbitals that actually bond to the metal, and the ligand-internal
 * orbitals that do not.
 *
 * WHY A SEPARATE MODEL, NOT MORE OF THE SAME MACHINERY: the SKELETON
 * construction (MoFragments::build()) draws the metal against its
 * donor atoms using the CANONICAL molecular orbitals themselves, and
 * for Cr(CO)6 that spreads the real metal-ligand interaction over
 * about two dozen levels that are mostly CO-internal -- correct, but
 * not what a textbook ligand-field diagram shows (Albright, Burdett &
 * Whangbo 2013, sec. 15.2-15.3). This class builds a SMALL, explicit
 * interaction model instead: the metal's own valence AOs plus the few
 * ligand fragment orbitals that actually couple to them, diagonalised
 * on their own -- and then reports, level by level, how much of the
 * real calculation that small model actually accounts for, because a
 * model that draws a clean textbook picture the calculation does not
 * support is worse than no picture (Andy, 2026-09-28).
 *
 * THE FOCK MATRIX IS RECONSTRUCTED, NOT PARSED: no code this project
 * integrates prints the AO Fock matrix, only S, C and the orbital
 * energies -- but for orbitals that are truly eigenvectors of F (any
 * canonical HF/DFT set is, by definition), F = S C diag(eps) C^T S
 * reproduces the same C/eps exactly on their own span (buildFock()'s
 * header has the derivation and the C^T F C = diag(eps) check this
 * relies on). With nMO < nbasis (virtuals the code did not report)
 * the reconstruction is only valid ON THAT SPAN -- it has no support
 * at all in whatever directions are missing from C, which matters
 * most for the ligand fragments' own high virtuals; see Result::note.
 *
 * wx-free (tdat), like the rest of this diagram's engine: every input
 * is a plain vector the caller (MoDiagramPanel::build(), SKELETON
 * only) has already built for the real calculation -- the same S,
 * S^{1/2}, MO coefficients and per-atom/per-shell bookkeeping
 * MoFragments::composeLevels() and MoComposition already use.
 */

#ifndef MOLIGANDFIELD_HH
#define MOLIGANDFIELD_HH

#include <string>
#include <vector>
using std::string;
using std::vector;


class MoLigandField
{
  public:

    /** One level drawn in one of the four columns. */
    struct Level
    {
      double energy;        // Hartree, this model's own eigenvalue
      string label;          // e.g. "3d", "4s", "CO sigma(5)", ...
      string irrep;          // best-effort Mulliken-style tag; may be empty
      int    degeneracy;     // orbitals merged into this one level
      double metalWeight;    // share (0..1) on the metal valence basis
      double ligandWeight;   // share (0..1) on the interacting ligand basis

      enum Character { UNKNOWN, BONDING, NONBONDING, ANTIBONDING };
      Character character;
      int    pairing;        // paired bonding/antibonding level, or -1

      //  --- validity (Andy, 2026-09-28): the model must say how much
      //  of the real calculation it actually represents, not just draw
      //  a picture. ---

      /**
       * The fraction of the best-matching canonical MO's norm (under S)
       * that falls inside the model's own small basis' span -- the
       * projector onto that span, evaluated on the canonical orbital,
       * not on this model level. -1 when no canonical match was found
       * at all (an empty model, or no MO within the matching window).
       */
      double capture;

      /** Which canonical MO(s) (indices into the caller's own MO list,
       *  0-based) this level's eigenvector overlaps most; empty if none
       *  clears MoLigandField::MATCH_OVERLAP_MIN. */
      vector<int>    canonicalMOs;

      /** |<model level | canonical MO>|^2 under S, matching canonicalMOs. */
      vector<double> canonicalOverlap;

      /** |E_model - E_canonical|, Hartree, for the best canonicalMOs
       *  entry; -1 if canonicalMOs is empty. */
      double energyDeviation;

      Level() : energy(0.0), degeneracy(0), metalWeight(0.0),
                ligandWeight(0.0), character(UNKNOWN), pairing(-1),
                capture(-1.0), energyDeviation(-1.0) {}
    };

    /** A line from a metal level to an interaction-model level, or from
     *  an interacting-ligand level to one -- kept separate from
     *  MoConnection because this model has no left/centre/right shape,
     *  it has four columns and the centre one draws its own levels. */
    struct Link
    {
      int    metalIndex;    // index into Result::metal, or -1
      int    modelIndex;    // index into Result::interaction
      int    ligandIndex;   // index into Result::ligandInteracting, or -1
      double weight;        // the model eigenvector's weight (0..1) there

      Link() : metalIndex(-1), modelIndex(-1), ligandIndex(-1), weight(0.0) {}
    };

    /** One entry of the coupling table (step 4/6): whether a ligand
     *  orbital was judged to interact with the metal, and by how much. */
    struct Coupling
    {
      string label;          // which ligand orbital, e.g. "CO#0 sigma(5)"
      double energy;          // Hartree, its own fragment-solve energy
      double maxCoupling;     // Hartree, the largest |<metal|F-eS|ligand>|
      bool   interacting;     // maxCoupling > Result::couplingThreshold
    };

    /**
     * One point of the robustness scan (item 3, Andy 2026-09-28): the
     * whole model rebuilt with the coupling threshold at `scale` times
     * the chosen value, and whether each qualitative claim survives.
     */
    struct RobustnessPoint
    {
      double scale;
      bool   piAcceptorPattern;  // the 3-level t2g pattern (#7 of the
                                 // targets doc) still holds
      bool   deltaOSign;         // Delta_o (2eg*-like minus the t2g HOMO)
                                 // still positive
      bool   eg2AboveT2g;        // the LUMO-region eg*-like level is
                                 // still above the t2g-like HOMO level
      vector<string> characterChanges; // "<label>: BONDING -> NONBONDING"

      RobustnessPoint() : scale(1.0), piAcceptorPattern(false),
                          deltaOSign(false), eg2AboveT2g(false) {}
    };

    struct Result
    {
      bool   ok;
      string note;             // why empty, or a caveat that still lets
                               // `ok` be true (e.g. a shortfall in the
                               // metal valence shell count)

      /** What was actually diagonalised, so a reader does not mistake
       *  an orbital-energy gap for an excitation energy (item 5,
       *  Andy 2026-09-28): canonical HF/DFT orbital energies, Koopmans-
       *  like at best, never a computed excited state. */
      string method;
      string energyScaleNote;

      vector<Level> metal;              // column 1
      vector<Level> interaction;         // column 2: the ML6 model levels
      vector<Level> ligandInteracting;   // column 3
      vector<Level> ligandInternal;      // column 4, one entry per
                                         // symmetry-equivalent SET (not
                                         // per ligand), with `degeneracy`
                                         // holding how many were folded in

      vector<Link>     links;
      vector<Coupling>  couplings;
      double            couplingThreshold;
      string            thresholdWhy;

      /** Symmetry-equivalence check (step 2): the largest spread, in
       *  Hartree, between corresponding orbital energies of the six
       *  (or however many) ligand fragment solves -- large means the
       *  ligands are not actually equivalent in this calculation
       *  (a distorted structure, or a bug), and the model should be
       *  read with that in mind. */
      double ligandAsymmetry;

      /** E(the eg*-like antibonding level) - E(the t2g-like HOMO
       *  level), Hartree; 0 if either could not be identified. */
      double deltaO;

      vector<string> disagreements;      // honest list, see the header
      vector<RobustnessPoint> robustness;

      enum Verdict { REPRESENTATIVE, APPROXIMATE, NOT_REPRESENTATIVE };
      Verdict verdict;
      vector<string> verdictReasons;

      Result() : ok(false), couplingThreshold(0.0), ligandAsymmetry(0.0),
                 deltaO(0.0), verdict(NOT_REPRESENTATIVE) {}
    };

    /**
     * A canonical MO's norm this close to captured by the model space
     * (or better) is what "REPRESENTATIVE" requires of every drawn
     * bonding/frontier level (Andy, 2026-09-28). Below MATCH_OVERLAP_MIN
     * of it and a canonical match is not reported as a match at all --
     * a model level with no real match is not evidence of anything.
     */
    static constexpr double MATCH_OVERLAP_MIN = 0.05;
    static constexpr double CAPTURE_REPRESENTATIVE = 0.8;
    static constexpr double CAPTURE_APPROXIMATE = 0.5;

    /**
     * Build the model.
     *
     * @param coords/elements   the WHOLE molecule (coordinationSkeleton's
     *                          own atom set is not required -- the
     *                          ligand fragments are found from `coords`/
     *                          `elements` and `metalAtomIndex` alone, by
     *                          the same covalent-bond test
     *                          MoFragments::covalentBonds() uses)
     * @param metalAtomIndex    index into elements
     * @param moEnergies/moOccupancies/moCoefficients
     *                          the calculation's OWN canonical orbitals,
     *                          ALPHA ONLY -- see @p openShell
     * @param perAtom           basis functions per atom, elements' order
     * @param shellOf           angular momentum of each basis function
     * @param overlap           nbasis*nbasis AO overlap S, row major
     * @param sqrtOverlap       S^{1/2}, from MoComposition::buildSqrtOverlap()
     * @param openShell         true when moCoefficients/moEnergies are the
     *                          alpha set of an open-shell calculation --
     *                          used only to annotate Result::method; the
     *                          reconstruction itself always uses exactly
     *                          the orbitals it is handed
     * @param couplingThresholdScale  multiplies the data-chosen coupling
     *                          threshold (see Result::thresholdWhy); 1.0
     *                          for the real model, 0.5/2.0 for the
     *                          robustness scan this fills into
     *                          Result::robustness itself
     */
    static void build(const vector<double>& coords,
                      const vector<string>& elements,
                      int metalAtomIndex,
                      const vector<double>& moEnergies,
                      const vector<double>& moOccupancies,
                      const vector< vector<double> >& moCoefficients,
                      const vector<int>& perAtom,
                      const vector<int>& shellOf,
                      const vector<double>& overlap,
                      const vector<double>& sqrtOverlap,
                      bool openShell,
                      Result& result);

    /**
     * F = S C diag(eps) C^T S, exposed for the unit tests: on a
     * complete C (nMO == nbasis, C^T S C == I) this is F exactly,
     * because C already diagonalises it; with nMO < nbasis it is the
     * unique F consistent with those orbitals AND zero on the
     * orthogonal complement of their span, which is what
     * C^T F C == diag(eps) checks.
     *
     * @param coefficients  one row per orbital, moCoefficients' own layout
     * @param energies      one per orbital
     * @param overlap       nbasis*nbasis, row major
     * @param nbasis
     * @param fock          out: nbasis*nbasis, row major
     */
    static void buildFock(const vector< vector<double> >& coefficients,
                          const vector<double>& energies,
                          const vector<double>& overlap,
                          int nbasis,
                          vector<double>& fock);

    /**
     * Solve F c = e S c for a symmetric F/S of the same size, by
     * symmetric orthogonalisation (c = S^{-1/2} v, v the eigenvectors of
     * S^{-1/2} F S^{-1/2}) -- exposed for the unit tests. An eigenvalue
     * of S at or below @p linearDependenceTol is dropped from S^{-1/2}
     * (treated as a linear dependency) rather than blown up by 1/sqrt of
     * something near zero.
     *
     * @param values   out: ascending
     * @param vectors  out: one per value, in the SAME (sub-block) AO
     *                 basis @p fock/@p overlap are in -- vectors[k][i]
     *                 is component i of eigenvector k
     * @return the number of modes kept (n minus however many were
     *         dropped as linearly dependent)
     */
    static int generalizedEigen(const vector<double>& fock,
                                const vector<double>& overlap,
                                int n,
                                vector<double>& values,
                                vector< vector<double> >& vectors,
                                double linearDependenceTol = 1.0e-8);
};

#endif
