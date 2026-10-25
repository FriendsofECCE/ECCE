// Implicit solvent: conductor-like polarisable continuum (CPCM) on a union-of-
// spheres cavity with smooth Gaussian surface charges.
//
// Sources (formulas re-derived and checked against ORCA 6.1.1 and the Born
// ion, tests/qm/unit_test.cpp and docs/claude/codereg/ecceqm-cpcm-solvent.md):
//  * Barone & Cossi, J. Phys. Chem. A 102, 1995 (1998): CPCM, K q = -f(eps) V.
//  * York & Karplus, J. Phys. Chem. A 103, 11060 (1999) and Scalmani & Frisch,
//    J. Chem. Phys. 132, 114110 (2010): Gaussian surface charges whose
//    self-interaction is divided by a switching function, so that the energy
//    is continuous when the cavity changes (SWIG/ISWIG).
#pragma once
#include <array>
#include <string>
#include <vector>
#include "basis.hpp"
#include "common.hpp"
#include "molecule.hpp"

namespace qm {

// Static dielectric constant of a named solvent (case-insensitive); false if
// the name is unknown.
bool solvent_epsilon(const std::string& name, double& eps);
std::string solvent_names();  // "water, methanol, ..."

// Cavity radius of element Z in angstrom before scaling (Bondi; H 1.10 as in
// ORCA).  Throws for elements without a value.
double cavity_radius(int Z);

// The surface charges of a molecule in a dielectric.
class Cpcm {
 public:
  // Spheres of cavity_radius(Z) * radius_scale on every atom, nang Lebedev
  // points on each.
  Cpcm(const Molecule& mol, double eps, double radius_scale = 1.2, int nang = 302);

  int size() const { return static_cast<int>(pos_.size()); }
  const std::vector<std::array<double, 3>>& points() const { return pos_; }  // bohr
  double factor() const { return f_; }                                        // f(eps)
  // Exposed fraction (switching function) of each point, in (0, 1].
  const std::vector<double>& weight() const { return F_; }
  // Potential of the nuclei at the points.
  Vec nuclear_potential(const Molecule& mol) const;
  // Charges q = F p with M p = -f V; the energy of the polarised medium is q.V/2.
  Vec charges(const Vec& V) const;

 private:
  double f_;
  std::vector<std::array<double, 3>> pos_;
  std::vector<double> zeta_, F_;
  Eigen::PartialPivLU<Mat> lu_;
};

// Surface charges coupled to the electrons of a basis set: V(D) at the points
// and the Fock contribution of a set of charges.
class CpcmField {
 public:
  CpcmField(Basis& basis, const Molecule& mol, double eps, double radius_scale = 1.2);
  const Cpcm& cavity() const { return cav_; }
  // Total potential (nuclei minus electrons of density Dt) at the points.
  Vec potential(const Mat& Dt);
  // Fock matrix term -sum_k q_k <mu|1/|r-r_k||nu> (both spins alike).
  Mat fock(const Vec& q);

 private:
  Basis& basis_;
  Cpcm cav_;
  Vec vnuc_;
  // <mu|1/|r-r_k||nu> per point, one row each, if it fits in memory; else
  // recomputed in every SCF cycle.
  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> P_;
  bool cached_ = false;
  void point_matrix(int k, std::vector<double>& env, std::vector<double>& buf, std::vector<double>& cache, Mat& out) const;
};

}  // namespace qm
