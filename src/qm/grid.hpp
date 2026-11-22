// Molecular integration grid for DFT: Treutler-Ahlrichs (M4) radial points x
// Lebedev angular points per atom, with Becke atomic partition weights.
#pragma once
#include <array>
#include <vector>
#include "molecule.hpp"

namespace qm {

struct Grid {
  std::vector<double> x, y, z, w;  // bohr; weights include r^2 and 4*pi
  std::vector<int> atom;          // centre the point moves with
  std::vector<double> atomic_w;   // radial/angular weight before partitioning
  size_t size() const { return w.size(); }
};

class BeckePartition {
 public:
  explicit BeckePartition(const Molecule& mol);
  // Optional nuclear derivative (3 per atom), including point motion with owner.
  double weight(const std::array<double, 3>& point, int owner,
                std::vector<double>* gradient = nullptr) const;

 private:
  const Molecule& mol_;
  std::vector<double> aij_, rij_;
};

// nrad radial points and nang Lebedev points per atom (nang must be one of
// the tabulated sizes, see lebedev_sizes()).  With prune, the angular grid is
// thinned close to the nucleus, where the density is nearly spherical; the
// full nang points are kept from about one Bragg radius outwards.
Grid make_grid(const Molecule& mol, int nrad, int nang, bool prune = true);
std::vector<int> lebedev_sizes();
// Rows {x, y, z, weight} of the npts-point Lebedev rule on the unit sphere
// (weights sum to 1).
std::vector<std::array<double, 4>> lebedev_rule(int npts);

}  // namespace qm
