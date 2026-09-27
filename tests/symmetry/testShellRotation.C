//  ShellRotation::buildD() -- does a symmetry operation's matrix on a
//  whole shell (s, p, d, Cartesian or spherical) come out right, for
//  ECCE's REAL angular tables (#151)?
//
//  Loads Gaussian-16's actual MOOrdering (JCode::getAngFunc(), the same
//  one ai.gauss16/the ESP field evaluator use), not a hand-built one --
//  a p or d shell's function ORDER inside one code's table is not
//  assumed to be x,y,z (see the earlier ORCA p-ordering fix, "ORCA
//  p functions were declared x,y,z; ORCA prints pz,px,py"), so a test
//  that hardcodes an order would validate the test's own assumption
//  instead of the table ECCE actually ships.
//
//  Independent oracles, not self-consistency:
//    * the identity operation must give D = the identity matrix, for
//      every shell type -- true by definition, no geometry needed.
//    * a p shell IS x, y, z (up to which slot holds which, read from
//      the table itself): the closed-form D for a rotation matrix R
//      is derivable directly from R and that ordering, with no
//      reference to buildD()'s own machinery.
//    * group closure: applying a four-fold rotation's D four times (or
//      a mirror's D twice, or a threefold's three times) must return
//      the identity, because the OPERATIONS do -- this holds for any
//      shell, including d, where no hand-derived closed form is used.
//
//  Needs a build tree (for JCode/CodeFactory/XML) -- see run_tests.py.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;

#include "tdat/ShellRotation.H"
#include "tdat/SymmetryAnalysis.H"
#include "tdat/TGBSAngFunc.H"
#include "dsm/TGaussianBasisSet.H"
#include "dsm/JCode.H"
#include "dsm/CodeFactory.H"

static int bad = 0;

static void check(bool ok, const string& what)
{
  printf("  %-70s %s\n", what.c_str(), ok ? "ok" : "FAIL");
  if (!ok) bad++;
}

static SymOp rotationAboutZ(double angleDeg)
{
  double t = angleDeg*M_PI/180.0;
  SymOp R;
  R.m[0][0]=cos(t); R.m[0][1]=-sin(t); R.m[0][2]=0;
  R.m[1][0]=sin(t); R.m[1][1]= cos(t); R.m[1][2]=0;
  R.m[2][0]=0;      R.m[2][1]=0;       R.m[2][2]=1;
  return R;
}

static SymOp mirrorXY()
{
  SymOp R;
  for (int i=0;i<3;i++) for (int j=0;j<3;j++) R.m[i][j] = (i==j)?1.0:0.0;
  R.m[2][2] = -1.0;
  return R;
}

static vector< vector<double> > matmul(const vector< vector<double> >& A,
                                       const vector< vector<double> >& B)
{
  size_t n = A.size();
  vector< vector<double> > C(n, vector<double>(n, 0.0));
  for (size_t i=0;i<n;i++)
    for (size_t j=0;j<n;j++) {
      double s=0.0;
      for (size_t k=0;k<n;k++) s += A[i][k]*B[k][j];
      C[i][j]=s;
    }
  return C;
}

static double maxDiffFromIdentity(const vector< vector<double> >& A)
{
  double d = 0.0;
  for (size_t i=0;i<A.size();i++)
    for (size_t j=0;j<A.size();j++)
      d = std::max(d, fabs(A[i][j] - ((i==j)?1.0:0.0)));
  return d;
}

