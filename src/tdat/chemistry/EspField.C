#include <cmath>

#include "tdat/EspField.H"
#include "tdat/CoulombIntegrals.H"

namespace {

  //  Closer than this to a nucleus the point-charge term diverges while
  //  the electronic term stays finite.  Clamped rather than skipped: a
  //  single infinity would take the whole colour range with it, since
  //  the ramp is scaled to the extreme value.
  const double NUCLEUS_CLAMP = 1.0e-3;

  /** The Coulomb integral between two basis functions at a point. */
  double pairIntegral(const EspBasisFunction& a, const EspBasisFunction& b,
                      const double* point)
  {
    double total = 0.0;

    for (size_t ta = 0; ta < a.angularCoef.size(); ta++) {
      const int la[3] = { a.powerX[ta], a.powerY[ta], a.powerZ[ta] };

      for (size_t tb = 0; tb < b.angularCoef.size(); tb++) {
        const int lb[3] = { b.powerX[tb], b.powerY[tb], b.powerZ[tb] };
        const double angular = a.angularCoef[ta]*b.angularCoef[tb];
        if (angular == 0.0) continue;

        for (size_t ia = 0; ia < a.exponent.size(); ia++) {
          for (size_t ib = 0; ib < b.exponent.size(); ib++) {
            const double coef = a.contraction[ia]*b.contraction[ib];
            if (coef == 0.0) continue;

            total += angular*coef
                     * CoulombIntegrals::potential(a.center, la,
                                                   a.exponent[ia],
                                                   b.center, lb,
                                                   b.exponent[ib], point);
          }
        }
      }
    }
    return total;
  }
}


double EspBasisFunction::value(double x, double y, double z) const
{
  const double dx = x - center[0];
  const double dy = y - center[1];
  const double dz = z - center[2];
  const double rr = dx*dx + dy*dy + dz*dz;

  double radial = 0.0;
  for (size_t i = 0; i < exponent.size(); i++) {
    const double ar = exponent[i]*rr;
    if (ar < 70.0) radial += contraction[i]*exp(-ar);
  }
  if (radial == 0.0) return 0.0;

  double angular = 0.0;
  for (size_t t = 0; t < angularCoef.size(); t++) {
    angular += angularCoef[t]
               * pow(dx, powerX[t]) * pow(dy, powerY[t]) * pow(dz, powerZ[t]);
  }
  return radial*angular;
}


void EspField::selectPairs(const vector<EspBasisFunction>& basis,
                           const vector<double>& density,
                           double cutoff, Pairs& pairs)
{
  pairs.mu.clear();
  pairs.nu.clear();
  pairs.weight.clear();

  const size_t nbas = basis.size();
  if (density.size() != nbas*nbas) return;

  double largest = 0.0;
  for (size_t i = 0; i < density.size(); i++) {
    const double magnitude = fabs(density[i]);
    if (magnitude > largest) largest = magnitude;
  }
  const double threshold = largest*cutoff;

  for (size_t mu = 0; mu < nbas; mu++) {
    if (basis[mu].empty()) continue;
    for (size_t nu = mu; nu < nbas; nu++) {
      if (basis[nu].empty()) continue;

      const double value = density[mu*nbas + nu];
      if (fabs(value) <= threshold) continue;

      pairs.mu.push_back(mu);
      pairs.nu.push_back(nu);
      //  The density matrix is symmetric, so an off-diagonal pair
      //  stands for two equal terms and is counted once, doubled.
      pairs.weight.push_back((mu == nu) ? value : 2.0*value);
    }
  }
}


double EspField::overlapOf(const EspBasisFunction& a,
                           const EspBasisFunction& b)
{
  double s = 0.0;
  for (size_t ta = 0; ta < a.angularCoef.size(); ta++) {
    const int la[3] = { a.powerX[ta], a.powerY[ta], a.powerZ[ta] };
    for (size_t tb = 0; tb < b.angularCoef.size(); tb++) {
      const int lb[3] = { b.powerX[tb], b.powerY[tb], b.powerZ[tb] };
      const double angular = a.angularCoef[ta]*b.angularCoef[tb];
      if (angular == 0.0) continue;
      for (size_t ia = 0; ia < a.exponent.size(); ia++) {
        for (size_t ib = 0; ib < b.exponent.size(); ib++) {
          s += angular*a.contraction[ia]*b.contraction[ib]
               * CoulombIntegrals::overlap(a.center, la, a.exponent[ia],
                                           b.center, lb, b.exponent[ib]);
        }
      }
    }
  }
  return s;
}


double EspField::electronCount(const vector<EspBasisFunction>& basis,
                               const Pairs& pairs)
{
  double total = 0.0;

  for (size_t i = 0; i < pairs.size(); i++) {
    total += pairs.weight[i]*overlapOf(basis[pairs.mu[i]],
                                       basis[pairs.nu[i]]);
  }
  return total;
}


double EspField::density(const vector<EspBasisFunction>& basis,
                         const Pairs& pairs, double x, double y, double z)
{
  double total = 0.0;
  for (size_t i = 0; i < pairs.size(); i++) {
    total += pairs.weight[i]
             * basis[pairs.mu[i]].value(x, y, z)
             * basis[pairs.nu[i]].value(x, y, z);
  }
  return total;
}


double EspField::electronicPotential(const vector<EspBasisFunction>& basis,
                                     const Pairs& pairs, const double* point)
{
  double total = 0.0;
  for (size_t i = 0; i < pairs.size(); i++) {
    total += pairs.weight[i]
             * pairIntegral(basis[pairs.mu[i]], basis[pairs.nu[i]], point);
  }
  //  Electrons are negative.
  return -total;
}


double EspField::potential(const vector<EspBasisFunction>& basis,
                           const Pairs& pairs,
                           const vector<EspNucleus>& nuclei,
                           const double* point)
{
  double total = 0.0;

  for (size_t a = 0; a < nuclei.size(); a++) {
    double r2 = 0.0;
    for (int k = 0; k < 3; k++) {
      const double d = point[k] - nuclei[a].center[k];
      r2 += d*d;
    }
    double r = sqrt(r2);
    if (r < NUCLEUS_CLAMP) r = NUCLEUS_CLAMP;
    total += nuclei[a].charge/r;
  }

  return total + electronicPotential(basis, pairs, point);
}
