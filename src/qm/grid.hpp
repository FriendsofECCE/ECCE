// Molecular integration grid for DFT: Treutler-Ahlrichs (M4) radial points x
// Lebedev angular points per atom, with Becke atomic partition weights.
#pragma once
#include <vector>
#include "molecule.hpp"

namespace qm {

struct Grid {
  std::vector<double> x, y, z, w;  // bohr; weights include r^2 and 4*pi
  size_t size() const { return w.size(); }
};

// nrad radial points and nang Lebedev points per atom (nang must be one of
// the tabulated sizes, see lebedev_sizes()).  With prune, the angular grid is
// thinned close to the nucleus, where the density is nearly spherical; the
// full nang points are kept from about one Bragg radius outwards.
Grid make_grid(const Molecule& mol, int nrad, int nang, bool prune = true);
std::vector<int> lebedev_sizes();

}  // namespace qm
