#ifndef MULTIPOLE__H
#define MULTIPOLE__H

#include <string>
#include <vector>

/**
 * Cartesian multipole moments of rank k (2 quadrupole, 3 octupole, 4
 * hexadecapole) as the angular function f(n) = sum T_ij.. n_i n_j.. and the
 * surface r(n) = s |f(n)|.  No GUI dependency.
 *
 * Codes print the raw (traced) Cartesian moments, one number per distinct
 * index multiset, labelled by the indices (XY, XXZ, ...).  fromComponents
 * expands them to the full symmetric tensor.  The traces are lower
 * multipoles times r^2 and would show up as a sphere, so harmonicPart
 * removes them.
 */
namespace Multipole
{
  struct Mesh {
    std::vector<float> xyz;      // vertices, 3 per vertex, at s |f(n)|
    std::vector<float> normal;   // unit vertex normals, 3 per vertex
    std::vector<float> f;        // signed f(n) per vertex
    std::vector<int> tri;        // triangles, 3 indices, counter-clockwise
                                 // seen from outside
  };

  // Full symmetric tensor (3^rank values, row-major) from the printed
  // components; labels are index strings of x/y/z in any case.  False if a
  // label has the wrong length or a component is missing.
  bool fromComponents(int rank, const std::vector<std::string>& labels,
                      const std::vector<double>& values,
                      std::vector<double>& full);

  // f(n) for a unit vector n.
  double evaluate(int rank, const std::vector<double>& full, const double n[3]);

  // The tensor of f minus its spherical-harmonic parts of degree < rank
  // (the traces).  Idempotent.
  std::vector<double> harmonicPart(int rank, const std::vector<double>& full);

  // Icosphere with `level` subdivisions (level 0: 12 vertices); unit vertices.
  void icosphere(int level, std::vector<float>& xyz, std::vector<int>& tri);

  // The surface r = scale |f| on an icosphere; the tensor is used as given
  // (call harmonicPart first to drop the traces).
  Mesh surface(int rank, const std::vector<double>& full, double scale,
               int level);

  // Largest |f| on the sphere (sampled on an icosphere of `level`).
  double maxAbs(int rank, const std::vector<double>& full, int level = 3);
}

#endif
