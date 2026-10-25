// RadialDist and LineProfile: the quadrature, the maxima finder, and the
// numbers on stored ECCE-QM atoms and diatomics.
//
//   radialdist_test FIXTURE-ROOT      (tests/builder/radialdist-fixtures)
//
// Oracles that do not know the code under test: the exact hydrogen 1s
// radial distribution 4 r^2 exp(-2r) (bohr), whose maximum is the Bohr
// radius, the electron count / orbital populations of closed-shell Ne and
// Ar, the symmetry of N2 and the node of sigma-u* in H2.
#include <cmath>
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "util/EcceURL.H"
#include "tdat/LineProfile.H"
#include "tdat/RadialDist.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/IPropCalculation.H"
#include "dsm/Resource.H"

using namespace std;

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { ++failures; \
  fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); } } while (0)

static const double BOHR = 0.52917721092;

static void numerics()
{
  //  Maxima and integral of the exact curve, on the default grid (bohr).
  RadialDist::Options opt;
  opt.bohr = true;
  opt.rMax = 12.0;
  const vector<double> r = RadialDist::grid(opt);
  vector<double> p(r.size());
  for (size_t i = 0; i < r.size(); i++) p[i] = 4*r[i]*r[i]*exp(-2*r[i]);
  vector<double> mr, mp;
  RadialDist::findMaxima(r, p, mr, mp);
  CHECK(mr.size() == 1 && fabs(mr[0] - 1.0) < 1e-4, "exact 1s: one maximum at 1 bohr");
  CHECK(fabs(RadialDist::integrate(r, p) - 1.0) < 1e-6, "exact 1s integrates to 1");
  fprintf(stderr, "exact 1s: max %.6f bohr, integral %.8f\n", mr.empty() ? 0 : mr[0],
          RadialDist::integrate(r, p));

  //  A ripple below 1% of the top is not a maximum.
  vector<double> q(r.size());
  for (size_t i = 0; i < r.size(); i++) q[i] = p[i] * (1 + 0.001*sin(40*r[i]));
  RadialDist::findMaxima(r, q, mr, mp);
  CHECK(mr.size() == 1, "ripple is not a shell");
}

static Resource *open(const string& path, IPropCalculation *&calc)
{
  Resource *res = EDSIFactory::getResource(EcceURL("file://" + path));
  calc = dynamic_cast<IPropCalculation*>(res);
  return res;
}

static RadialDist::Spec mo(int n)
{
  RadialDist::Spec s;
  s.total = false;
  s.mo = n;
  s.label = "MO " + to_string(n);
  return s;
}

