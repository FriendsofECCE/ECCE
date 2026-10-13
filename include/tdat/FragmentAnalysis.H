/**
 * @file
 *
 * Fragment-orbital analysis (#201): expand each MO of a parent calculation
 * in the orbitals of N fragment calculations and report how much of the MO
 * each fragment orbital carries.  No wx and no calculation I/O here: the
 * input is plain matrices, so a fragment's orbitals can come from a real
 * calculation of the same basis (loadFragmentCalc() in FragmentLoader.H),
 * from ECCE-QM run in the parent basis, or from extended Hueckel in its own
 * minimal basis.
 *
 * With T the fragment orbitals embedded in the parent AO basis (one column
 * each), G = T'ST their overlap, and c a parent MO, the expansion
 * c = T a has a = G^-1 T'S c.  Fragments overlap, so G is not 1 between
 * them; Multiwfn's CDA (main function 16) solves the same system.
 * Mulliken weight of orbital j is a_j (G a)_j, SCPA a_j^2 / sum a^2.
 * When the fragment orbitals span the parent basis, sum_j a_j (G a)_j =
 * c'Sc: nothing is lost.  When they do not (a different, smaller basis) the
 * sum is the share of the MO the fragments can represent, reported as the
 * capture, not an error.
 *
 * Different-basis fragments plug in through FragmentOrbitalSet::crossOverlap
 * (parent AO x fragment AO); embedding them means projecting through S^-1,
 * which is declared here and not implemented yet.
 */

#ifndef FRAGMENTANALYSIS_HH
#define FRAGMENTANALYSIS_HH

#include <string>
#include <vector>
using std::string;
using std::vector;

/** The parent calculation: its AO overlap and restricted MOs. */
struct FragmentParent
{
  int nAO;
  vector<double> overlap;                   // nAO*nAO row major
  vector< vector<double> > coefficients;    // per MO, nAO
  vector<double> energies;                  // Hartree, per MO
  vector<double> occupations;               // per MO
  FragmentParent() : nAO(0) {}
};

/** One fragment's orbitals, in the fragment's OWN AO basis. */
struct FragmentOrbitalSet
{
  string name;
  int nAO;                                  // size of its own basis
  vector<double> energies;                  // Hartree, per orbital
  vector<double> occupations;               // per orbital
  vector< vector<double> > coefficients;    // per orbital, nAO
  vector<double> selfOverlap;               // nAO*nAO, or empty

  //  Same basis as the parent: own AO k is parent AO parentIndex[k].
  bool sameBasis;
  vector<int> parentIndex;

  //  Different basis: S(parent AO, own AO), nParent*nAO row major.
  vector<double> crossOverlap;

  FragmentOrbitalSet() : nAO(0), sameBasis(true) {}
};

class FragmentAnalysis
{
  public:
    struct Orbital              // one fragment orbital, in input order
    {
      int fragment;             // 0-based
      int index;                // 0-based within its fragment
      double energy;
      double occupation;
    };

    struct Checks
    {
      vector<double> orthonormality;   // per fragment: max |T'ST - 1| on its block
      vector<double> overlapMismatch;  // per fragment: max |own S - parent block|, -1 if none
      double maxNormError;             // max |c'Sc - 1| over parent MOs
      double maxCaptureError;          // same-basis only: max |capture - 1|
      Checks() : maxNormError(0), maxCaptureError(0) {}
    };

    vector<Orbital> orbitals;
    vector< vector<double> > coefficient;    // [parent MO][orbital]: a
    vector< vector<double> > mulliken;       // [parent MO][orbital], fractions
    vector< vector<double> > scpa;
    vector<double> capture;                  // per parent MO, sum of mulliken / c'Sc
    vector< vector<double> > perFragment;    // [parent MO][fragment], Mulliken
    Checks checks;
    bool allSameBasis;

    FragmentAnalysis() : allSameBasis(true) {}

    /**
     * Run the analysis.  Returns false with `problem` set when an input is
     * inconsistent, a check fails to `tolerance` (orthonormality of each
     * fragment's orbitals; for same-basis sets, the Mulliken sum being 1;
     * the fragment's own overlap equal to the parent's block to 100x
     * tolerance), or the fragment orbitals are linearly dependent.  The
     * numbers are filled in either way, for diagnosis.
     */
    bool run(const FragmentParent& parent,
             const vector<FragmentOrbitalSet>& fragments,
             double tolerance, string& problem);

    /**
     * The fragment's orbitals as columns in the parent AO basis (nAO x
     * norb, row major).  Same-basis sets embed directly.  Different-basis
     * sets need S^-1 * crossOverlap * C: not implemented, returns false.
     */
    static bool embed(const FragmentParent& parent,
                      const FragmentOrbitalSet& frag,
                      vector<double>& columns, string& problem);
};

#endif