int main()
{
  const JCode* code = CodeFactory::lookup("Gaussian-16");
  check(code != 0, "Gaussian-16 JCode loaded");
  if (!code) return 1;

  TGBSAngFunc* cart = code->getAngFunc(TGaussianBasisSet::Cartesian);
  check(cart != 0, "Cartesian angular table loaded");
  if (!cart) return 1;

  SymOp I; for (int i=0;i<3;i++) for(int j=0;j<3;j++) I.m[i][j]=(i==j)?1.0:0.0;

  //  Identity, every shell type this table has.
  for (int shell = 0; shell <= 2; shell++) {
    vector< vector<double> > D;
    bool ok = ShellRotation::buildD(shell, cart, I, 1.0e-6, D);
    char label[64]; snprintf(label, sizeof(label), "shell %d: identity op", shell);
    check(ok, label);
    if (ok) {
      char m2[96]; snprintf(m2, sizeof(m2),
        "shell %d: D(identity) == identity matrix (max diff %.2e)",
        shell, maxDiffFromIdentity(D));
      check(maxDiffFromIdentity(D) < 1.0e-9, m2);
    }
  }

  //  p shell against the closed form derived from R and the table's
  //  OWN reported order (not assumed x,y,z).
  {
    int axisOf[3];
    bool orderOk = true;
    for (int deg = 0; deg < 3; deg++) {
      AngMomFunc f = cart->getFunc(1, deg);
      if (f.size() != 1) { orderOk = false; break; }
      if (f[0].m_l == 1) axisOf[deg] = 0;
      else if (f[0].m_m == 1) axisOf[deg] = 1;
      else if (f[0].m_n == 1) axisOf[deg] = 2;
      else { orderOk = false; break; }
    }
    check(orderOk, "p shell: each of the 3 functions is a single Cartesian axis");

    SymOp R = rotationAboutZ(53.0);
    vector< vector<double> > D;
    bool ok = ShellRotation::buildD(1, cart, R, 1.0e-6, D);
    check(ok, "p shell: buildD succeeded for a 53deg z-rotation");
    if (ok && orderOk) {
      double maxDiff = 0.0;
      for (int j = 0; j < 3; j++)
        for (int i = 0; i < 3; i++) {
          double expected = R.m[axisOf[j]][axisOf[i]];
          maxDiff = std::max(maxDiff, fabs(D[j][i]-expected));
        }
      char msg[120];
      snprintf(msg, sizeof(msg), "p shell: D matches the closed form from R "
               "and the table's own axis order (max diff %.2e)", maxDiff);
      check(maxDiff < 1.0e-9, msg);
    }
  }

  //  Group closure: a fourfold rotation's D, applied 4 times, must be
  //  the identity -- for every shell type, including d, where no
  //  hand-derived closed form is used here.
  for (int shell = 0; shell <= 2; shell++) {
    SymOp C4 = rotationAboutZ(90.0);
    vector< vector<double> > D;
    bool ok = ShellRotation::buildD(shell, cart, C4, 1.0e-6, D);
    char label[80]; snprintf(label, sizeof(label),
      "shell %d: buildD succeeded for a 90deg (C4) rotation", shell);
    check(ok, label);
    if (ok) {
      vector< vector<double> > D2 = matmul(D, D);
      vector< vector<double> > D4 = matmul(D2, D2);
      double diff = maxDiffFromIdentity(D4);
      char msg[120]; snprintf(msg, sizeof(msg),
        "shell %d: D(C4)^4 == identity (group closure, max diff %.2e)",
        shell, diff);
      check(diff < 1.0e-8, msg);
    }
  }

  //  Same, for a mirror (order 2) and a threefold (order 3), and for
  //  the SPHERICAL table too (a different code path inside buildD via
  //  getoddNormalize's non-1.0 branch for d).
  {
    SymOp sigma = mirrorXY();
    vector< vector<double> > D;
    bool ok = ShellRotation::buildD(2, cart, sigma, 1.0e-6, D);
    check(ok, "d shell (Cartesian): buildD succeeded for a mirror");
    if (ok) {
      vector< vector<double> > D2 = matmul(D, D);
      double diff = maxDiffFromIdentity(D2);
      char msg[120]; snprintf(msg, sizeof(msg),
        "d shell (Cartesian): D(sigma)^2 == identity (max diff %.2e)", diff);
      check(diff < 1.0e-8, msg);
    }
  }

  TGBSAngFunc* sph = code->getAngFunc(TGaussianBasisSet::Spherical);
  check(sph != 0, "Spherical angular table loaded");
  if (sph) {
    SymOp C3;
    { double t = 120.0*M_PI/180.0;
      C3.m[0][0]=cos(t); C3.m[0][1]=-sin(t); C3.m[0][2]=0;
      C3.m[1][0]=sin(t); C3.m[1][1]= cos(t); C3.m[1][2]=0;
      C3.m[2][0]=0; C3.m[2][1]=0; C3.m[2][2]=1; }
    vector< vector<double> > D;
    bool ok = ShellRotation::buildD(2, sph, C3, 1.0e-6, D);
    check(ok, "d shell (spherical): buildD succeeded for a 120deg rotation");
    if (ok) {
      vector< vector<double> > D2 = matmul(D, D);
      vector< vector<double> > D3 = matmul(D2, D);
      double diff = maxDiffFromIdentity(D3);
      char msg[120]; snprintf(msg, sizeof(msg),
        "d shell (spherical): D(C3)^3 == identity (max diff %.2e)", diff);
      check(diff < 1.0e-8, msg);
    }
    delete sph;
  }

  delete cart;

  printf("%s\n", bad ? "FAIL" : "PASS");
  return bad ? 1 : 0;
}
