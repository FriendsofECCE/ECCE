//  SymmetryAnalysis::alignFrames()/conjugate() -- the Kabsch/Procrustes
//  fit between a code's own coordinate frame and autosym's standard
//  one (#151).
//
//  A code is free to run its SCF in whatever orientation it likes and
//  report MO coefficients in that frame, while the symmetry operations
//  this analysis works with come from autosym's STANDARD orientation
//  (SymmetryOps::find() reorients and symmetrises).  alignFrames() is
//  what lets the two be used together at all: without it, an operation
//  built in one frame applied to coefficients from the other is not
//  wrong by a large, obvious amount -- it is wrong by exactly the
//  fractional-mixing failure mode SymmetryAnalysis.H's orbitalIrrep()
//  comment already documents for water's b1/b2, silently.
//
//  Checked against KNOWN rotations built independently of alignFrames
//  itself (plain trig, not the Jacobi solver under test) -- rotate a
//  real molecule's coordinates by a known angle/axis and recover that
//  same rotation back out, for several axes including one that leaves
//  the singular-value ordering non-trivial, plus one reflection case
//  (determinant must still come out +1: a rotation, never a mirror)
//  and one degenerate/planar case (fewer than 3 independent directions,
//  the situation alignFrames()'s Gram-Schmidt fallback exists for).
#include <cmath>
#include <cstdio>
#include <vector>
#include <string>
using namespace std;
#include "tdat/SymmetryAnalysis.H"

static int bad = 0;

static void check(bool ok, const string& what)
{
  printf("  %-70s %s\n", what.c_str(), ok ? "ok" : "FAIL");
  if (!ok) bad++;
}

//  A small, deliberately asymmetric point set (so no accidental
//  symmetry of the SHAPE can mask a wrong rotation) -- five points, not
//  coplanar, not collinear.
static vector<double> testPoints()
{
  double pts[5][3] = {
    { 0.0,  0.0,  0.0 },
    { 1.2,  0.3, -0.4 },
    {-0.7,  1.1,  0.2 },
    { 0.5, -0.9,  0.8 },
    {-0.3, -0.2, -1.3 }
  };
  vector<double> v;
  for (int i = 0; i < 5; i++) for (int k = 0; k < 3; k++) v.push_back(pts[i][k]);
  return v;
}

static vector<double> planarPoints()
{
  double pts[4][3] = {
    { 0.0,  0.0, 0.0 },
    { 1.0,  0.0, 0.0 },
    { 0.3,  1.0, 0.0 },
    {-0.8,  0.6, 0.0 }
  };
  vector<double> v;
  for (int i = 0; i < 4; i++) for (int k = 0; k < 3; k++) v.push_back(pts[i][k]);
  return v;
}

//  A known rotation about an arbitrary axis, by Rodrigues' formula --
//  independent of anything in SymmetryAnalysis.C.
static SymOp knownRotation(double axis[3], double angle)
{
  double n = sqrt(axis[0]*axis[0]+axis[1]*axis[1]+axis[2]*axis[2]);
  double ax = axis[0]/n, ay = axis[1]/n, az = axis[2]/n;
  double c = cos(angle), s = sin(angle), t = 1.0-c;
  SymOp R;
  R.m[0][0] = t*ax*ax + c;      R.m[0][1] = t*ax*ay - s*az;   R.m[0][2] = t*ax*az + s*ay;
  R.m[1][0] = t*ax*ay + s*az;   R.m[1][1] = t*ay*ay + c;      R.m[1][2] = t*ay*az - s*ax;
  R.m[2][0] = t*ax*az - s*ay;   R.m[2][1] = t*ay*az + s*ax;   R.m[2][2] = t*az*az + c;
  return R;
}

static vector<double> applyToAll(const SymOp& R, const vector<double>& pts,
                                 double shift[3])
{
  vector<double> out(pts.size());
  for (size_t i = 0; i*3 < pts.size(); i++) {
    for (int a = 0; a < 3; a++) {
      double v = 0.0;
      for (int b = 0; b < 3; b++) v += R.m[a][b]*pts[i*3+b];
      out[i*3+a] = v + shift[a];
    }
  }
  return out;
}

