#include <algorithm>
#include <cmath>
#include <cstdio>

#include "tdat/LineProfile.H"

using namespace std;

namespace {
const double BOHR_ANG = 0.52917721092;
const double MARK_TOLERANCE = 0.3;     // Angstrom

//  Plane grid: a Becke-mapped radial rule r = rm u/(1-u) and an even
//  angular rule about each centre.
const int PLANE_RADIAL = 40, PLANE_ANGULAR = 32;
const double PLANE_RM = 0.35;          // Angstrom
const double PLANE_ATOM_REACH = 1.5;   // atoms this close to the plane get a centre

void gaussLegendre(int n, double a, double b, vector<double>& x, vector<double>& w)
{
  x.assign(n, 0.0);
  w.assign(n, 0.0);
  for (int i = 0; i < n; i++) {
    double z = cos(M_PI * (i + 0.75) / (n + 0.5)), dp = 1;
    for (int it = 0; it < 100; it++) {
      double p0 = 1, p1 = 0;
      for (int k = 1; k <= n; k++) { const double p2 = p1; p1 = p0; p0 = ((2*k - 1) * z * p1 - (k - 1) * p2) / k; }
      dp = n * (z * p0 - p1) / (z * z - 1);
      const double dz = p0 / dp;
      z -= dz;
      if (fabs(dz) < 1e-15) break;
    }
    x[i] = 0.5 * (a + b) - 0.5 * (b - a) * z;
    w[i] = (b - a) / ((1 - z * z) * dp * dp);
  }
}
}

