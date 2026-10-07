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
// the tabulated sizes, see lebedev_sizes()).
Grid make_grid(const Molecule& mol, int nrad, int nang);
std::vector<int> lebedev_sizes();

}  // namespace qm
