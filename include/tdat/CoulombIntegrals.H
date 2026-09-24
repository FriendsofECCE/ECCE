/**
 * @file
 *
 * Coulomb integrals over contracted Cartesian Gaussians, for the
 * electrostatic potential.
 *
 * The electrostatic potential a unit positive test charge feels is
 *
 *     V(r) = sum_A Z_A/|r - R_A|  -  integral rho(r')/|r - r'| dr'
 *
 * The first term is trivial.  The second is the real work: it is not a
 * sum over point charges but an integral over the electron density, and
 * for Gaussian basis functions it has a closed form -- the same nuclear
 * attraction integral every quantum chemistry package computes, with
 * the "nucleus" placed at the grid point.
 *
 * Implemented by McMurchie-Davidson: expand each Cartesian pair in
 * Hermite Gaussians, then contract against the Hermite Coulomb
 * integrals R, which recur from the Boys function.
 */

#ifndef COULOMBINTEGRALS_HH
#define COULOMBINTEGRALS_HH

#include <vector>
using std::vector;

class CoulombIntegrals
{
  public:

    /**
     * The Boys function F_n(T) = integral_0^1 t^2n exp(-T t^2) dt.
     *
     * Fills values[0..n] at once, which is what the recursion below
     * needs and what makes evaluating it once per pair affordable.
     */
    static void boys(int n, double T, vector<double>& values);

    /**
     * The potential at C from the charge distribution of one primitive
     * Cartesian Gaussian pair.
     *
     * That is, the integral of G_a(r) G_b(r) / |r - C| over all space,
     * without the density matrix factor or the contraction
     * coefficients -- the caller supplies those.
     *
     * @param A,la  centre and (x,y,z) powers of the first primitive
     * @param alpha its exponent
     * @param B,lb  centre and powers of the second
     * @param beta  its exponent
     * @param C     the point the potential is wanted at
     */
    static double potential(const double* A, const int* la, double alpha,
                            const double* B, const int* lb, double beta,
                            const double* C);
};

#endif
