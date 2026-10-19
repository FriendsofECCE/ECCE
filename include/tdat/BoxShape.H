// The shape GROMACS actually simulates for its triclinic boxes, drawn in
// place of the box-vector parallelepiped (#930).  wx-free.
#ifndef BOXSHAPE_HH
#define BOXSHAPE_HH

#include <array>
#include <utility>
#include <vector>

namespace boxshape {

typedef std::array<double,3> Vec;

struct Shape {
  std::vector<Vec> vertices;                 // centred on the lattice point at the origin
  std::vector<std::pair<int,int> > edges;    // indices into vertices
  int nfaces;
};

// True when a,b,c are one of the reduced GROMACS forms of the rhombic
// dodecahedron (xy-square or xy-hexagon) or the truncated octahedron.
bool isGromacsCompactBox(const Vec& a, const Vec& b, const Vec& c);

// Wigner-Seitz cell of the lattice a,b,c, centred on the origin.
bool wignerSeitz(const Vec& a, const Vec& b, const Vec& c, Shape& out);

}
#endif
