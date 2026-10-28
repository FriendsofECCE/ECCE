#ifndef LINEPROFILE_HH
#define LINEPROFILE_HH

#include <functional>
#include <string>
#include <vector>

#include "tdat/RadialDist.H"

/**
 * Values of orbitals and of the electron density along a straight line
 * through a molecule (wx-free), from DensityPlane::Evaluator.  The
 * companion of RadialDist, which averages over spheres about an atom.
 *
 * Position s is measured from the first atom of the line along it, in
 * Angstrom (or bohr).  Values: psi in length^-3/2, psi^2 and the density in
 * length^-3, in the same length unit.
 */
namespace LineProfile
{
  using std::string;
  using std::vector;

  enum Quantity { PSI, PSI2, DENSITY };

  struct Line
  {
    double start[3], end[3];     // Angstrom
    double origin[3];            // the point s = 0 (the first atom)
    double unit[3];              // from the first atom towards the second
  };

  /**
   * The line through atoms i and j (0-based), `margin` Angstrom beyond each,
   * moved sideways by `offset` Angstrom: out of the molecular plane (the
   * plane of the atoms and the line; any perpendicular for a linear
   * molecule) or within it, perpendicular to the line.  Atom marks still
   * refer to the unshifted line.
   */
  Line throughAtoms(const DensityPlane::Evaluator& ev, int i, int j, double margin,
                    double offset = 0.0, bool outOfPlane = true);

  /** An atom the line passes within `tolerance` Angstrom of. */
  struct Mark { double s; string label; };

  struct Result
  {
    vector<double>           s;
    vector<RadialDist::Curve> curves;   // .p holds the values; no maxima
    vector<Mark>             marks;
  };

  /**
   * Sample `specs` at `points` positions, plus a fine run (0.004 Angstrom)
   * through each atom the line passes, where the density is sharpest.  PSI uses each spec's first
   * member, signed; PSI2 and DENSITY the weighted sum of |psi|^2 (the
   * total density for a total spec).  onStep gets the fraction done and
   * stops the run (false returned) when it returns false.
   */
  bool compute(const DensityPlane::Evaluator& ev, const Line& line, int points,
               bool bohr, Quantity q, const vector<RadialDist::Spec>& specs,
               Result& out, std::function<bool(double)> onStep = nullptr);

  /** "s,label,..." rows. */
  string csv(const Result& r, bool bohr);
}

#endif
