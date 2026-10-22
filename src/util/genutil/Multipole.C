#include <cmath>
#include <cctype>
#include <map>
#include <algorithm>

#include "util/Multipole.H"

using std::vector;
using std::string;

namespace {

int axis(char c)
{
  c = (char)tolower((unsigned char)c);
  return c == 'x' ? 0 : c == 'y' ? 1 : c == 'z' ? 2 : -1;
}

int flat(int rank, const int *idx)
{
  int k = 0;
  for (int i = 0; i < rank; i++) k = k*3 + idx[i];
  return k;
}

int power3(int rank) { int n = 1; for (int i = 0; i < rank; i++) n *= 3; return n; }

void gaussLegendre(int n, vector<double>& x, vector<double>& w)
{
  x.resize(n); w.resize(n);
  for (int i = 0; i < n; i++) {
    double z = cos(M_PI*(i + 0.75)/(n + 0.5)), pp = 1.0;
    for (int it = 0; it < 100; it++) {
      double p1 = 1.0, p2 = 0.0;
      for (int j = 0; j < n; j++) {
        double p3 = p2; p2 = p1;
        p1 = ((2.0*j + 1.0)*z*p2 - j*p3)/(j + 1.0);
      }
      pp = n*(z*p1 - p2)/(z*z - 1.0);
      double dz = p1/pp;
      z -= dz;
      if (fabs(dz) < 1e-15) break;
    }
    x[i] = z;
    w[i] = 2.0/((1.0 - z*z)*pp*pp);
  }
}

double legendre(int l, double x)
{
  double p0 = 1.0, p1 = x;
  if (l == 0) return p0;
  for (int k = 2; k <= l; k++) {
    double p2 = ((2.0*k - 1.0)*x*p1 - (k - 1.0)*p0)/k;
    p0 = p1; p1 = p2;
  }
  return p1;
}

}

