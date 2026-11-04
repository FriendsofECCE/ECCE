/**
 * @file
 *
 * The electron density, or one molecular orbital, on a plane through a
 * molecule (#215), evaluated from the stored MO coefficients and basis
 * so it works for every code whose MOs ECCE reads.  No wx: the Builder's
 * Density Map panel only gathers options and draws what this returns.
 *
 * Everything is in the frame the coefficients were computed in (the
 * calculation's last geometry step).  Lengths are Angstrom; the density
 * is in e/bohr^3 and an orbital in bohr^-3/2.
 */

#ifndef DENSITYPLANE_HH
#define DENSITYPLANE_HH

#include <functional>
#include <memory>
#include <string>
#include <vector>
using std::string;
using std::vector;

class IPropCalculation;
class Fragment;
struct EspBasisFunction;

namespace DensityPlane
{
  /** An orthonormal frame in the plane: point = origin + a*u + b*v. */
  struct Plane
  {
    double origin[3], u[3], v[3], n[3];
    double rms;          // out-of-plane scatter of all atoms (best fit), A
    double offset;       // distance moved along n from the base plane, A
    string description;  // includes the offset ("0.8 A above ...")
    string base;         // without it
    Plane();
  };

  enum PlaneKind {
    MOLECULAR,    // best fit through all atoms; a linear molecule: through the axis
    THREE_ATOMS,  // through atoms a, b, c (a->b is the horizontal axis)
    AXIAL         // through atoms a, b, perpendicular to the molecular plane
  };

  /** Atoms are 0-based indices; a, b, c are used as the kind requires. */
  bool makePlane(PlaneKind kind, const vector<double>& coordsAng,
                 const vector<string>& labels, int a, int b, int c,
                 Plane& plane, string& complaint);

  /** Three centroid planes, ordered by increasing RMS distance (A).
   *  Empty for fewer than three atoms or a linear geometry. */
  vector<Plane> principalPlanes(const vector<double>& coordsAng);

  /**
   * The plane moved `distance` A along its normal (from the plane as it
   * was before any offset), for the offset slider.  The description then
   * reads "0.8 A above plane through C1, C2, C3".
   */
  Plane offsetPlane(const Plane& plane, double distance);

  /**
   * The plane parallel to the screen through the atoms' centre.  `right`
   * and `up` are the camera's screen axes in the stored frame; the plane
   * keeps them as its u and v, so it is a plane, not a view.
   */
  bool makeViewPlane(const vector<double>& coordsAng, const double right[3],
                     const double up[3], Plane& plane, string& complaint);

  /** Same energy within this many Eh: the MO diagram's grouping tolerance. */
  const double DEGENERACY_TOLERANCE = 1.5e-5;

  /** 1-based orbitals degenerate with `mo` (including it), in order. */
  vector<int> degenerateSet(const vector<double>& energy, int mo,
                            double tolerance = DEGENERACY_TOLERANCE);

  /** Planar within `tolerance` A (rms), or linear. */
  bool isPlanar(const vector<double>& coordsAng, double tolerance = 0.1);

  /** Collinear atoms. */
  bool isLinear(const vector<double>& coordsAng);

  /** A molecular or best-fit plane exists (not a linear geometry). */
  bool hasMolecularPlane(const vector<double>& coordsAng);

  /** Covalent-radius bonding of atoms a and b (0-based), as a hint for defaults. */
  bool bonded(const vector<string>& symbols, const vector<double>& coordsAng,
              int a, int b);

  /** ORBITAL_SET: the sum of psi^2 over the orbitals degenerate with `mo`. */
  enum Quantity { DENSITY, ORBITAL, ORBITAL_SET };

  struct Mark { string label; double u, v, off; };   // off: distance to the plane

  struct Map
  {
    Quantity quantity;
    int  mo;            // 1-based, ORBITAL and ORBITAL_SET
    bool beta;
    int  nu, nv;
    double u0, v0, step;
    vector<double> z;   // z[j*nu + i] at (u0 + i*step, v0 + j*step)
    double zmin, zmax;
    Plane plane;
    vector<Mark> atoms;
    bool difference;    // a density difference: signed, drawn on a diverging scale
    double colourLimit; // difference: |value| at full colour (99th percentile of |z|)
    double levelTop;    // > 0: level ladder from this maximum, not zmax/zmin
    string title;       // "Total density", "MO 5 (HOMO)  -0.4 Eh  2 e"
    Map();
    double u(int i) const { return u0 + i*step; }
    double v(int j) const { return v0 + j*step; }
  };

  /** The window the atoms need on the plane: extent + margin, A. */
  void window(const Plane& plane, const vector<double>& coordsAng,
              double margin, double& u0, double& u1, double& v0, double& v1);

  /** A lone atom defines no plane, so the panel opens on the screen plane;
   *  two atoms get the molecular plane through their axis, as CO2 does. */
  inline bool startsOnViewPlane(size_t natoms) { return natoms <= 1; }

  /** One line segment of a contour, in plane coordinates (A). */
  struct Segment { double u0, v0, u1, v1; };

  /** Marching squares at `level`, linear interpolation. */
  vector<Segment> contour(const Map& m, double level);

  /** Value at (u, v) of the plane, in the stored frame (x, y, z), A. */
  void pointOn(const Plane& plane, double u, double v, double xyz[3]);

