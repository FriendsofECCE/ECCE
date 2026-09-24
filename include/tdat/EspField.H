/**
 * @file
 *
 * The electrostatic potential of a molecule, from its wavefunction.
 *
 *     V(r) = sum_A Z_A/|r - R_A|  -  integral rho(r')/|r - r'| dr'
 *
 * The electronic term is an integral over the electron density, not a
 * sum over fitted point charges, which is what makes this the potential
 * a unit positive test charge actually feels.
 *
 * Separate from the rendering code so the assembly -- the sign of the
 * electronic term, the factor of two on off-diagonal density matrix
 * elements, the basis normalisation -- can be tested against cases
 * whose answer is known.  A mistake in any of those produces a smooth,
 * plausible-looking, wrong surface.
 */

#ifndef ESPFIELD_HH
#define ESPFIELD_HH

#include <vector>
using std::vector;

/**
 * One atomic orbital, flattened out of the basis set.
 *
 * The angular part is a sum of Cartesian monomials, which is what a
 * spherical function is in this representation (2z^2-x^2-y^2 and so
 * on), with any shell normalisation already folded into the
 * coefficients.  Everything is in bohr.
 */
struct EspBasisFunction
{
   double center[3];

   vector<int>    powerX;            // one entry per angular term
   vector<int>    powerY;
   vector<int>    powerZ;
   vector<double> angularCoef;

   vector<double> exponent;          // one entry per primitive
   vector<double> contraction;

   EspBasisFunction() { center[0] = center[1] = center[2] = 0.0; }

   bool empty() const { return angularCoef.empty(); }

   /** The value of this basis function at a point. */
   double value(double x, double y, double z) const;
};


/** A nucleus: a charge at a point. */
struct EspNucleus
{
   double center[3];
   double charge;
};


class EspField
{
  public:

    /**
     * The significant density matrix pairs, as (mu, nu, weight).
     *
     * Built once and reused at every grid point.  Off-diagonal pairs
     * appear once with their weight doubled, since the density matrix
     * is symmetric, and pairs below the cutoff are dropped -- most of
     * them, for anything but a small molecule, and each surviving pair
     * costs a Coulomb integral at every single grid point.
     */
    struct Pairs
    {
      vector<size_t> mu;
      vector<size_t> nu;
      vector<double> weight;

      size_t size() const { return mu.size(); }
    };

    /**
     * Select the pairs worth computing from a density matrix.
     *
     * @param density  nbas x nbas, row major
     * @param cutoff   relative to the largest element
     */
    static void selectPairs(const vector<EspBasisFunction>& basis,
                            const vector<double>& density,
                            double cutoff, Pairs& pairs);

    /** The electron density at a point, from the selected pairs. */
    static double density(const vector<EspBasisFunction>& basis,
                          const Pairs& pairs,
                          double x, double y, double z);

    /** The potential at a point: nuclei minus electrons. */
    static double potential(const vector<EspBasisFunction>& basis,
                            const Pairs& pairs,
                            const vector<EspNucleus>& nuclei,
                            const double* point);

    /**
     * The number of electrons the density matrix describes, trace(P S).
     *
     * A hard invariant, and the cheapest way to tell a sound density
     * matrix from a wrongly normalised one: it must come out at the
     * molecule's electron count.  If it does not, every potential
     * computed from it is wrong by the same factor and there is no
     * point looking at the colours.
     */
    //  <a|b>.  Exposed because the diagonal of it is the check that
    //  says whether the basis this was handed is normalised at all --
    //  a question no other part of the viewer ever asks, since an MO
    //  isosurface looks the same at any scale.
    static double overlapOf(const EspBasisFunction& a,
                            const EspBasisFunction& b);

    static double electronCount(const vector<EspBasisFunction>& basis,
                                const Pairs& pairs);

    /**
     * The electronic term alone, which is negative everywhere.
     */
    static double electronicPotential(const vector<EspBasisFunction>& basis,
                                      const Pairs& pairs,
                                      const double* point);
};

#endif
