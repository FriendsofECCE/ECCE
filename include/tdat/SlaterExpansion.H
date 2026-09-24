/**
 * @file
 *
 * Expansion of Slater-type orbitals as contracted Gaussians.
 *
 * Semiempirical codes describe their valence orbitals with Slater
 * functions, r^(n-1) exp(-zeta r), while ECCE's whole rendering path --
 * TGaussianBasisSet, TGBSConfig, ComputeMoCmd -- evaluates contracted
 * Gaussians and nothing else.  Expanding the Slater functions is what
 * lets a semiempirical calculation reach that path without a second
 * evaluator being written alongside the one every other code uses.
 *
 * The primitives available are r^l exp(-alpha r^2), where the angular
 * momentum fixes the radial power, so a 3s Slater (r^2) is fitted with
 * pure s Gaussians.  That is the classic STO-nG situation.
 *
 * Exponents are even-tempered, alpha_k = a b^k.  With them fixed the
 * coefficients are the solution of a linear least-squares problem, so
 * there is no table of published STO-nG coefficients to transcribe --
 * the expansion is derived for whatever zeta the code reports, at any
 * principal quantum number, and cannot silently disagree with it.
 */

#ifndef SLATEREXPANSION_HH
#define SLATEREXPANSION_HH

#include <vector>
using std::vector;

class SlaterExpansion
{
  public:

    /**
     * Fit one Slater function with nGauss Gaussian primitives.
     *
     * @param n      principal quantum number, 1 or greater
     * @param l      angular momentum, 0 = s, 1 = p, 2 = d
     * @param zeta   Slater exponent, in inverse bohr
     * @param nGauss number of primitives
     * @param exponents    filled with the Gaussian exponents
     * @param coefficients filled with the matching coefficients
     *
     * @return the overlap between the exact Slater function and the
     *         fit, both normalised: 1 is exact.  Callers should treat
     *         a low value as a failure rather than render from it.
     */
    static double fit(int n, int l, double zeta, int nGauss,
                      vector<double>& exponents,
                      vector<double>& coefficients);

    /** Overlap of the fit with the exact function, for a given ladder. */
    static double fitFixed(int n, int l, double zeta,
                           const vector<double>& exponents,
                           vector<double>& coefficients);
};

#endif
