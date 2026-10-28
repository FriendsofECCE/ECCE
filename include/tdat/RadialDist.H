#ifndef RADIALDIST_HH
#define RADIALDIST_HH

#include <functional>
#include <string>
#include <vector>

#include "tdat/DensityPlane.H"

class IPropCalculation;

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

  /** One orbital in a curve, with its weight (an occupation, or 1). */
  struct Member
  {
    int    mo;       // 1-based
    bool   beta;
    double weight;
  };

  /**
   * One curve to compute: the total density, or a weighted sum of |psi|^2
   * over members (`mo`/`beta`/`weight` is the first member when mo > 0,
   * `more` the rest).  Signed psi, in a line profile, uses the first only.
   */
  struct Spec
  {
    bool   total;
    int    mo;
    bool   beta;
    double weight;
    vector<Member> more;
    string label;

    Spec() : total(true), mo(0), beta(false), weight(1.0) {}
  };

  /** The spec's |psi|^2 sum (or the density) at a point, Angstrom; bohr^-3. */
  double value(const DensityPlane::Evaluator& ev, const Spec& s,
               double x, double y, double z);
  /** The spec's signed psi (its first member only); bohr^-3/2. */
  double amplitude(const DensityPlane::Evaluator& ev, const Spec& s,
                   double x, double y, double z);

  /**
   * value() and amplitude() of many specs at a point from one pass over the
   * basis functions (a cluster has ~1000 of them), with the same numbers.
   */
  class PointValues
  {
    public:
      PointValues(const DensityPlane::Evaluator& ev, const vector<Spec>& specs);
      void at(double x, double y, double z);
      double value(const Spec& s) const;
      double amplitude(const Spec& s) const;
    private:
      double psi(int mo, bool beta) const;
      const DensityPlane::Evaluator& p_ev;
      vector<char> p_needA, p_needB;
      bool p_wantRho;
      vector<double> p_psiA, p_psiB, p_phi;
      double p_rho;
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
    bool   radial;     // r^2 times the integral over directions; else the plain average
    //  Spacing of the radii: even, even in sqrt(r), or even in log(r) from
    //  1e-3 Angstrom (0 is still the first point), so a curve plotted on that
    //  axis has as many points near the nucleus as far out.
    enum Spacing { EVEN, SQRT, LOG };
    Spacing spacing;

    Options() : rMax(6.0), points(1201), lebedev(434), bohr(false), radial(true),
                spacing(EVEN) {}
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

  /** Simpson integral over a uniform grid (trapezoid if the count is even or
   *  the grid is not uniform). */
  double integrate(const vector<double>& r, const vector<double>& p);

  /** The atom nearest the centre of mass (0-based); the only atom if there is one. */
  int defaultCentre(const DensityPlane::Evaluator& ev);

  enum Spin { ALPHA, BETA, BOTH };

  /**
   * An atom's occupied MOs named by shell ("1s", "2s", "2p"): the dominant
   * angular momentum of each, counted up in energy order within it, a
   * degenerate set being one shell.  Empty for a molecule; empty strings
   * where no shell is clear.  A virtual is "3s (virtual)" when the radial
   * nodes of its type count up cleanly from the occupied shells (an IVO in
   * a diffuse basis), else "s (virtual)".
   */
  vector<string> shellLabels(IPropCalculation *calc,
                             const DensityPlane::Evaluator& ev, bool beta);

  /**
   * Radial nodes of each MO of a single atom (empty for a molecule): the
   * sign changes of psi along the direction where it is largest, so
   * n = l + nodes + 1.  -1 where it cannot be said.  meanR, when given,
   * gets <r> (bohr) along that direction.
   */
  vector<int> radialNodes(const DensityPlane::Evaluator& ev, bool beta,
                          vector<double> *meanR = 0);

  /** shellLabels for a calculation's atom; empty when it is not one atom. */
  vector<string> atomOrbitalLabels(IPropCalculation *calc, bool beta);

  /**
   * Curves for the given orbitals (1-based).  `weighted` weights each by its
   * occupation (a contribution to the density), else by 1.  `combine` joins
   * orbitals of one shell label (or, without labels, of equal energy) into
   * one curve (a tag that is not unique, a symmetry label, joins only orbitals
   * of equal energy and is shown with the orbital number).  BOTH adds the beta orbital of the same number to each curve.
   */
  vector<Spec> orbitalSpecs(const DensityPlane::Evaluator& ev,
                            const vector<int>& mos, Spin spin, bool weighted,
                            bool combine, const vector<string>& labels,
                            bool tagsUnique = true);

  /** max |sum of the non-total curves - the total curve|; negative if none. */
  double maxSumError(const vector<Curve>& curves);

  /** "r,label,label..." with one row per radius. */
  string csv(const vector<double>& r, const vector<Curve>& curves, bool bohr);
}

#endif
