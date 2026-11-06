#include "tdat/BondDipoles.H"
#include <algorithm>
#include <cmath>

using std::vector;

namespace BondDipoles {

namespace {

// Connected component label per atom.
int components(int n, const vector<std::pair<int,int> >& bonds,
               vector<int>& comp)
{
  vector<int> parent(n);
  for (int i = 0; i < n; i++) parent[i] = i;
  auto find = [&](int a) {
    while (parent[a] != a) a = parent[a] = parent[parent[a]];
    return a;
  };
  for (const auto& b : bonds) parent[find(b.first)] = find(b.second);
  comp.assign(n, -1);
  vector<int> id(n, -1);
  int nc = 0;
  for (int i = 0; i < n; i++) {
    int r = find(i);
    if (id[r] < 0) id[r] = nc++;
    comp[i] = id[r];
  }
  return nc;
}

// Conjugate gradients on L phi = b; L is singular but b sums to zero in each
// component, so the iteration stays in the consistent subspace.
void solveLaplacian(int n, const vector<std::pair<int,int> >& bonds,
                    const vector<double>& b, vector<double>& phi)
{
  auto apply = [&](const vector<double>& x, vector<double>& y) {
    y.assign(n, 0.0);
    for (const auto& e : bonds) {
      double d = x[e.first] - x[e.second];
      y[e.first] += d;
      y[e.second] -= d;
    }
  };
  phi.assign(n, 0.0);
  vector<double> r = b, p = b, Ap;
  double rr = 0.0, bb = 0.0;
  for (int i = 0; i < n; i++) rr += r[i]*r[i];
  bb = rr;
  for (int it = 0; it < 10*n + 100 && rr > 1e-28*(bb > 0 ? bb : 1.0); it++) {
    apply(p, Ap);
    double pAp = 0.0;
    for (int i = 0; i < n; i++) pAp += p[i]*Ap[i];
    if (pAp <= 0.0) break;
    double a = rr/pAp, rr2 = 0.0;
    for (int i = 0; i < n; i++) { phi[i] += a*p[i]; r[i] -= a*Ap[i]; rr2 += r[i]*r[i]; }
    for (int i = 0; i < n; i++) p[i] = r[i] + (rr2/rr)*p[i];
    rr = rr2;
  }
}

}

Result compute(const vector<double>& q, const vector<double>& xyz,
               const vector<std::pair<int,int> >& bonds,
               const double origin[3])
{
  const int n = (int)q.size();
  Result res;
  for (int k = 0; k < 3; k++) {
    res.bondSum[k] = res.netChargeTerm[k] = res.pointCharge[k] = 0.0;
  }
  vector<int> comp;
  int nc = components(n, bonds, comp);
  vector<double> Q(nc, 0.0), cx[3];
  vector<int> cnt(nc, 0);
  for (int k = 0; k < 3; k++) cx[k].assign(nc, 0.0);
  for (int i = 0; i < n; i++) {
    Q[comp[i]] += q[i];
    cnt[comp[i]]++;
    for (int k = 0; k < 3; k++) cx[k][comp[i]] += xyz[3*i + k];
  }
  vector<double> qp(n);
  for (int i = 0; i < n; i++) qp[i] = q[i] - Q[comp[i]]/cnt[comp[i]];
  for (int c = 0; c < nc; c++)
    for (int k = 0; k < 3; k++)
      res.netChargeTerm[k] += Q[c]*(cx[k][c]/cnt[c] - origin[k]);
  for (int i = 0; i < n; i++)
    for (int k = 0; k < 3; k++)
      res.pointCharge[k] += q[i]*(xyz[3*i + k] - origin[k]);

  vector<double> phi;
  solveLaplacian(n, bonds, qp, phi);
  const int m = (int)bonds.size();
  res.delta.resize(m);
  for (int k = 0; k < 3; k++) res.mu[k].resize(m);
  vector<double> got(n, 0.0);
  for (int e = 0; e < m; e++) {
    int i = bonds[e].first, j = bonds[e].second;
    double d = phi[i] - phi[j];
    res.delta[e] = d;
    got[i] += d;
    got[j] -= d;
    for (int k = 0; k < 3; k++) {
      res.mu[k][e] = d*(xyz[3*i + k] - xyz[3*j + k]);
      res.bondSum[k] += res.mu[k][e];
    }
  }
  res.residual = 0.0;
  for (int i = 0; i < n; i++)
    res.residual = std::max(res.residual, std::fabs(qp[i] - got[i]));
  return res;
}

}