namespace Multipole {

bool fromComponents(int rank, const vector<string>& labels,
                    const vector<double>& values, vector<double>& full)
{
  if (rank < 1 || rank > 6 || labels.size() != values.size()) return false;
  full.assign(power3(rank), 0.0);
  vector<bool> seen(full.size(), false);
  for (size_t c = 0; c < labels.size(); c++) {
    if ((int)labels[c].size() != rank) return false;
    int idx[8];
    for (int i = 0; i < rank; i++) {
      idx[i] = axis(labels[c][i]);
      if (idx[i] < 0) return false;
    }
    std::sort(idx, idx + rank);
    do {
      int k = flat(rank, idx);
      full[k] = values[c];
      seen[k] = true;
    } while (std::next_permutation(idx, idx + rank));
  }
  for (size_t k = 0; k < seen.size(); k++) if (!seen[k]) return false;
  return true;
}

double evaluate(int rank, const vector<double>& full, const double n[3])
{
  vector<double> cur(full);
  for (int r = rank; r >= 1; r--) {
    vector<double> nxt(cur.size()/3, 0.0);
    for (size_t k = 0; k < nxt.size(); k++)
      nxt[k] = cur[3*k]*n[0] + cur[3*k + 1]*n[1] + cur[3*k + 2]*n[2];
    cur.swap(nxt);
  }
  return cur[0];
}

vector<double> harmonicPart(int rank, const vector<double>& full)
{
  // f - sum_l P_l f over l < rank of the same parity, with the projection
  // (2l+1)/(4 pi) int P_l(n.m) f(m) dm done by a quadrature exact for these
  // polynomials; the remainder is a harmonic polynomial of degree `rank`,
  // whose tensor is recovered by a fit in the degree-`rank` monomials.
  const int NT = 12, NP = 24;
  vector<double> xt, wt;
  gaussLegendre(NT, xt, wt);
  vector<double> m, w, fm;
  for (int i = 0; i < NT; i++) {
    double st = sqrt(1.0 - xt[i]*xt[i]);
    for (int j = 0; j < NP; j++) {
      double ph = 2.0*M_PI*(j + 0.5)/NP;
      double v[3] = { st*cos(ph), st*sin(ph), xt[i] };
      m.push_back(v[0]); m.push_back(v[1]); m.push_back(v[2]);
      w.push_back(wt[i]*2.0*M_PI/NP);
      fm.push_back(evaluate(rank, full, v));
    }
  }
  const int n = (int)w.size();
  vector<double> h(n);
  for (int p = 0; p < n; p++) {
    double v = fm[p];
    for (int l = rank - 2; l >= 0; l -= 2) {
      double acc = 0.0;
      for (int q = 0; q < n; q++) {
        double d = m[3*p]*m[3*q] + m[3*p + 1]*m[3*q + 1] + m[3*p + 2]*m[3*q + 2];
        acc += w[q]*legendre(l, d)*fm[q];
      }
      v -= (2.0*l + 1.0)/(4.0*M_PI)*acc;
    }
    h[p] = v;
  }
  vector<int> ea, eb, ec;
  for (int a = rank; a >= 0; a--)
    for (int b = rank - a; b >= 0; b--) {
      ea.push_back(a); eb.push_back(b); ec.push_back(rank - a - b);
    }
  const int D = (int)ea.size();
  vector<double> A(D*D, 0.0), coef(D, 0.0);
  for (int p = 0; p < n; p++) {
    vector<double> row(D);
    for (int d = 0; d < D; d++)
      row[d] = pow(m[3*p], ea[d])*pow(m[3*p + 1], eb[d])*pow(m[3*p + 2], ec[d]);
    for (int d = 0; d < D; d++) {
      coef[d] += w[p]*row[d]*h[p];
      for (int e = 0; e < D; e++) A[d*D + e] += w[p]*row[d]*row[e];
    }
  }
  for (int c = 0; c < D; c++) {
    int piv = c;
    for (int r = c + 1; r < D; r++)
      if (fabs(A[r*D + c]) > fabs(A[piv*D + c])) piv = r;
    for (int e = 0; e < D; e++) std::swap(A[c*D + e], A[piv*D + e]);
    std::swap(coef[c], coef[piv]);
    for (int r = c + 1; r < D; r++) {
      double fct = A[r*D + c]/A[c*D + c];
      for (int e = c; e < D; e++) A[r*D + e] -= fct*A[c*D + e];
      coef[r] -= fct*coef[c];
    }
  }
  for (int c = D - 1; c >= 0; c--) {
    for (int e = c + 1; e < D; e++) coef[c] -= A[c*D + e]*coef[e];
    coef[c] /= A[c*D + c];
  }
  // Each monomial's coefficient is shared evenly by its index orderings.
  vector<double> res(full.size(), 0.0);
  for (size_t k = 0; k < res.size(); k++) {
    int cnt[3] = {0, 0, 0}, t = (int)k;
    for (int i = 0; i < rank; i++) { cnt[t % 3]++; t /= 3; }
    int mult = 1;
    for (int i = 2; i <= rank; i++) mult *= i;
    for (int a = 0; a < 3; a++) for (int i = 2; i <= cnt[a]; i++) mult /= i;
    for (int d = 0; d < D; d++)
      if (ea[d] == cnt[0] && eb[d] == cnt[1] && ec[d] == cnt[2])
        res[k] = coef[d]/mult;
  }
  return res;
}

void icosphere(int level, vector<float>& xyz, vector<int>& tri)
{
  const double t = (1.0 + sqrt(5.0))/2.0;
  const double base[12][3] = {
    {-1, t, 0}, {1, t, 0}, {-1, -t, 0}, {1, -t, 0},
    {0, -1, t}, {0, 1, t}, {0, -1, -t}, {0, 1, -t},
    {t, 0, -1}, {t, 0, 1}, {-t, 0, -1}, {-t, 0, 1}};
  const int faces[20][3] = {
    {0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},
    {1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},
    {3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},
    {4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1}};
  xyz.clear(); tri.clear();
  for (int i = 0; i < 12; i++) {
    double l = sqrt(base[i][0]*base[i][0] + base[i][1]*base[i][1]
                    + base[i][2]*base[i][2]);
    for (int k = 0; k < 3; k++) xyz.push_back((float)(base[i][k]/l));
  }
  for (int f = 0; f < 20; f++)
    for (int k = 0; k < 3; k++) tri.push_back(faces[f][k]);
  for (int s = 0; s < level; s++) {
    std::map<std::pair<int,int>, int> mid;
    vector<int> out;
    for (size_t f = 0; f < tri.size(); f += 3) {
      int v[3] = {tri[f], tri[f + 1], tri[f + 2]}, m[3];
      for (int e = 0; e < 3; e++) {
        int a = v[e], b = v[(e + 1) % 3];
        std::pair<int,int> key(std::min(a, b), std::max(a, b));
        std::map<std::pair<int,int>, int>::iterator it = mid.find(key);
        if (it == mid.end()) {
          double p[3], l = 0.0;
          for (int k = 0; k < 3; k++) {
            p[k] = xyz[3*a + k] + xyz[3*b + k];
            l += p[k]*p[k];
          }
          l = sqrt(l);
          for (int k = 0; k < 3; k++) xyz.push_back((float)(p[k]/l));
          it = mid.insert(std::make_pair(key, (int)xyz.size()/3 - 1)).first;
        }
        m[e] = it->second;
      }
      const int nt[4][3] = {{v[0], m[0], m[2]}, {v[1], m[1], m[0]},
                            {v[2], m[2], m[1]}, {m[0], m[1], m[2]}};
      for (int q = 0; q < 4; q++)
        for (int k = 0; k < 3; k++) out.push_back(nt[q][k]);
    }
    tri.swap(out);
  }
}

Mesh surface(int rank, const vector<double>& full, double scale, int level)
{
  Mesh mesh;
  vector<float> unit;
  icosphere(level, unit, mesh.tri);
  const size_t nv = unit.size()/3;
  mesh.xyz.resize(3*nv); mesh.f.resize(nv);
  for (size_t i = 0; i < nv; i++) {
    double n[3] = {unit[3*i], unit[3*i + 1], unit[3*i + 2]};
    double f = evaluate(rank, full, n);
    mesh.f[i] = (float)f;
    for (int k = 0; k < 3; k++) mesh.xyz[3*i + k] = (float)(scale*fabs(f)*n[k]);
  }
  // Area-weighted vertex normals; at the nodes of f (vertex at the origin)
  // the unit-sphere direction stands in.
  mesh.normal.assign(3*nv, 0.f);
  for (size_t t = 0; t < mesh.tri.size(); t += 3) {
    const int *v = &mesh.tri[t];
    double a[3], b[3], c[3];
    for (int k = 0; k < 3; k++) {
      a[k] = mesh.xyz[3*v[1] + k] - mesh.xyz[3*v[0] + k];
      b[k] = mesh.xyz[3*v[2] + k] - mesh.xyz[3*v[0] + k];
    }
    c[0] = a[1]*b[2] - a[2]*b[1];
    c[1] = a[2]*b[0] - a[0]*b[2];
    c[2] = a[0]*b[1] - a[1]*b[0];
    for (int e = 0; e < 3; e++)
      for (int k = 0; k < 3; k++) mesh.normal[3*v[e] + k] += (float)c[k];
  }
  for (size_t i = 0; i < nv; i++) {
    double l = 0.0;
    for (int k = 0; k < 3; k++) l += mesh.normal[3*i + k]*mesh.normal[3*i + k];
    l = sqrt(l);
    for (int k = 0; k < 3; k++)
      mesh.normal[3*i + k] = l > 1e-20 ? (float)(mesh.normal[3*i + k]/l)
                                       : unit[3*i + k];
  }
  return mesh;
}

double maxAbs(int rank, const vector<double>& full, int level)
{
  vector<float> unit; vector<int> tri;
  icosphere(level, unit, tri);
  double m = 0.0;
  for (size_t i = 0; i < unit.size()/3; i++) {
    double n[3] = {unit[3*i], unit[3*i + 1], unit[3*i + 2]};
    m = std::max(m, fabs(evaluate(rank, full, n)));
  }
  return m;
}

}