namespace LineProfile {

Line throughAtoms(const DensityPlane::Evaluator& ev, int i, int j, double margin,
                  double offset, bool outOfPlane)
{
  Line l;
  double len = 0;
  for (int d = 0; d < 3; d++) {
    l.origin[d] = ev.coordsAng[3*i + d];
    l.unit[d] = ev.coordsAng[3*j + d] - ev.coordsAng[3*i + d];
    len += l.unit[d] * l.unit[d];
  }
  len = sqrt(len);
  for (int d = 0; d < 3; d++) {
    l.unit[d] = len > 0 ? l.unit[d] / len : (d == 2 ? 1.0 : 0.0);
    l.start[d] = l.origin[d] - margin * l.unit[d];
    l.end[d] = l.origin[d] + (len + margin) * l.unit[d];
  }
  if (offset != 0.0) {
    //  In-plane direction: the part of the atom farthest from the line that
    //  is perpendicular to it.
    double a[3] = { 0, 0, 0 }, best = 0;
    const int natoms = (int)ev.symbols.size();
    for (int k = 0; k < natoms; k++) {
      double r[3], along = 0;
      for (int d = 0; d < 3; d++) {
        r[d] = ev.coordsAng[3*k + d] - l.origin[d];
        along += r[d] * l.unit[d];
      }
      double c[3], n2 = 0;
      for (int d = 0; d < 3; d++) { c[d] = r[d] - along * l.unit[d]; n2 += c[d] * c[d]; }
      if (sqrt(n2) > best) {
        best = sqrt(n2);
        for (int d = 0; d < 3; d++) a[d] = c[d] / best;
      }
    }
    if (best < 0.05) {            // linear: any perpendicular
      int e = 0;
      for (int d = 1; d < 3; d++) if (fabs(l.unit[d]) < fabs(l.unit[e])) e = d;
      double ex[3] = { 0, 0, 0 };
      ex[e] = 1.0;
      a[0] = ex[1]*l.unit[2] - ex[2]*l.unit[1];
      a[1] = ex[2]*l.unit[0] - ex[0]*l.unit[2];
      a[2] = ex[0]*l.unit[1] - ex[1]*l.unit[0];
      const double n = sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
      for (int d = 0; d < 3; d++) a[d] /= n;
    }
    const double nrm[3] = { l.unit[1]*a[2] - l.unit[2]*a[1],
                            l.unit[2]*a[0] - l.unit[0]*a[2],
                            l.unit[0]*a[1] - l.unit[1]*a[0] };
    const double *dir = outOfPlane ? nrm : a;
    for (int d = 0; d < 3; d++) {
      l.start[d] += offset * dir[d];
      l.end[d] += offset * dir[d];
    }
  }
  return l;
}

void planeIntegrals(const DensityPlane::Evaluator& ev, RadialDist::PointValues& pv,
                    const double point[3], const double unit[3], double radius,
                    const vector<RadialDist::Spec>& specs, vector<double>& out)
{
  out.assign(specs.size(), 0.0);
  //  Two directions in the plane.
  int e = 0;
  for (int d = 1; d < 3; d++) if (fabs(unit[d]) < fabs(unit[e])) e = d;
  double ex[3] = { 0, 0, 0 };
  ex[e] = 1.0;
  double e1[3] = { ex[1]*unit[2] - ex[2]*unit[1], ex[2]*unit[0] - ex[0]*unit[2],
                   ex[0]*unit[1] - ex[1]*unit[0] };
  const double n1 = sqrt(e1[0]*e1[0] + e1[1]*e1[1] + e1[2]*e1[2]);
  for (int d = 0; d < 3; d++) e1[d] /= n1;
  const double e2[3] = { unit[1]*e1[2] - unit[2]*e1[1], unit[2]*e1[0] - unit[0]*e1[2],
                         unit[0]*e1[1] - unit[1]*e1[0] };
  //  Centres in plane coordinates: the line, then nearby atoms off it.  A
  //  cylinder is one finer grid about the line: an atom's grid would be cut
  //  by the cylinder wall between its points.
  vector<double> cu(1, 0.0), cv(1, 0.0);
  for (size_t a = 0; radius <= 0 && a < ev.symbols.size(); a++) {
    double r[3], h = 0, u = 0, v = 0;
    for (int d = 0; d < 3; d++) r[d] = ev.coordsAng[3*a + d] - point[d];
    for (int d = 0; d < 3; d++) { h += r[d]*unit[d]; u += r[d]*e1[d]; v += r[d]*e2[d]; }
    if (fabs(h) > PLANE_ATOM_REACH) continue;
    bool near = false;
    for (size_t c = 0; c < cu.size(); c++)
      if (hypot(u - cu[c], v - cv[c]) < 0.05) near = true;
    if (!near) { cu.push_back(u); cv.push_back(v); }
  }
  const size_t nc = cu.size();
  //  Radial rule in u = r/(r + rm); a cylinder ends the line's own grid at
  //  its radius, so the cut falls between grid points there.
  vector<double> axX, axW, atX, atW;
  gaussLegendre(PLANE_RADIAL, 0.0, 1.0, atX, atW);
  if (radius > 0) gaussLegendre(2 * PLANE_RADIAL, 0.0, radius / (radius + PLANE_RM), axX, axW);
  else { axX = atX; axW = atW; }
  const int nphi = radius > 0 ? 2 * PLANE_ANGULAR : PLANE_ANGULAR;
  const double dphi = 2 * M_PI / nphi;
  const double toAng3 = pow(BOHR_ANG, -3.0);
  vector<double> dist(nc);
  for (size_t c = 0; c < nc; c++) {
    const vector<double>& X = c == 0 ? axX : atX;
    const vector<double>& W = c == 0 ? axW : atW;
    for (size_t k = 0; k < X.size(); k++) {
      const double u = X[k], r = PLANE_RM * u / (1 - u);
      const double wr = W[k] * PLANE_RM / ((1 - u) * (1 - u)) * r * dphi;
      for (int j = 0; j < nphi; j++) {
        const double phi = (j + 0.5) * dphi;
        const double pu = cu[c] + r * cos(phi), pvv = cv[c] + r * sin(phi);
        //  Becke's fuzzy cells, in two dimensions.
        double w = 1.0;
        if (nc > 1) {
          for (size_t i = 0; i < nc; i++) dist[i] = hypot(pu - cu[i], pvv - cv[i]);
          double sum = 0, mine = 0;
          for (size_t i = 0; i < nc; i++) {
            double P = 1.0;
            for (size_t m = 0; m < nc && P > 0; m++) {
              if (m == i) continue;
              double mu = (dist[i] - dist[m]) / hypot(cu[i] - cu[m], cv[i] - cv[m]);
              for (int it = 0; it < 3; it++) mu = 1.5 * mu - 0.5 * mu * mu * mu;
              P *= 0.5 * (1 - mu);
            }
            sum += P;
            if (i == c) mine = P;
          }
          w = sum > 0 ? mine / sum : 0.0;
        }
        if (w < 1e-14) continue;
        double x[3];
        for (int d = 0; d < 3; d++) x[d] = point[d] + pu * e1[d] + pvv * e2[d];
        pv.at(x[0], x[1], x[2]);
        for (size_t q = 0; q < specs.size(); q++)
          out[q] += w * wr * pv.value(specs[q]) * toAng3;
      }
    }
  }
}

bool compute(const DensityPlane::Evaluator& ev, const Line& line, int points,
             bool bohr, Quantity q, const vector<RadialDist::Spec>& specs,
             Result& out, std::function<bool(double)> onStep)
{
  points = max(2, q == PLANE ? min(points, 241) : points);
  double length = 0;
  for (int d = 0; d < 3; d++) length += (line.end[d] - line.start[d]) * (line.end[d] - line.start[d]);
  length = sqrt(length);
  const double unitA = bohr ? BOHR_ANG : 1.0;       // output length in Angstrom
  //  The start lies behind the first atom by the margin.
  const double startS = -((line.origin[0] - line.start[0]) * line.unit[0] +
                          (line.origin[1] - line.start[1]) * line.unit[1] +
                          (line.origin[2] - line.start[2]) * line.unit[2]);
  //  Positions as fractions of the line: even steps, and a fine run
  //  through each atom the line passes.
  vector<double> ts;
  for (int n = 0; n < points; n++) ts.push_back(double(n) / (points - 1));
  if (length > 0) {
    //  The plane integral is costlier per point, so its runs are coarser, but
    //  wider: its area is the electron count.
    const double run = q == PLANE ? 0.15 : 0.25, step = q == PLANE ? 0.005 : 0.004;
    const double run2 = q == PLANE ? 0.8 : 0.0, step2 = 0.02;
    for (int a = 0; a < (int)ev.symbols.size(); a++) {
      double along = 0, perp2 = 0, r[3];
      for (int d = 0; d < 3; d++) {
        r[d] = ev.coordsAng[3*a + d] - line.origin[d];
        along += r[d] * line.unit[d];
      }
      for (int d = 0; d < 3; d++) {
        const double c = r[d] - along * line.unit[d];
        perp2 += c * c;
      }
      if (sqrt(perp2) > MARK_TOLERANCE) continue;
      const double centre = along - startS;         // from the line's start
      for (double u = -run; u <= run + 1e-9; u += step) {
        const double t = (centre + u) / length;
        if (t > 0.0 && t < 1.0) ts.push_back(t);
      }
      for (double u = -run2; u <= run2 + 1e-9 && run2 > 0; u += step2) {
        const double t = (centre + u) / length;
        if (t > 0.0 && t < 1.0) ts.push_back(t);
      }
    }
    sort(ts.begin(), ts.end());
    vector<double> kept;
    for (double t : ts)
      if (kept.empty() || (t - kept.back()) * length > 1e-9) kept.push_back(t);
    ts.swap(kept);
  }
  points = (int)ts.size();
  out.s.assign(points, 0.0);
  out.curves.assign(specs.size(), RadialDist::Curve());
  for (size_t k = 0; k < specs.size(); k++) {
    out.curves[k].spec = specs[k];
    out.curves[k].p.assign(points, 0.0);
  }
  RadialDist::PointValues pv(ev, specs);
  for (int n = 0; n < points; n++) {
    const double t = ts[n];
    double x[3];
    for (int d = 0; d < 3; d++) x[d] = line.start[d] + t * (line.end[d] - line.start[d]);
    out.s[n] = (startS + t * length) / unitA;
    if (q == PLANE) {
      vector<double> vals;
      planeIntegrals(ev, pv, x, line.unit, line.radius, specs, vals);
      for (size_t k = 0; k < specs.size(); k++) out.curves[k].p[n] = vals[k] * unitA;
      if (onStep && n % 4 == 3 && !onStep(double(n + 1) / points)) return false;
      continue;
    }
    pv.at(x[0], x[1], x[2]);
    for (size_t k = 0; k < specs.size(); k++) {
      double v;
      if (q == PSI) v = pv.amplitude(specs[k]) * pow(BOHR_ANG / unitA, -1.5);
      else v = pv.value(specs[k]) * pow(BOHR_ANG / unitA, -3.0);
      out.curves[k].p[n] = v;
    }
    if (onStep && n % 16 == 15 && !onStep(double(n + 1) / points)) return false;
  }
  for (RadialDist::Curve& c : out.curves) {
    c.integral = 0.0;
    for (int n = 1; n < points; n++)
      c.integral += 0.5 * (c.p[n] + c.p[n-1]) * (out.s[n] - out.s[n-1]);
  }
  out.marks.clear();
  const int natoms = (int)ev.symbols.size();
  for (int a = 0; a < natoms; a++) {
    double r[3], along = 0;
    for (int d = 0; d < 3; d++) {
      r[d] = ev.coordsAng[3*a + d] - line.origin[d];
      along += r[d] * line.unit[d];
    }
    double perp = 0;
    for (int d = 0; d < 3; d++) {
      const double c = r[d] - along * line.unit[d];
      perp += c * c;
    }
    if (sqrt(perp) > MARK_TOLERANCE) continue;
    if (along < startS - 1e-9 || along > startS + length + 1e-9) continue;
    Mark m;
    m.s = along / unitA;
    m.label = ev.symbols[a] + to_string(a + 1);
    out.marks.push_back(m);
  }
  return true;
}

string csv(const Result& r, bool bohr)
{
  string s = bohr ? "# s in bohr from the first atom\n" : "# s in Angstrom from the first atom\n";
  for (const Mark& m : r.marks) {
    char b[64];
    snprintf(b, sizeof b, "# atom %s at s = %.6f\n", m.label.c_str(), m.s);
    s += b;
  }
  s += "s";
  for (const RadialDist::Curve& c : r.curves) {
    string l = c.spec.label;
    for (char& ch : l) if (ch == ',') ch = ';';
    s += "," + l;
  }
  s += "\n";
  char buf[64];
  for (size_t i = 0; i < r.s.size(); i++) {
    snprintf(buf, sizeof buf, "%.6f", r.s[i]);
    s += buf;
    for (const RadialDist::Curve& c : r.curves) {
      snprintf(buf, sizeof buf, ",%.8e", c.p[i]);
      s += buf;
    }
    s += "\n";
  }
  return s;
}

}  // namespace LineProfile