static void hydrogen(const string& root)
{
  IPropCalculation *calc;
  open(root + "/h", calc);
  DensityPlane::Evaluator ev;
  CHECK(calc && ev.build(calc), ("h builds: " + ev.complaint).c_str());
  if (!calc || ev.symbols.empty()) return;
  const double c[3] = { ev.coordsAng[0], ev.coordsAng[1], ev.coordsAng[2] };

  for (int unit = 0; unit < 2; unit++) {
    RadialDist::Options opt;
    opt.bohr = unit == 1;
    opt.rMax = unit ? 12.0 : 6.0;
    vector<RadialDist::Curve> out;
    vector<RadialDist::Spec> specs = { mo(1) };
    RadialDist::Spec t;
    t.label = "total";
    specs.push_back(t);
    CHECK(RadialDist::compute(ev, c, opt, specs, out), "compute");
    const RadialDist::Curve& s = out[0];
    CHECK(s.maxR.size() == 1, "H 1s has one maximum");
    if (s.maxR.empty()) continue;
    const double rmaxA = unit ? s.maxR[0]*BOHR : s.maxR[0];
    //  Exact curve in the output unit: 4 r^2 exp(-2r) per bohr; per
    //  Angstrom the same function scaled by 1/BOHR.
    const vector<double> r = RadialDist::grid(opt);
    double dev = 0, top = 0;
    for (size_t i = 0; i < r.size(); i++) {
      const double rb = unit ? r[i] : r[i]/BOHR;
      const double exact = 4*rb*rb*exp(-2*rb) * (unit ? 1.0 : 1.0/BOHR);
      dev = max(dev, fabs(s.p[i] - exact));
      top = max(top, exact);
    }
    fprintf(stderr, "H 1s (%s): max at %.5f A (exact 0.52918), integral %.6f, "
            "max deviation from 4r^2exp(-2r) %.3e (peak %.4f)\n",
            unit ? "bohr grid" : "A grid", rmaxA, s.integral, dev, top);
    CHECK(fabs(rmaxA - BOHR) < 0.01, "H 1s maximum at the Bohr radius (0.01 A)");
    CHECK(fabs(s.integral - 1.0) < 1e-3, "H 1s integrates to 1");
    CHECK(dev < 0.02*top, "H 1s within 2% of the exact curve");
    CHECK(fabs(out[1].integral - 1.0) < 1e-3, "H density integrates to 1 electron");
    //  One electron: the density curve is the orbital's.
    double d = 0;
    for (size_t i = 0; i < r.size(); i++) d = max(d, fabs(out[1].p[i] - s.p[i]));
    CHECK(d < 1e-6*top, "H density equals the 1s curve");
  }
}

static void neon(const string& root)
{
  IPropCalculation *calc;
  open(root + "/ne", calc);
  DensityPlane::Evaluator ev;
  CHECK(calc && ev.build(calc), ("ne builds: " + ev.complaint).c_str());
  if (!calc || ev.symbols.empty()) return;
  const double c[3] = { ev.coordsAng[0], ev.coordsAng[1], ev.coordsAng[2] };
  RadialDist::Options opt;
  vector<RadialDist::Spec> specs = { mo(1), mo(2), mo(3) };
  RadialDist::Spec t;
  t.label = "total";
  specs.push_back(t);
  vector<RadialDist::Curve> out;
  CHECK(RadialDist::compute(ev, c, opt, specs, out), "compute");
  const char *name[] = { "1s", "2s", "2p", "total" };
  for (int k = 0; k < 4; k++) {
    fprintf(stderr, "Ne %s: integral %.6f, maxima", name[k], out[k].integral);
    for (size_t m = 0; m < out[k].maxR.size(); m++) fprintf(stderr, " %.4f", out[k].maxR[m]);
    fprintf(stderr, " A\n");
  }
  CHECK(fabs(out[3].integral - 10.0) < 1e-3, "Ne density integrates to 10 electrons");
  for (int k = 0; k < 3; k++)
    CHECK(fabs(out[k].integral - 1.0) < 1e-3, "Ne orbital population is 1");
  CHECK(out[0].maxR.size() == 1, "1s: one maximum");
  CHECK(out[1].maxR.size() == 2, "2s: two maxima (n - l)");
  CHECK(out[2].maxR.size() == 1, "2p: one maximum");
  if (out[0].maxR.size() == 1 && out[1].maxR.size() == 2 && out[2].maxR.size() == 1) {
    CHECK(out[1].maxR[0] < out[2].maxR[0], "2s inner maximum lies inside the 2p maximum");
    CHECK(out[1].maxR[0] < 2.5*out[0].maxR[0], "2s inner maximum sits in the 1s region (penetration)");
    CHECK(out[1].maxR[1] > out[2].maxR[0] * 0.8, "2s outer maximum is out with the 2p");
    CHECK(out[0].maxR[0] < 0.15, "Ne 1s maximum is near the nucleus");
  }
  CHECK(out[3].maxR.size() >= 2, "total density shows two shells");
}


static LineProfile::Line axisLine(double half)
{
  LineProfile::Line l = {};
  l.start[0] = -half; l.end[0] = half;
  l.unit[0] = 1.0;
  return l;
}

