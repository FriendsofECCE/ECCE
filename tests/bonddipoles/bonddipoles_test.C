// Bond dipoles from atomic charges (#849): the bond sum is sum q r exactly.
#include "tdat/BondDipoles.H"
#include <cmath>
#include <cstdio>
using namespace std;
typedef vector<pair<int,int> > Bonds;
static int fails = 0;

static void check(bool ok, const char* what)
{
  printf("%s %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) fails++;
}

static bool near(double a, double b, double tol = 1e-9)
{
  return fabs(a - b) < tol;
}

int main()
{
  const double o[3] = {0, 0, 0};
  {
    // H2O: q_H = 0.4, q_O = -0.8; the bond charges are fixed by the graph.
    vector<double> xyz = {0,0,0.1173, 0,0.7572,-0.4692, 0,-0.7572,-0.4692};
    vector<double> q = {-0.8, 0.4, 0.4};
    Bonds b = {{0,1},{0,2}};
    BondDipoles::Result r = BondDipoles::compute(q, xyz, b, o);
    check(near(r.delta[0], -0.4) && near(r.delta[1], -0.4),
          "H2O: d(O<-H) = -0.4 on each bond (unique)");
    double s = 0;
    for (int k = 0; k < 3; k++)
      s += fabs(r.bondSum[k] - r.pointCharge[k]);
    check(s < 1e-12, "H2O: bonds sum to the point-charge dipole");
    check(r.mu[2][0] < 0 && r.mu[1][0] > 0 && r.mu[1][1] < 0,
          "H2O: each bond dipole points from O (negative) to H (positive)");
    check(r.residual < 1e-12, "H2O: charges reproduced by the bond transfers");
  }
  {
    // Six-ring with alternating charges, a ring of atoms plus an off-ring
    // charge pair so the sum is not trivially zero.
    vector<double> xyz, q;
    Bonds b;
    for (int i = 0; i < 6; i++) {
      xyz.push_back(1.4*cos(i*M_PI/3));
      xyz.push_back(1.4*sin(i*M_PI/3));
      xyz.push_back(0.1*i);
      q.push_back(i % 2 ? 0.15 : -0.15);
      b.push_back({i, (i + 1) % 6});
    }
    q[0] = -0.3; q[1] = 0.3;     // net zero, uneven
    BondDipoles::Result r = BondDipoles::compute(q, xyz, b, o);
    double s = 0;
    for (int k = 0; k < 3; k++) s += fabs(r.bondSum[k] - r.pointCharge[k]);
    check(s < 1e-9, "ring: bonds sum to the point-charge dipole");
    check(r.residual < 1e-9, "ring: charges reproduced (minimum-norm solve)");
    // Minimum norm: the transfers carry no circulation round the ring.
    double circ = 0;
    for (int e = 0; e < 6; e++) circ += r.delta[e];
    check(near(circ, 0.0, 1e-9), "ring: no circulating current (minimum norm)");
  }
  {
    // Charged ion: bonds leave the net-charge term out, point sum uses origin.
    vector<double> xyz = {0,0,0, 1,0,0, 2,0,0};
    vector<double> q = {0.5, 0.2, 0.3};
    Bonds b = {{0,1},{1,2}};
    const double c[3] = {1, 0, 0};
    BondDipoles::Result r = BondDipoles::compute(q, xyz, b, c);
    double t = 0;
    for (int k = 0; k < 3; k++)
      t += fabs(r.bondSum[k] + r.netChargeTerm[k] - r.pointCharge[k]);
    check(t < 1e-12, "ion: bonds + net-charge term = point-charge sum");
  }
  printf(fails ? "FAILED\n" : "all passed\n");
  return fails ? 1 : 0;
}
