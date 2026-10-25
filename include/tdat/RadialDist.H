#ifndef RADIALDIST_HH
#define RADIALDIST_HH

#include <functional>
#include <string>
#include <vector>

#include "tdat/DensityPlane.H"

/**
 * Radial distributions about a centre (wx-free), from the stored MO
 * coefficients and basis through DensityPlane::Evaluator.
 *
 *   orbital  P(r) = r^2 * Int |psi(r,Omega)|^2 dOmega   (integrates to the
 *            orbital's population about the centre, ~1 on an atom)
 *   density  D(r) = r^2 * Int rho(r,Omega) dOmega       (integrates to the
 *            electron count)
 *
 * The angular integral is a Lebedev grid centred on `centre`.  The curves
 * are per Angstrom, or per bohr when `bohr` is set (r then in bohr).
 */
namespace RadialDist
{
  using std::string;
  using std::vector;

  /** One curve to compute: one MO (1-based, alpha or beta) or the total density. */
  struct Spec
  {
    bool   total;
    int    mo;
    bool   beta;
    string label;

    Spec() : total(true), mo(0), beta(false) {}
  };

  struct Curve
  {
    Spec           spec;
    vector<double> p;           // one value per grid radius
    vector<double> maxR;        // radii of the local maxima, in grid units
    vector<double> maxP;        // their heights
    double         integral;    // Int P dr over the grid

    Curve() : integral(0.0) {}
  };

  struct Options
  {
    double rMax;       // grid end, in the output unit
    int    points;     // grid points, odd, r = 0 included
    int    lebedev;    // angular points (a Lebedev size, else the next larger)
    bool   bohr;

    Options() : rMax(6.0), points(1201), lebedev(434), bohr(false) {}
  };

  /** The radius grid for these options, starting at 0. */
  vector<double> grid(const Options& opt);

  /**
   * The curves for `specs` about `centre` (Angstrom, the evaluator's frame).
   * onStep gets 0..1 and returns false to stop (false is then returned).
   */
  bool compute(const DensityPlane::Evaluator& ev, const double centre[3],
               const Options& opt, const vector<Spec>& specs,
               vector<Curve>& out, std::function<bool(double)> onStep = nullptr);

  /** Local maxima of a sampled curve, with a parabolic refinement. */
  void findMaxima(const vector<double>& r, const vector<double>& p,
                  vector<double>& maxR, vector<double>& maxP);

  /** Simpson integral over a uniform grid (trapezoid if the count is even). */
  double integrate(const vector<double>& r, const vector<double>& p);

  /** The atom nearest the centre of mass (0-based); the only atom if there is one. */
  int defaultCentre(const DensityPlane::Evaluator& ev);

  /** "r,label,label..." with one row per radius. */
  string csv(const vector<double>& r, const vector<Curve>& curves, bool bohr);
}

#endif
