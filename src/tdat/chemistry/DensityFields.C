#include <cmath>

#include "tdat/DensityFields.H"

using DensityPlane::FieldPoint;

namespace {

const double PI = 3.14159265358979323846;

// D (the Pauli excess kinetic energy) and its homogeneous-gas reference.
void spinSums(const FieldPoint& p, double& D, double& D0, double& tau)
{
  static const double c = 0.3*pow(6.0*PI*PI, 2.0/3.0);
  D = D0 = tau = 0.0;
  for (int s = 0; s < 2; s++) {
    if (p.rho[s] <= 0.0) continue;
    const double g2 = p.grad[s][0]*p.grad[s][0] + p.grad[s][1]*p.grad[s][1]
                    + p.grad[s][2]*p.grad[s][2];
    D  += p.tau[s] - g2/(8.0*p.rho[s]);
    D0 += c*pow(p.rho[s], 5.0/3.0);
    tau += p.tau[s];
  }
}

}

double DensityFields::elf(const FieldPoint& p)
{
  if (p.rhoTotal < RHO_FLOOR) return 0.0;
  double D, D0, tau;
  spinSums(p, D, D0, tau);
  if (D0 <= 0.0) return 0.0;
  const double x = D/D0;
  return 1.0/(1.0 + x*x);
}

double DensityFields::lol(const FieldPoint& p)
{
  if (p.rhoTotal < RHO_FLOOR) return 0.0;
  double D, D0, tau;
  spinSums(p, D, D0, tau);
  if (tau <= 0.0) return 0.0;
  const double t = D0/tau;
  return t/(1.0 + t);
}

double DensityFields::rdg(const FieldPoint& p)
{
  if (p.rhoTotal < RHO_FLOOR) return RDG_LARGE;
  const double g = sqrt(p.gradTotal[0]*p.gradTotal[0] + p.gradTotal[1]*p.gradTotal[1]
                        + p.gradTotal[2]*p.gradTotal[2]);
  return g/(2.0*pow(3.0*PI*PI, 1.0/3.0)*pow(p.rhoTotal, 4.0/3.0));
}

double DensityFields::signL2Rho(const FieldPoint& p)
{
  return (p.eig[1] < 0.0 ? -1.0 : 1.0)*p.rhoTotal;
}
