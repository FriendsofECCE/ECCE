/**
 * @file
 *
 * Builds an ECCE basis set description for a semiempirical calculation.
 *
 * Semiempirical codes never write a basis set: the valence basis is
 * implied by the Hamiltonian, and all the code reports is one Slater
 * exponent per shell per element.  ECCE's rendering path needs a real
 * TGBSConfig, so this turns those exponents into the NumericalBasis text
 * that ICalcUtils::importConfig() already reads.
 *
 * Going through that text rather than assembling TGaussianBasisSet by
 * hand is deliberate -- it is the same path an imported basis takes, so
 * there is one construction routine to keep correct rather than two.
 */

#ifndef SLATERBASISSET_HH
#define SLATERBASISSET_HH

#include <string>
#include <vector>
using std::string;
using std::vector;

class SlaterBasisSet
{
  public:

    /** One element's valence shells, as the code reports them. */
    struct Element {
      string symbol;
      int    atomicNumber;
      double zetaS;       // zero where the shell is absent
      double zetaP;
      double zetaD;
    };

    /**
     * The principal quantum number of an element's valence s, p and d
     * shells, following MOPAC's own convention.
     *
     * These are not simply the row of the periodic table: the noble
     * gases take the next s shell, and helium takes a 2p.  Getting one
     * wrong changes the radial power and so the shape of the orbital,
     * without changing anything that would fail visibly.
     */
    static void principalQuantumNumbers(int atomicNumber,
                                        int& nS, int& nP, int& nD);

    /**
     * The NumericalBasis section describing these elements.
     *
     * @param elements  one entry per distinct element
     * @param nGauss    primitives per shell; six is accurate to better
     *                  than 1e-4 for every shell MOPAC can produce
     * @param basis     filled with the text
     *
     * @return false if any shell could not be expanded, in which case
     *         nothing should be rendered rather than something wrong.
     */
    static bool numericalBasis(const vector<Element>& elements,
                               int nGauss, string& basis);
};

#endif