static bool load(const string& root, const char *name, IPropCalculation *&calc,
                 DensityPlane::Evaluator& ev)
{
  open(root + "/" + name, calc);
  const bool ok = calc && ev.build(calc);
  CHECK(ok, (string(name) + " builds: " + ev.complaint).c_str());
  return ok;
}

//  H 1s along a line through the nucleus: exp(-|s|/a0) from the stored basis.
static void hydrogenLine(const string& root)
{
  IPropCalculation *calc;
  DensityPlane::Evaluator ev;
  if (!load(root, "h", calc, ev)) return;
  LineProfile::Line line = axisLine(3.0);
  for (int d = 0; d < 3; d++) line.origin[d] = ev.coordsAng[d];
  for (int d = 0; d < 3; d++) { line.start[d] += ev.coordsAng[d]; line.end[d] += ev.coordsAng[d]; }
  vector<RadialDist::Spec> specs = { mo(1) };
  RadialDist::Spec t;
  t.label = "total";
  specs.push_back(t);
  LineProfile::Result psi, rho;
  LineProfile::compute(ev, line, 1201, false, LineProfile::PSI, specs, psi);
  LineProfile::compute(ev, line, 1201, false, LineProfile::DENSITY, specs, rho);
  const size_t mid = psi.s.size() / 2;
  //  Normalised at s = 0 the Gaussian basis, which has no cusp, is 3% low
  //  there; normalised at 0.25 A it follows the exponential.
  size_t ref = mid;
  for (size_t i = mid; i < psi.s.size(); i++)
    if (fabs(psi.s[i] - 0.25) < fabs(psi.s[ref] - 0.25)) ref = i;
  double dev0 = 0, devRef = 0, peak = -1;
  size_t peakAt = 0;
  for (size_t i = 0; i < psi.s.size(); i++) {
    const double exact = exp(-fabs(psi.s[i]) / BOHR);
    dev0 = max(dev0, fabs(psi.curves[0].p[i] / psi.curves[0].p[mid] - exact) / exact);
    if (fabs(psi.s[i]) >= 0.1)
      devRef = max(devRef, fabs(psi.curves[0].p[i] / psi.curves[0].p[ref] * exp(-psi.s[ref] / BOHR) - exact) / exact);
    if (rho.curves[1].p[i] > peak) { peak = rho.curves[1].p[i]; peakAt = i; }
  }
  fprintf(stderr, "H line, |s| < 3 A: psi vs exp(-|s|/a0): max relative deviation %.3e normalised at s = 0, "
          "%.3e normalised at 0.25 A (|s| > 0.1 A); density peak at s = %.4f A, rho(0) = %.5f "
          "(exact 1/(pi a0^3) = %.5f A^-3)\n",
          dev0, devRef, psi.s[peakAt], peak, 1.0 / (M_PI * BOHR * BOHR * BOHR));
  CHECK(fabs(psi.s[peakAt]) < 0.01, "H density peaks at the nucleus");
  CHECK(devRef < 0.03, "H 1s along a line is exp(-|s|/a0) to 3% beyond 0.1 A");
  CHECK(dev0 < 0.06, "H 1s along a line is exp(-|s|/a0) to 6% when normalised at the nucleus (no cusp in a Gaussian basis)");
  CHECK(fabs(psi.curves[0].p[mid - 3] - psi.curves[0].p[mid + 3]) < 1e-9 * fabs(psi.curves[0].p[mid]),
        "H 1s is even along the line");
}

