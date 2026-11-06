#ifndef BONDDIPOLES_HH
#define BONDDIPOLES_HH

#include <utility>
#include <vector>

/**
 * Bond dipoles from atomic charges (#849). No wx or fragment dependency.
 *
 * Each atomic charge is written as a sum of charge transfers along the bonds,
 * q_i = sum_j d_ij with d_ji = -d_ij; the minimum-norm d is taken (unique for
 * an acyclic molecule, the least-squares-smallest one for rings), found as
 * d = B^T phi with the graph Laplacian L = B B^T and L phi = q. The bond dipole
 * is mu_ij = d_ij (r_i - r_j), so the sum over bonds is exactly sum q_i r_i.
 * A connected piece with a net charge Q cannot be written that way; its Q/n
 * share per atom is left out of the bonds and returned as netChargeTerm.
 */
namespace BondDipoles {

struct Result {
  std::vector<double> delta;      // d_ij for bond k = (i, j): charge on i
  std::vector<double> mu[3];      // per bond, e Angstrom, same index
  double bondSum[3];              // sum over bonds, e Angstrom
  double netChargeTerm[3];        // sum_c Q_c * centroid_c, e Angstrom
  double pointCharge[3];          // sum q_i (r_i - origin), e Angstrom
  double residual;                // largest |q_i - sum_j d_ij| over atoms
};

// xyz[3*i..] in Angstrom. origin: where the point-charge sum is taken (the
// centre of mass for ions); bond dipoles do not depend on it.
Result compute(const std::vector<double>& q, const std::vector<double>& xyz,
               const std::vector<std::pair<int,int> >& bonds,
               const double origin[3]);

const double DEBYE_PER_E_ANGSTROM = 4.80320;

}

#endif
