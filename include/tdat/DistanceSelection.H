#ifndef DISTANCESELECTION_H
#define DISTANCESELECTION_H

#include <vector>
class Fragment;

// Atom indices are zero-based; distances and lattice vectors are in Angstrom.
std::vector<int> selectWithinDistance(const Fragment& frag,
    const std::vector<int>& seeds, double radius, bool molecules, bool keep);

#endif
