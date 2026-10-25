// RadialDist: the quadrature, the maxima finder, and the numbers on
// stored ECCE-QM atoms.
//
//   radialdist_test FIXTURE-ROOT      (tests/builder/radialdist-fixtures)
//
// Oracles that do not know the code under test: the exact hydrogen 1s
// radial distribution 4 r^2 exp(-2r) (bohr), whose maximum is the Bohr
// radius, and the electron count / orbital populations of closed-shell Ne.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "util/EcceURL.H"
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
  return failures ? 1 : 0;
}
