// Local distortion of the bonds around one atom (#619): each chosen neighbour
// moves along the bond vector by its own dr.  No wx, no display.
#ifndef BONDDISTORT_HH
#define BONDDISTORT_HH

#include <string>
#include <vector>

class Fragment;

namespace BondDistort {

/// Indices of the atoms bonded to centre, in bond order.
std::vector<int> neighbours(const Fragment& frag, int centre);

/// Moves each atom nb[i] by dr[i] Angstrom along the unit vector centre->nb[i]
/// (positive stretches); the centre stays.  The atoms hanging off a neighbour
/// (the H of an aqua ligand) move with it, so the ligand stays intact; a
/// neighbour whose branch loops back to the centre or to another neighbour
/// moves alone.  False if an index is bad or a bond has no length.
bool apply(Fragment& frag, int centre, const std::vector<int>& nb,
           const std::vector<double>& dr);

/// The same dr for every neighbour.
std::vector<double> uniform(size_t n, double dr);

/// Symmetric Jahn-Teller preset for six neighbours: the trans pair closest to
/// axis ("x", "y", "z", or "longest" for the longest trans pair) gets +delta,
/// the other four -delta/2.  False if there are not six neighbours or the
/// axis is unknown.
bool jahnTeller(const Fragment& frag, int centre, const std::vector<int>& nb,
                double delta, const std::string& axis,
                std::vector<double>& dr);

}

#endif