  /** 0.001 * 2^k e/bohr^3 up to the map's maximum (at most maxLevels). */
  vector<double> densityLevels(double zmax, int maxLevels = 14);
  /** 0.02 * 2^k bohr^-3/2 up to the map's absolute maximum; draw as +-. */
  vector<double> orbitalLevels(double zabsmax, int maxLevels = 6);
  /** The levels to draw for `m`: the fixed series above when it gives at
   *  least three, else max/2^k from the plane's own maximum (`scaled`),
   *  so a core orbital or a weak plane still shows its shape. */
  vector<double> mapLevels(const Map& m, bool& scaled);

  /** The map as CSV: u, v, x, y, z (stored frame, A) and the value. */
  string csv(const Map& m, const string& calcName);

  /**
   * The density and what ELF, LOL and NCI need, at one point, in bohr units.
   * Per spin: rho, gradient, tau = 1/2 sum n_i |grad psi_i|^2.  A restricted
   * open shell (one MO set, occupations 0..2) puts min(n,1) electrons of an
   * orbital in alpha and the rest in beta.
   */
  struct FieldPoint
  {
    double rho[2];        // alpha, beta
    double grad[2][3];
    double tau[2];
    double rhoTotal;
    double gradTotal[3];
    double hess[6];       // total density: xx yy zz xy xz yz
    double eig[3];        // ascending
    FieldPoint();
  };

  class Evaluator;

  /**
   * Whether `fragment` can be subtracted from `whole` (#726): every fragment
   * atom sits on an atom of `whole` of the same element within `tolerance` A.
   * `used` (one flag per atom of `whole`, may be shared over several
   * fragments) catches an atom claimed twice.  On refusal `complaint` says
   * which atom and why.
   */
  bool matchFragment(const Evaluator& whole, const Evaluator& fragment,
                     vector<char>& used, string& complaint,
                     double tolerance = 0.05);

  class Evaluator
  {
    public:
      vector<string> symbols;
      vector<double> coordsAng;     // atom-major, stored frame
      vector<double> energy, occ;   // alpha (or the only) set
      vector<double> energyBeta, occBeta;
      bool unrestricted;
      string complaint;

      Evaluator();
      ~Evaluator();

      //  shown: the geometry on screen (else the final step's), so a surface sits on the atoms drawn.
      bool build(IPropCalculation *calc, const Fragment *shown = 0);

      int  orbitals(bool beta) const;
      /** 1-based number of the highest occupied alpha orbital. */
      int  homo() const;
      /** "O1", "H2", ... in coefficient order. */
      vector<string> atomLabels() const;

      /** Total density (alpha + beta) at a point, A, stored frame.  With
       *  fragments added it is the difference: this - sum of the fragments. */
      double density(double x, double y, double z) const;

      /**
       * Make density() and the density maps/profiles the difference
       * rho(this) - sum rho(fragment), each from its own MOs and basis (#726).
       * The fragment must pass matchFragment(); `name` is for the title.
       */
      void addFragment(std::shared_ptr<const Evaluator> fragment, const string& name);
      void clearFragments();
      bool isDifference() const { return !p_fragments.empty(); }
      /** "Density difference: complex - H_a - H_b" for titles. */
      string differenceTitle() const;
      /** True when alpha and beta electrons differ in number: a second
       *  MO set, or a restricted open shell (occupations other than 0, 2). */
      bool openShell() const;
      /** Spin density, alpha - beta, at a point.  A restricted open shell
       *  has one MO set; an occupation n of 0..2 then gives min(n, 2-n). */
      double spinDensity(double x, double y, double z) const;
      /** Integrate the spin density (and the total) over a cube grid, bohr
       *  `step`, the atoms' extent plus `margin` Angstrom.  A smooth
       *  Gaussian-basis density converges fast; a tight core needs a small
       *  step, the spin density seldom has one. */
      void integrate(double step, double margin, double& total, double& spin) const;
      /** psi_mo at a point; mo is 1-based. */
      double orbital(int mo, bool beta, double x, double y, double z) const;
      /**
       * One pass over the basis functions for many values at a point: psi of
       * every orbital flagged in needA/needB (others left 0), and, when
       * wantRho, the density.  Bit-identical to orbital() and density(), at
       * the cost of one basis evaluation instead of one per call.  `phi` is
       * scratch kept by the caller.
       */
      void evaluateAt(double x, double y, double z, const vector<char>& needA,
                      const vector<char>& needB, bool wantRho, vector<double>& psiA,
                      vector<double>& psiB, vector<double>& phi, double& rho) const;

      /** Everything in FieldPoint at x, y, z (A, stored frame). */
      void fieldsAt(double x, double y, double z, FieldPoint& out) const;

      /**
       * Evaluate on a plane.  The window is the atoms' extent plus
       * `margin`.  onRow gets 0..1 and returns false to stop (false is
       * then returned and the map is incomplete).  `win` (u0, u1, v0, v1 in
       * A) replaces the atoms' window, for a zoomed map.
       */
      bool map(const Plane& plane, Quantity q, int mo, bool beta,
               double step, double margin, Map& out,
               std::function<bool(double)> onRow = nullptr,
               const double *win = nullptr) const;

    private:
      vector< std::shared_ptr<const Evaluator> > p_fragments;
      vector<string> p_fragmentNames;
      vector<EspBasisFunction> *p_basis;
      vector< vector<double> > p_c, p_cBeta;
      void functions(double x, double y, double z, vector<double>& phi) const;
      Evaluator(const Evaluator&);
      Evaluator& operator=(const Evaluator&);
  };
}

#endif
