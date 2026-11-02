/**
 * @file
 *
 * ELF, LOL, the reduced density gradient and sign(lambda2)rho (#798, #799,
 * #800) from the quantities DensityPlane::Evaluator::fieldsAt returns.
 * Atomic units throughout.  No wx.
 */

#ifndef DENSITYFIELDS_HH
#define DENSITYFIELDS_HH

#include "tdat/DensityPlane.H"

namespace DensityFields
{
  /** Below this total density (au) ELF and LOL are 0 and s is RDG_LARGE. */
  const double RHO_FLOOR = 1e-12;
  const double RDG_LARGE = 100.0;

  /**
   * Becke-Edgecombe ELF, spin-resolved (Multiwfn's open-shell choice):
   * D = sum_s (tau_s - |grad rho_s|^2/(8 rho_s)),
   * D0 = (3/10)(6 pi^2)^(2/3) sum_s rho_s^(5/3), ELF = 1/(1 + (D/D0)^2).
   * For a closed shell this is the usual (3/10)(3 pi^2)^(2/3) rho^(5/3).
   */
  double elf(const DensityPlane::FieldPoint& p);

  /** Localised orbital locator, t/(1+t) with t = D0/tau, same spin treatment. */
  double lol(const DensityPlane::FieldPoint& p);

  /** s = |grad rho| / (2 (3 pi^2)^(1/3) rho^(4/3)). */
  double rdg(const DensityPlane::FieldPoint& p);

  /** sign(lambda2) rho, lambda2 the middle Hessian eigenvalue. */
  double signL2Rho(const DensityPlane::FieldPoint& p);
}

#endif
