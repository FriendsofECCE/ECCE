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
#include <string>
#include <vector>
using std::string;
using std::vector;

class IPropCalculation;
struct EspBasisFunction;

namespace DensityPlane
{
  /** An orthonormal frame in the plane: point = origin + a*u + b*v. */
  struct Plane
  {
    double origin[3], u[3], v[3], n[3];
    double rms;          // out-of-plane scatter of all atoms (best fit), A
    string description;
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

  /** Planar within `tolerance` A (rms), or linear. */
  bool isPlanar(const vector<double>& coordsAng, double tolerance = 0.1);

  enum Quantity { DENSITY, ORBITAL };

  struct Mark { string label; double u, v, off; };   // off: distance to the plane

  struct Map
  {
    Quantity quantity;
    int  mo;            // 1-based, ORBITAL only
    bool beta;
    int  nu, nv;
    double u0, v0, step;
    vector<double> z;   // z[j*nu + i] at (u0 + i*step, v0 + j*step)
    double zmin, zmax;
    Plane plane;
    vector<Mark> atoms;
    string title;       // "Total density", "MO 5 (HOMO)  -0.4 Eh  2 e"
    Map();
    double u(int i) const { return u0 + i*step; }
    double v(int j) const { return v0 + j*step; }
  };

  /** One line segment of a contour, in plane coordinates (A). */
  struct Segment { double u0, v0, u1, v1; };

  /** Marching squares at `level`, linear interpolation. */
  vector<Segment> contour(const Map& m, double level);

  /** 0.001 * 2^k e/bohr^3 up to the map's maximum (at most maxLevels). */
  vector<double> densityLevels(double zmax, int maxLevels = 14);
  /** 0.02 * 2^k bohr^-3/2 up to the map's absolute maximum; draw as +-. */
  vector<double> orbitalLevels(double zabsmax, int maxLevels = 6);

  /** The map as CSV: u, v, x, y, z (stored frame, A) and the value. */
  string csv(const Map& m, const string& calcName);

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

      bool build(IPropCalculation *calc);

      int  orbitals(bool beta) const;
      /** 1-based number of the highest occupied alpha orbital. */
      int  homo() const;
      /** "O1", "H2", ... in coefficient order. */
      vector<string> atomLabels() const;

      /** Total density (alpha + beta) at a point, A, stored frame. */
      double density(double x, double y, double z) const;
      /** psi_mo at a point; mo is 1-based. */
      double orbital(int mo, bool beta, double x, double y, double z) const;

      /**
       * Evaluate on a plane.  The window is the atoms' extent plus
       * `margin`.  onRow gets 0..1 and returns false to stop (false is
       * then returned and the map is incomplete).
       */
      bool map(const Plane& plane, Quantity q, int mo, bool beta,
               double step, double margin, Map& out,
               std::function<bool(double)> onRow = nullptr) const;

    private:
      vector<EspBasisFunction> *p_basis;
      vector< vector<double> > p_c, p_cBeta;
      void functions(double x, double y, double z, vector<double>& phi) const;
      Evaluator(const Evaluator&);
      Evaluator& operator=(const Evaluator&);
  };
}

#endif