//  Ne 2p along a line through the nucleus: zero there, odd, changes sign.
static void neonLine(const string& root)
{
  IPropCalculation *calc;
  DensityPlane::Evaluator ev;
  if (!load(root, "ne", calc, ev)) return;
  LineProfile::Line line = axisLine(2.0);
  vector<RadialDist::Spec> specs = { mo(3), mo(4), mo(5) };
  LineProfile::Result r;
  LineProfile::compute(ev, line, 1001, false, LineProfile::PSI, specs, r);
  const size_t mid = r.s.size() / 2;
  bool changes = false;
  for (size_t k = 0; k < 3; k++) {
    const vector<double>& p = r.curves[k].p;
    double top = 0;
    for (double v : p) top = max(top, fabs(v));
    CHECK(fabs(p[mid]) < 1e-9 * (1 + top), "2p is zero at the nucleus");
    CHECK(fabs(p[mid + 100] + p[mid - 100]) < 1e-8 * (1 + top), "2p is odd along the line");
    if (p[mid + 100] * p[mid - 100] < -1e-12) changes = true;
  }
  CHECK(changes, "a 2p orbital changes sign through the nucleus");
}

//  Ar: shells as contributions, summing to the density.
static void argon(const string& root)
{
  IPropCalculation *calc;
  DensityPlane::Evaluator ev;
  if (!load(root, "ar", calc, ev)) return;
  const vector<string> labels = RadialDist::shellLabels(calc, ev, false);
  vector<int> occ;
  for (size_t m = 0; m < ev.occ.size(); m++) if (ev.occ[m] > 0) occ.push_back((int)m + 1);
  vector<RadialDist::Spec> specs =
      RadialDist::orbitalSpecs(ev, occ, RadialDist::BOTH, true, true, labels);
  RadialDist::Spec t;
  t.label = "total";
  specs.push_back(t);
  CHECK(specs.size() == 6, "Ar: 1s 2s 2p 3s 3p and the total");
  string names;
  for (const RadialDist::Spec& s : specs) names += s.label + " ";
  fprintf(stderr, "Ar curves: %s\n", names.c_str());
  RadialDist::Options opt;
  vector<RadialDist::Curve> out;
  const double c[3] = { ev.coordsAng[0], ev.coordsAng[1], ev.coordsAng[2] };
  RadialDist::compute(ev, c, opt, specs, out);
  const double sumErr = RadialDist::maxSumError(out);
  fprintf(stderr, "Ar around the atom: total integral %.5f, contributions vs total max deviation %.2e\n",
          out.back().integral, sumErr);
  const double want[] = { 2, 2, 6, 2, 6 };   // 1s 2s 2p 3s 3p electrons
  for (int k = 0; k < 5 && k < (int)out.size(); k++) {
    fprintf(stderr, "  %s: integral %.5f, outermost maximum %.4f A\n", out[k].spec.label.c_str(),
            out[k].integral, out[k].maxR.empty() ? 0.0 : out[k].maxR.back());
    CHECK(fabs(out[k].integral - want[k]) < 2e-3, "Ar shell electron count");
  }
  CHECK(fabs(out.back().integral - 18.0) < 1e-3, "Ar density integrates to 18");
  CHECK(sumErr >= 0 && sumErr < 1e-9, "Ar contributions sum to the total");
  if (out.size() == 6 && !out[0].maxR.empty() && !out[2].maxR.empty() && !out[4].maxR.empty()) {
    CHECK(out[0].maxR.back() < out[2].maxR.back() && out[2].maxR.back() < out[4].maxR.back(),
          "Ar shells peak at 1s < 2p < 3p");
    CHECK(out[1].maxR.size() == 2 && out[3].maxR.size() == 3 && out[4].maxR.size() == 2,
          "Ar: 2s has 2 maxima, 3s 3, 3p 2 (n - l)");
  }
  //  The same along a line: the sum also holds there.
  LineProfile::Line line = axisLine(2.0);
  LineProfile::Result r;
  LineProfile::compute(ev, line, 801, false, LineProfile::DENSITY, specs, r);
  const double lineErr = RadialDist::maxSumError(r.curves);
  fprintf(stderr, "Ar along a line: contributions vs total max deviation %.2e\n", lineErr);
  CHECK(lineErr >= 0 && lineErr < 1e-9, "Ar line contributions sum to the total");
}

