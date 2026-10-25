#include <cmath>
#include <cstdio>

#include "tdat/LineProfile.H"

using namespace std;

namespace {
const double BOHR_ANG = 0.52917721092;
const double MARK_TOLERANCE = 0.3;     // Angstrom
}

namespace LineProfile {

Line throughAtoms(const DensityPlane::Evaluator& ev, int i, int j, double margin)
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
  return l;
}

bool compute(const DensityPlane::Evaluator& ev, const Line& line, int points,
             bool bohr, Quantity q, const vector<RadialDist::Spec>& specs,
             Result& out)
{
  points = max(2, points);
  double length = 0;
  for (int d = 0; d < 3; d++) length += (line.end[d] - line.start[d]) * (line.end[d] - line.start[d]);
  length = sqrt(length);
  const double unitA = bohr ? BOHR_ANG : 1.0;       // output length in Angstrom
  //  The start lies behind the first atom by the margin.
  const double startS = -((line.origin[0] - line.start[0]) * line.unit[0] +
                          (line.origin[1] - line.start[1]) * line.unit[1] +
                          (line.origin[2] - line.start[2]) * line.unit[2]);
  out.s.assign(points, 0.0);
  out.curves.assign(specs.size(), RadialDist::Curve());
  for (size_t k = 0; k < specs.size(); k++) {
    out.curves[k].spec = specs[k];
    out.curves[k].p.assign(points, 0.0);
  }
  for (int n = 0; n < points; n++) {
    const double t = double(n) / (points - 1);
    double x[3];
    for (int d = 0; d < 3; d++) x[d] = line.start[d] + t * (line.end[d] - line.start[d]);
    out.s[n] = (startS + t * length) / unitA;
    for (size_t k = 0; k < specs.size(); k++) {
      double v;
      if (q == PSI) v = RadialDist::amplitude(ev, specs[k], x[0], x[1], x[2]) * pow(BOHR_ANG / unitA, -1.5);
      else v = RadialDist::value(ev, specs[k], x[0], x[1], x[2]) * pow(BOHR_ANG / unitA, -3.0);
      out.curves[k].p[n] = v;
    }
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
