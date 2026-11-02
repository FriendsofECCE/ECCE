#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>

#include "tdat/CoordCombine.H"

using std::string;
using std::vector;

namespace {

string lower(string s)
{
  for (char& c : s) c = (char)tolower((unsigned char)c);
  return s;
}

// Eigenvector of the largest eigenvalue of a symmetric 4x4 (cyclic Jacobi).
// The largest algebraic eigenvalue is what makes the fit a proper rotation.
void largestEigenvector(double a[4][4], double q[4])
{
  double v[4][4] = {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};
  for (int sweep = 0; sweep < 60; sweep++) {
    double off = 0;
    for (int i = 0; i < 4; i++)
      for (int j = i + 1; j < 4; j++) off += a[i][j] * a[i][j];
    if (off < 1e-30) break;
    for (int p = 0; p < 3; p++)
      for (int r = p + 1; r < 4; r++) {
        if (std::fabs(a[p][r]) < 1e-300) continue;
        double th = (a[r][r] - a[p][p]) / (2 * a[p][r]);
        double t = (th >= 0 ? 1 : -1) / (std::fabs(th) + std::sqrt(th * th + 1));
        double c = 1 / std::sqrt(t * t + 1), s = t * c;
        for (int k = 0; k < 4; k++) {
          double akp = a[k][p], akr = a[k][r];
          a[k][p] = c * akp - s * akr;
          a[k][r] = s * akp + c * akr;
        }
        for (int k = 0; k < 4; k++) {
          double apk = a[p][k], ark = a[r][k];
          a[p][k] = c * apk - s * ark;
          a[r][k] = s * apk + c * ark;
        }
        for (int k = 0; k < 4; k++) {
          double vkp = v[k][p], vkr = v[k][r];
          v[k][p] = c * vkp - s * vkr;
          v[k][r] = s * vkp + c * vkr;
        }
      }
  }
  int best = 0;
  for (int i = 1; i < 4; i++) if (a[i][i] > a[best][best]) best = i;
  for (int k = 0; k < 4; k++) q[k] = v[k][best];
}

}

bool CoordCombine::parseSet(const string& spec, int n, vector<int>& out,
                            string& err)
{
  std::set<int> got;
  size_t i = 0;
  while (i < spec.size()) {
    size_t e = spec.find(',', i);
    if (e == string::npos) e = spec.size();
    string tok;
    for (size_t k = i; k < e; k++)
      if (!isspace((unsigned char)spec[k])) tok += spec[k];
    i = e + 1;
    if (tok.empty()) continue;
    int lo, hi;
    char *end;
    lo = (int)strtol(tok.c_str(), &end, 10);
    hi = lo;
    if (*end == '-') hi = (int)strtol(end + 1, &end, 10);
    if (*end || end == tok.c_str() || hi < lo) {
      err = "cannot read \"" + tok + "\" in the atom list";
      return false;
    }
    if (lo < 1 || hi > n) {
      char buf[96];
      snprintf(buf, sizeof buf, "atom %d is outside 1..%d", lo < 1 ? lo : hi, n);
      err = buf;
      return false;
    }
    for (int a = lo; a <= hi; a++) got.insert(a - 1);
  }
  out.assign(got.begin(), got.end());
  if (out.empty()) { err = "the atom list is empty"; return false; }
  return true;
}

bool CoordCombine::combine(const vector<string>& elemA, const vector<double>& A,
                           const vector<string>& elemB, const vector<double>& B,
                           const vector<int>& S, bool align, Result& out)
{
  out.xyz.clear();
  out.rmsd = 0;
  out.aligned = false;
  const int n = (int)elemA.size();
  char buf[160];
  if ((int)elemB.size() != n || (int)A.size() != 3 * n || (int)B.size() != 3 * n) {
    snprintf(buf, sizeof buf, "the structures have %d and %d atoms; they must "
             "have the same number", n, (int)elemB.size());
    out.error = buf;
    return false;
  }
  for (int i = 0; i < n; i++)
    if (lower(elemA[i]) != lower(elemB[i])) {
      snprintf(buf, sizeof buf, "atom %d is %s here and %s in the other "
               "structure", i + 1, elemA[i].c_str(), elemB[i].c_str());
      out.error = buf;
      return false;
    }
  if (S.empty()) { out.error = "no atoms chosen"; return false; }
  std::vector<char> inS(n, 0);
  for (int s : S) {
    if (s < 0 || s >= n) { out.error = "an atom of the list is out of range"; return false; }
    inS[s] = 1;
  }
  vector<double> Bf = B;
  if (align) {
    vector<int> rest;
    for (int i = 0; i < n; i++) if (!inS[i]) rest.push_back(i);
    if (rest.size() < 3) {
      out.error = "Align needs at least three atoms outside the chosen set";
      return false;
    }
    double ca[3] = {0,0,0}, cb[3] = {0,0,0};
    for (int i : rest) for (int k = 0; k < 3; k++) {
      ca[k] += A[3*i+k]; cb[k] += B[3*i+k];
    }
    for (int k = 0; k < 3; k++) { ca[k] /= rest.size(); cb[k] /= rest.size(); }
    double S3[3][3] = {{0}};
    for (int i : rest)
      for (int p = 0; p < 3; p++)
        for (int r = 0; r < 3; r++)
          S3[p][r] += (B[3*i+p] - cb[p]) * (A[3*i+r] - ca[r]);
    double N[4][4] = {
      {S3[0][0]+S3[1][1]+S3[2][2], S3[1][2]-S3[2][1], S3[2][0]-S3[0][2], S3[0][1]-S3[1][0]},
      {S3[1][2]-S3[2][1], S3[0][0]-S3[1][1]-S3[2][2], S3[0][1]+S3[1][0], S3[2][0]+S3[0][2]},
      {S3[2][0]-S3[0][2], S3[0][1]+S3[1][0], -S3[0][0]+S3[1][1]-S3[2][2], S3[1][2]+S3[2][1]},
      {S3[0][1]-S3[1][0], S3[2][0]+S3[0][2], S3[1][2]+S3[2][1], -S3[0][0]-S3[1][1]+S3[2][2]}};
    double q[4];
    largestEigenvector(N, q);
    double nq = std::sqrt(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3]);
    for (int k = 0; k < 4; k++) q[k] /= nq;
    const double w = q[0], x = q[1], y = q[2], z = q[3];
    const double R[3][3] = {
      {1-2*(y*y+z*z), 2*(x*y-w*z), 2*(x*z+w*y)},
      {2*(x*y+w*z), 1-2*(x*x+z*z), 2*(y*z-w*x)},
      {2*(x*z-w*y), 2*(y*z+w*x), 1-2*(x*x+y*y)}};
    for (int i = 0; i < n; i++)
      for (int p = 0; p < 3; p++) {
        double s = ca[p];
        for (int r = 0; r < 3; r++) s += R[p][r] * (B[3*i+r] - cb[r]);
        Bf[3*i+p] = s;
      }
    double sum = 0;
    for (int i : rest)
      for (int k = 0; k < 3; k++) {
        double d = Bf[3*i+k] - A[3*i+k];
        sum += d * d;
      }
    out.rmsd = std::sqrt(sum / rest.size());
    out.aligned = true;
  }
  out.xyz = A;
  for (int s : S) for (int k = 0; k < 3; k++) out.xyz[3*s+k] = Bf[3*s+k];
  return true;
}