static void diatomics(const string& root)
{
  IPropCalculation *calc;
  DensityPlane::Evaluator n2, h2;
  if (load(root, "n2", calc, n2)) {
    const LineProfile::Line line = LineProfile::throughAtoms(n2, 0, 1, 2.0);
    vector<int> occ;
    for (size_t m = 0; m < n2.occ.size(); m++) if (n2.occ[m] > 0) occ.push_back((int)m + 1);
    vector<RadialDist::Spec> specs =
        RadialDist::orbitalSpecs(n2, occ, RadialDist::BOTH, true, true, vector<string>());
    RadialDist::Spec t;
    t.label = "total";
    specs.push_back(t);
    LineProfile::Result r;
    LineProfile::compute(n2, line, 1001, false, LineProfile::DENSITY, specs, r);
    const vector<double>& rho = r.curves.back().p;
    const size_t n = rho.size();
    double asym = 0, top = 0;
    for (size_t i = 0; i < n; i++) {
      asym = max(asym, fabs(rho[i] - rho[n - 1 - i]));
      top = max(top, rho[i]);
    }
    //  The two nuclei at s = 0 and the bond length; the midpoint between.
    size_t a = 0, b = 0;
    for (size_t i = 0; i < n; i++) {
      if (fabs(r.s[i] - r.marks[0].s) < fabs(r.s[a] - r.marks[0].s)) a = i;
      if (fabs(r.s[i] - r.marks[1].s) < fabs(r.s[b] - r.marks[1].s)) b = i;
    }
    const size_t mid = (a + b) / 2;
    double lo = 1e300;
    for (size_t i = a; i <= b; i++) lo = min(lo, rho[i]);
    fprintf(stderr, "N2 line: %zu marks (%s %.4f, %s %.4f), rho(nucleus) %.4f %.4f, rho(mid) %.4f A^-3, "
            "max asymmetry %.2e of peak %.4f, contributions vs total %.2e\n", r.marks.size(),
            r.marks[0].label.c_str(), r.marks[0].s, r.marks[1].label.c_str(), r.marks[1].s,
            rho[a], rho[b], rho[mid], asym, top, RadialDist::maxSumError(r.curves));
    CHECK(r.marks.size() == 2, "both nuclei are on the line");
    CHECK(asym < 1e-6 * top, "N2 density is symmetric about the midpoint");
    CHECK(rho[a] > 0.99 * top && rho[b] > 0.99 * top, "N2 density is largest at the nuclei");
    CHECK(rho[mid] < 0.2 * rho[a] && rho[mid] < lo * 1.001 + 1e-12, "N2 density has a saddle between the nuclei");
    CHECK(RadialDist::maxSumError(r.curves) < 1e-9, "N2 contributions sum to the total");
  }
  if (load(root, "h2", calc, h2)) {
    const LineProfile::Line line = LineProfile::throughAtoms(h2, 0, 1, 2.0);
    vector<RadialDist::Spec> specs = { mo(1), mo(2) };
    LineProfile::Result r;
    LineProfile::compute(h2, line, 1001, false, LineProfile::PSI, specs, r);
    size_t mid = 0;
    for (size_t i = 0; i < r.s.size(); i++)
      if (fabs(r.s[i] - 0.37) < fabs(r.s[mid] - 0.37)) mid = i;
    const vector<double>& g = r.curves[0].p;
    const vector<double>& u = r.curves[1].p;
    double top = 0;
    for (double v : u) top = max(top, fabs(v));
    fprintf(stderr, "H2 line: sigma_g(mid) = %.4f, sigma_u*(mid) = %.2e of peak %.4f; sign %+.3f / %+.3f\n",
            g[mid], u[mid], top, u[mid - 50], u[mid + 50]);
    CHECK(g[mid] > 0.3 * top, "sigma_g has amplitude between the nuclei");
    CHECK(fabs(u[mid]) < 1e-6 * top, "sigma_u* has a node at the midpoint");
    CHECK(u[mid - 50] * u[mid + 50] < 0, "sigma_u* changes sign at the node");
  }
}

