// CoordCombine: A with some atoms taken from a rotated, partly moved B;
// element mismatches are refused by name (#620).
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
using namespace std;
#include "tdat/CoordCombine.H"

static int fails = 0;
static void check(bool ok, const char *what)
{
  printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) fails++;
}

int main()
{
  // A cluster of six atoms and a seventh, the "counter-ion".
  const double xyz[7][3] = {{0,0,0},{1.5,0.2,0},{-0.4,1.4,0.3},{0.1,-0.5,1.6},
                            {1.1,1.2,1.0},{-1.2,-0.8,-0.7},{4.0,1.0,-2.0}};
  const char *el[7] = {"Fe","O","O","N","C","H","Na"};
  vector<string> E(el, el + 7);
  vector<double> A, B;
  for (int i = 0; i < 7; i++) for (int k = 0; k < 3; k++) A.push_back(xyz[i][k]);
  // B: A moved by a proper rotation (about a skew axis) and a shift ...
  const double w = 0.7, x = 0.3, y = -0.5, z = 0.4;
  double nq = sqrt(w*w + x*x + y*y + z*z);
  const double q[4] = {w/nq, x/nq, y/nq, z/nq};
  const double R[3][3] = {
    {1-2*(q[2]*q[2]+q[3]*q[3]), 2*(q[1]*q[2]-q[0]*q[3]), 2*(q[1]*q[3]+q[0]*q[2])},
    {2*(q[1]*q[2]+q[0]*q[3]), 1-2*(q[1]*q[1]+q[3]*q[3]), 2*(q[2]*q[3]-q[0]*q[1])},
    {2*(q[1]*q[3]-q[0]*q[2]), 2*(q[2]*q[3]+q[0]*q[1]), 1-2*(q[1]*q[1]+q[2]*q[2])}};
  const double t[3] = {3.0, -2.0, 5.0};
  vector<double> trueB(21);   // B atoms where the fit puts them
  for (int i = 0; i < 7; i++)
    for (int p = 0; p < 3; p++) {
      double s = t[p];
      for (int r = 0; r < 3; r++) s += R[p][r] * A[3*i+r];
      B.push_back(s);
    }
  // ... and then atoms 5 and 7 (1-based) moved in B's own frame.
  B[3*4+0] += 0.5; B[3*4+2] -= 0.3;
  B[3*6+1] += 1.0; B[3*6+2] += 2.0;
  vector<int> S = {4, 6};   // 0-based: atoms 5 and 7

  CoordCombine::Result r;
  check(CoordCombine::combine(E, A, E, B, S, true, r), "aligned combine runs");
  check(r.aligned && r.rmsd < 1e-6, "fit of the unmoved atoms is exact");
  // Expected: B's S atoms taken back through the inverse transform.
  bool same = true, moved = true;
  for (int i = 0; i < 7; i++) {
    bool inS = (i == 4 || i == 6);
    double d = 0;
    for (int k = 0; k < 3; k++) d += fabs(r.xyz[3*i+k] - A[3*i+k]);
    if (!inS && d > 1e-6) same = false;
    if (inS && d < 0.1) moved = false;
  }
  check(same, "atoms outside S equal A to 1e-6");
  check(moved, "atoms in S differ from A");
  // S atoms: R^T (b - t) must equal the aligned position; check by the
  // inverse transform of the moved displacement.
  double e5[3];
  for (int p = 0; p < 3; p++) {
    e5[p] = 0;
    for (int rr = 0; rr < 3; rr++) e5[p] += R[rr][p] * (B[3*4+rr] - t[rr]);
  }
  double dev = 0;
  for (int k = 0; k < 3; k++) dev += fabs(r.xyz[3*4+k] - e5[k]);
  check(dev < 1e-6, "S atom 5 equals B's aligned position to 1e-6");

  CoordCombine::Result n;
  check(CoordCombine::combine(E, A, E, B, S, false, n) &&
        fabs(n.xyz[3*4] - B[3*4]) < 1e-12 && fabs(n.xyz[0] - A[0]) < 1e-12,
        "without align the S atoms are B's raw coordinates");

  // A mirror image is not fitted away: the rest must not reach rmsd 0.
  vector<double> M = A;
  for (int i = 0; i < 7; i++) M[3*i] = -M[3*i];
  CoordCombine::Result m;
  CoordCombine::combine(E, A, E, M, S, true, m);
  check(m.rmsd > 0.05, "a mirror image is not reported as a perfect fit");

  vector<string> E2 = E;
  E2[3] = "C";
  CoordCombine::Result bad;
  bool ok = CoordCombine::combine(E, A, E2, B, S, true, bad);
  check(!ok && bad.error.find("atom 4") != string::npos &&
        bad.error.find("N") != string::npos && bad.error.find("C") != string::npos,
        "different elements are refused naming the atom");
  vector<double> B6(B.begin(), B.begin() + 18);
  vector<string> E6(E.begin(), E.begin() + 6);
  check(!CoordCombine::combine(E, A, E6, B6, S, false, bad),
        "different atom counts are refused");

  vector<int> set;
  string err;
  check(CoordCombine::parseSet("1-3, 7,2", 7, set, err) && set.size() == 4 &&
        set[0] == 0 && set[3] == 6, "atom list ranges parse");
  check(!CoordCombine::parseSet("1-9", 7, set, err), "out-of-range atom refused");
  check(!CoordCombine::parseSet("a", 7, set, err), "junk atom list refused");
  printf(fails ? "FAILED\n" : "PASSED\n");
  return fails ? 1 : 0;
}
