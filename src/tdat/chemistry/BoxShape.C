#include "tdat/BoxShape.H"

#include <cmath>

namespace boxshape {

static bool near3(const Vec& v, double x, double y, double z, double tol)
{
  return std::fabs(v[0]-x) < tol && std::fabs(v[1]-y) < tol &&
         std::fabs(v[2]-z) < tol;
}

bool isGromacsCompactBox(const Vec& a, const Vec& b, const Vec& c)
{
  double d = a[0];
  if (d <= 0.0) return false;
  // Tolerance covers the 0.001 A / 0.001 degree rounding of a CRYST1 record.
  double t = 3e-3 * d;
  if (!near3(a, d, 0, 0, t)) return false;
  const double s2 = std::sqrt(2.0), s3 = std::sqrt(3.0), s6 = std::sqrt(6.0);
  if (near3(b, 0, d, 0, t) && near3(c, d/2, d/2, d*s2/2, t)) return true;
  if (near3(b, d/2, d*s3/2, 0, t) && near3(c, d/2, d*s3/6, d*s6/3, t)) return true;
  if (near3(b, d/3, 2*d*s2/3, 0, t) && near3(c, -d/3, d*s2/3, d*s6/3, t)) return true;
  return false;
}

static double det3(const double m[3][3])
{
  return m[0][0]*(m[1][1]*m[2][2]-m[1][2]*m[2][1])
       - m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0])
       + m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0]);
}

bool wignerSeitz(const Vec& a, const Vec& b, const Vec& c, Shape& out)
{
  out.vertices.clear();
  out.edges.clear();
  out.nfaces = 0;

  // Bisector planes p.x = |p|^2/2 of the lattice points around the origin.
  std::vector<Vec> P;
  for (int i=-2; i<=2; i++) for (int j=-2; j<=2; j++) for (int k=-2; k<=2; k++) {
    if (!i && !j && !k) continue;
    Vec p;
    for (int x=0; x<3; x++) p[x] = i*a[x] + j*b[x] + k*c[x];
    P.push_back(p);
  }
  const int n = (int)P.size();
  std::vector<double> rhs(n);
  double scale = 0.0;
  for (int i=0; i<n; i++) {
    rhs[i] = 0.5*(P[i][0]*P[i][0]+P[i][1]*P[i][1]+P[i][2]*P[i][2]);
    if (rhs[i] > scale) scale = rhs[i];
  }
  const double eps = 1e-7 * scale;   // plane-distance tolerance
  const double merge = 1e-5 * std::sqrt(scale);

  std::vector<std::vector<int> > vplanes;   // planes through each vertex
  for (int i=0; i<n; i++) for (int j=i+1; j<n; j++) for (int k=j+1; k<n; k++) {
    double m[3][3] = {{P[i][0],P[i][1],P[i][2]},{P[j][0],P[j][1],P[j][2]},
                      {P[k][0],P[k][1],P[k][2]}};
    double D = det3(m);
    if (std::fabs(D) < 1e-9 * scale * std::sqrt(scale)) continue;
    Vec v;
    for (int col=0; col<3; col++) {
      double t[3][3];
      for (int r=0; r<3; r++) for (int cc=0; cc<3; cc++) t[r][cc] = m[r][cc];
      t[0][col] = rhs[i]; t[1][col] = rhs[j]; t[2][col] = rhs[k];
      v[col] = det3(t)/D;
    }
    bool inside = true;
    std::vector<int> on;
    for (int q=0; q<n && inside; q++) {
      double s = P[q][0]*v[0]+P[q][1]*v[1]+P[q][2]*v[2] - rhs[q];
      if (s > eps*10) inside = false;
      else if (std::fabs(s) <= eps*10) on.push_back(q);
    }
    if (!inside) continue;
    bool dup = false;
    for (size_t u=0; u<out.vertices.size(); u++) {
      const Vec& w = out.vertices[u];
      if (std::fabs(w[0]-v[0])<merge && std::fabs(w[1]-v[1])<merge &&
          std::fabs(w[2]-v[2])<merge) { dup = true; break; }
    }
    if (dup) continue;
    out.vertices.push_back(v);
    vplanes.push_back(on);
  }
  if (out.vertices.size() < 4) return false;

  // Two vertices on two common planes lie on one edge of the convex cell.
  std::vector<int> planeCount(n, 0);
  for (size_t u=0; u<vplanes.size(); u++)
    for (size_t q=0; q<vplanes[u].size(); q++) planeCount[vplanes[u][q]]++;
  for (int q=0; q<n; q++) if (planeCount[q] >= 3) out.nfaces++;
  for (size_t u=0; u<vplanes.size(); u++)
    for (size_t w=u+1; w<vplanes.size(); w++) {
      int common = 0;
      for (size_t x=0; x<vplanes[u].size(); x++)
        for (size_t y=0; y<vplanes[w].size(); y++)
          if (vplanes[u][x] == vplanes[w][y]) common++;
      if (common >= 2) out.edges.push_back(std::make_pair((int)u,(int)w));
    }
  return true;
}

}