//  A pi orbital is zero on the axis of a linear molecule and not off it;
//  an atom's virtual orbitals are named by angular type only.
static void shiftAndVirtuals(const string& root)
{
  IPropCalculation *calc;
  DensityPlane::Evaluator n2, ne;
  if (load(root, "n2", calc, n2)) {
    vector<RadialDist::Spec> specs;
    for (int m = 1; m <= 7; m++) specs.push_back(mo(m));
    LineProfile::Result on, up, side;
    LineProfile::compute(n2, LineProfile::throughAtoms(n2, 0, 1, 1.0), 301, false,
                         LineProfile::PSI, specs, on);
    LineProfile::compute(n2, LineProfile::throughAtoms(n2, 0, 1, 1.0, 0.5, true), 301, false,
                         LineProfile::PSI, specs, up);
    LineProfile::compute(n2, LineProfile::throughAtoms(n2, 0, 1, 1.0, 0.5, false), 301, false,
                         LineProfile::PSI, specs, side);
    int zeroOnAxis = 0;
    double offAxis = 0;
    for (size_t k = 0; k < specs.size(); k++) {
      double a = 0, b = 0, c = 0;
      for (size_t i = 0; i < on.s.size(); i++) a = max(a, fabs(on.curves[k].p[i]));
      for (size_t i = 0; i < up.s.size(); i++) b = max(b, fabs(up.curves[k].p[i]));
      for (size_t i = 0; i < side.s.size(); i++) c = max(c, fabs(side.curves[k].p[i]));
      if (a < 1e-9) { zeroOnAxis++; offAxis = max(offAxis, max(b, c)); }
    }
    fprintf(stderr, "N2 pi orbitals: %d zero on the axis, largest shifted value %.4f\n", zeroOnAxis, offAxis);
    CHECK(zeroOnAxis == 2, "N2 has two pi orbitals that are zero on the axis");
    CHECK(offAxis > 1e-3, "a pi orbital is not zero on a line shifted 0.5 A");
    CHECK(up.marks.size() == 2 && fabs(up.marks[1].s - on.marks[1].s) < 1e-9,
          "atom marks of a shifted line are those of the axis");
    //  The shifted line is parallel to the axis, 0.5 A away, and the two
    //  directions are perpendicular to each other.
    const LineProfile::Line l0 = LineProfile::throughAtoms(n2, 0, 1, 1.0);
    const LineProfile::Line l1 = LineProfile::throughAtoms(n2, 0, 1, 1.0, 0.5, true);
    const LineProfile::Line l2 = LineProfile::throughAtoms(n2, 0, 1, 1.0, 0.5, false);
    double d1[3], d2[3], dot1 = 0, dot12 = 0, n1 = 0, n2len = 0;
    for (int d = 0; d < 3; d++) {
      d1[d] = l1.start[d] - l0.start[d];
      d2[d] = l2.start[d] - l0.start[d];
      dot1 += d1[d] * l0.unit[d];
      dot12 += d1[d] * d2[d];
      n1 += d1[d] * d1[d];
      n2len += d2[d] * d2[d];
    }
    CHECK(fabs(sqrt(n1) - 0.5) < 1e-9 && fabs(sqrt(n2len) - 0.5) < 1e-9 && fabs(dot1) < 1e-9 &&
          fabs(dot12) < 1e-9, "the shift is 0.5 A, perpendicular to the line and between the two directions");
  }
  if (load(root, "ne", calc, ne)) {
    const vector<string> labels = RadialDist::shellLabels(calc, ne, false);
    int virt = 0;
    bool plain = true;
    for (const string& l : labels)
      if (l.find("virtual") != string::npos) {
        virt++;
        if (!(l == "s (virtual)" || l == "p (virtual)" || l == "d (virtual)" || l == "f (virtual)"))
          plain = false;
      }
    fprintf(stderr, "Ne: %d virtual orbitals named by type\n", virt);
    CHECK(virt > 0 && plain, "Ne virtuals are named by angular type, no principal quantum number");
    vector<int> all;
    for (int m = 1; m <= ne.orbitals(false); m++) all.push_back(m);
    const vector<RadialDist::Spec> specs =
        RadialDist::orbitalSpecs(ne, all, RadialDist::ALPHA, false, true, labels);
    bool unique = true;
    for (size_t a = 0; a < specs.size(); a++)
      for (size_t b = a + 1; b < specs.size(); b++) if (specs[a].label == specs[b].label) unique = false;
    CHECK(unique, "no two curves of Ne carry the same name");
  }
}

