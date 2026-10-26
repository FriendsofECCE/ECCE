/**
 * @file
 */
#ifndef COMPACTCELL_HH
#define COMPACTCELL_HH

#include <array>
#include <vector>

/**
 * Wrapping a periodic system into the compact cell of a triclinic box, as
 * gmx trjconv -pbc mol -ur compact does.  Plain geometry: no GUI and no
 * GROMACS needed.  The units are the caller's (cell and bond length must
 * agree).
 */
namespace CompactCell {

/**
 * Makes each molecule whole and moves it, by lattice vectors only, to the
 * image whose centre lies nearest (a+b+c)/2.  A molecule is a run of
 * residues joined by a heavy-atom pair closer than bondLength; the atoms of
 * a residue are consecutive and share residueOf.  A molecule at least 0.9 of
 * a cell length long along an axis (periodic) is left as it was.
 *
 * @param x         atom positions, modified in place
 * @param residueOf per atom; equal for consecutive atoms of one residue
 * @param heavy     per atom; nonzero for non-hydrogens
 * @param cell      rows a, b, c in GROMACS' lower triangular form
 * @return false, x untouched, for a malformed box
 */
bool wrap(std::vector<std::array<double, 3> >& x,
          const std::vector<int>& residueOf, const std::vector<char>& heavy,
          const double cell[3][3], double bondLength);

}
#endif