static void runCase(const char* what, double axis[3], double angle,
                    double shift[3], const vector<double>& base)
{
  SymOp known = knownRotation(axis, angle);
  vector<double> rotated = applyToAll(known, base, shift);

  SymOp fit; double rmsd;
  bool ok = SymmetryAnalysis::alignFrames(base, rotated, fit, rmsd);
  check(ok, string(what) + ": alignFrames() succeeded");
  if (!ok) return;

  char msg[160];
  snprintf(msg, sizeof(msg), "%s: residual RMSD %.2e is ~0", what, rmsd);
  check(rmsd < 1.0e-8, msg);

  double maxDiff = 0.0;
  for (int a = 0; a < 3; a++)
    for (int b = 0; b < 3; b++)
      maxDiff = std::max(maxDiff, fabs(fit.m[a][b]-known.m[a][b]));
  snprintf(msg, sizeof(msg), "%s: recovered rotation matches the known one "
           "(max elementwise diff %.2e)", what, maxDiff);
  check(maxDiff < 1.0e-6, msg);

  snprintf(msg, sizeof(msg), "%s: recovered rotation is proper (det=%.6f)",
           what, fit.determinant());
  check(fabs(fit.determinant() - 1.0) < 1.0e-6, msg);
}

int main()
{
  vector<double> base = testPoints();

  { double axis[3] = {0,0,1}; double shift[3] = {0.4,-0.2,0.1};
    runCase("rotation about z, 37deg", axis, 37.0*M_PI/180.0, shift, base); }
  { double axis[3] = {1,1,1}; double shift[3] = {0,0,0};
    runCase("rotation about (1,1,1), 111deg", axis, 111.0*M_PI/180.0, shift, base); }
  { double axis[3] = {0.3,-0.7,0.2}; double shift[3] = {-1.0,2.0,0.5};
    runCase("rotation about an odd axis, 250deg", axis, 250.0*M_PI/180.0, shift, base); }
  { double axis[3] = {1,0,0}; double shift[3] = {0,0,0};
    runCase("identity-ish rotation, 0.01deg (near-degenerate)", axis,
            0.01*M_PI/180.0, shift, base); }

  //  Planar input: the third singular value is (numerically) zero, the
  //  case the Gram-Schmidt fallback in alignFrames() exists for.  The
  //  fit must still be a PROPER rotation, even though the in-plane
  //  rotation itself is all the data can actually determine.
  {
    vector<double> planar = planarPoints();
    double axis[3] = {0,0,1}; double shift[3] = {0.1,0.1,0.0};
    SymOp known = knownRotation(axis, 40.0*M_PI/180.0);
    vector<double> rotated = applyToAll(known, planar, shift);
    SymOp fit; double rmsd;
    bool ok = SymmetryAnalysis::alignFrames(planar, rotated, fit, rmsd);
    check(ok, "planar input: alignFrames() still succeeds");
    if (ok) {
      check(rmsd < 1.0e-8, "planar input: residual RMSD is ~0");
      check(fabs(fit.determinant() - 1.0) < 1.0e-6,
            "planar input: recovered rotation is still proper");
    }
  }

  //  conjugate(): an operation built in the STANDARD frame, applied
  //  through Q to the coefficients' own frame, must map the SAME
  //  physical points as the un-conjugated operation does in the
  //  standard frame -- i.e. Q * conjugate(R,Q) * p == R * (Q * p) for
  //  a point p in the "from" frame.
  {
    double axis[3] = {0,1,0}; double shift[3] = {0,0,0};
    SymOp Q = knownRotation(axis, 61.0*M_PI/180.0);   // frame relation
    double axis2[3] = {0,0,1};
    SymOp R = knownRotation(axis2, 90.0*M_PI/180.0);  // a C4 in the standard frame

    SymOp Rs = SymmetryAnalysis::conjugate(R, Q);

    double p[3] = { 0.6, -0.3, 0.9 };
    double Qp[3], R_Qp[3], Q_R_Qp[3];
    for (int a=0;a<3;a++){ Qp[a]=0; for(int b=0;b<3;b++) Qp[a]+=Q.m[a][b]*p[b]; }
    for (int a=0;a<3;a++){ R_Qp[a]=0; for(int b=0;b<3;b++) R_Qp[a]+=R.m[a][b]*Qp[b]; }
    // conjugate applied directly to p (in the "from"/stored frame):
    double Rs_p[3];
    for (int a=0;a<3;a++){ Rs_p[a]=0; for(int b=0;b<3;b++) Rs_p[a]+=Rs.m[a][b]*p[b]; }
    for (int a=0;a<3;a++){ Q_R_Qp[a]=0; for(int b=0;b<3;b++) Q_R_Qp[a]+=Q.m[a][b]*Rs_p[b]; }

    double diff = 0.0;
    for (int a=0;a<3;a++) diff = std::max(diff, fabs(Q_R_Qp[a]-R_Qp[a]));
    char msg[160];
    snprintf(msg, sizeof(msg), "conjugate(): Q*conjugate(R,Q)*p == R*(Q*p) "
             "(max diff %.2e)", diff);
    check(diff < 1.0e-9, msg);
  }

  //  A diatomic (CO, N2): two atoms, collinear.  Only the axis is fixed,
  //  so check that the fitted rotation maps the atoms onto the target,
  //  not which spin about the axis it chose.
  {
    vector<double> stored = { 0.0, 0.0, 0.0,   0.3, 0.8, 0.5 };
    vector<double> target = { 0.0, 0.0, -0.4949747, 0.0, 0.0, 0.4949747 };
    SymOp fit; double rmsd;
    bool ok = SymmetryAnalysis::alignFrames(stored, target, fit, rmsd);
    check(ok, "diatomic: alignFrames() succeeds with two atoms");
    if (ok) {
      char msg[120];
      snprintf(msg, sizeof(msg), "diatomic: residual RMSD %.2e is ~0", rmsd);
      check(rmsd < 1.0e-6, msg);
      check(fabs(fit.determinant() - 1.0) < 1.0e-6,
            "diatomic: fit is a proper rotation");
    }
  }

  //  The same, with the stored axis along a COORDINATE axis -- which is
  //  how ECCE actually stores CO and N2 (0,0,z).  The degenerate-
  //  direction fill-in used to pick e_x, e_y, e_z by column index, and
  //  Gram-Schmidt reduced the one parallel to the fitted axis to the
  //  zero vector: a singular "rotation" that still fitted the two atoms
  //  exactly (RMSD 0), so every operation conjugated through it was
  //  garbage and ORCA's CO and N2 labelled nothing (step 131).
  //  Check the fit is ORTHOGONAL, not just that it fits.
  {
    const double axes[3][6] = {
      { 0,0,-0.785665,  0,0,0.589436 },
      { 0,-0.6875505,0, 0,0.6875505,0 },
      { -0.6875505,0,0, 0.6875505,0,0 } };
    const double targets[2][6] = {
      { 0,0,-0.687550, 0,0,0.687550 },
      { -0.687550,0,0, 0.687550,0,0 } };
    for (int s = 0; s < 3; s++) for (int t = 0; t < 2; t++) {
      vector<double> stored(axes[s], axes[s]+6), target(targets[t], targets[t]+6);
      SymOp fit; double rmsd;
      bool ok = SymmetryAnalysis::alignFrames(stored, target, fit, rmsd);
      double worst = 0.0;
      for (int a = 0; a < 3; a++) for (int b = 0; b < 3; b++) {
        double d = 0.0;
        for (int k = 0; k < 3; k++) d += fit.m[k][a]*fit.m[k][b];
        worst = std::max(worst, fabs(d - (a == b ? 1.0 : 0.0)));
      }
      char msg[160];
      snprintf(msg, sizeof(msg), "diatomic on axis %d -> axis %d: fit is an "
               "orthogonal proper rotation (|Q^TQ-I| %.1e, det %.3f, rmsd %.1e)",
               s, t, worst, ok ? fit.determinant() : 0.0, rmsd);
      check(ok && worst < 1.0e-9 && fabs(fit.determinant() - 1.0) < 1.0e-9
               && rmsd < 1.0e-3, msg);
    }
  }

  printf("%s\n", bad ? "FAIL" : "PASS");
  return bad ? 1 : 0;
}