//  Orbital names of an atom from the radial nodes (n = l + nodes + 1).
static void atomLabels(const string& root)
{
  static const char *letters = "spdfgh";
  //  ne-aug (canonical virtuals, diffuse basis) is reported, not asserted.
  const char *cases[] = { "ne", "ne-aug", "ne-ivo", "ar" };
  for (const char *name : cases) {
    IPropCalculation *calc;
    DensityPlane::Evaluator ev;
    if (!load(root, name, calc, ev)) continue;
    const vector<string> labels = RadialDist::shellLabels(calc, ev, false);
    vector<double> meanR;
    const vector<int> nodes = RadialDist::radialNodes(ev, false, &meanR);
    string line;
    (void)meanR;
    for (size_t m = 0; m < labels.size(); m++) line += (m ? ", " : "") + labels[m];
    fprintf(stderr, "%s labels: %s\n", name, line.c_str());
    //  The occupied names come from the energy order; the nodes must agree.
    bool agree = true;
    int occupied = 0;
    for (size_t m = 0; m < labels.size(); m++) {
      const string& l = labels[m];
      if (l.empty() || l.find("virtual") != string::npos) continue;
      occupied++;
      const int n = atoi(l.c_str());
      const int ang = (int)(strchr(letters, l[l.size() - 1]) - letters);
      if (m >= nodes.size() || n != ang + nodes[m] + 1) {
        agree = false;
        fprintf(stderr, "  MO %zu: %s but %d radial nodes\n", m + 1, l.c_str(),
                m < nodes.size() ? nodes[m] : -1);
      }
    }
    CHECK(occupied > 0 && agree, (string(name) + ": occupied labels agree with the radial nodes").c_str());
    int named = 0;
    for (const string& l : labels)
      if (l.find("virtual") != string::npos && isdigit((unsigned char)l[0])) named++;
    if (string(name) == "ne-ivo") {
      bool s3 = false, p3 = false;
      for (const string& l : labels) {
        if (l == "3s (virtual)") s3 = true;
        if (l == "3p (virtual)") p3 = true;
      }
      CHECK(s3 && p3, "Ne IVO in aug-cc-pVDZ: 3s and 3p virtuals");
    } else if (string(name) != "ne-aug") {
      CHECK(named == 0, (string(name) + ": canonical virtuals carry no principal quantum number").c_str());
    }
  }
}

int main(int argc, char **argv)
{
  numerics();
  if (argc < 2) {
    cerr << "radialdist_test: no fixture root, numerics only\n";
    return failures ? 1 : 0;
  }
  EDSIServerCentral central;
  hydrogen(argv[1]);
  neon(argv[1]);
  hydrogenLine(argv[1]);
  neonLine(argv[1]);
  argon(argv[1]);
  diatomics(argv[1]);
  shiftAndVirtuals(argv[1]);
  atomLabels(argv[1]);
  return failures ? 1 : 0;
}
