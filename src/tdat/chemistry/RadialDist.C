#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>

#include "tdat/RadialDist.H"

//  The Lebedev-Laikov tables of the ECCE-QM engine; self-contained data.
#include "../../qm/lebedev_data.inc"

using namespace std;

namespace {

const double BOHR_ANG = 0.52917721092;

const LebEntry& lebedevEntry(int want)
{
  for (const LebEntry& e : leb_registry)
    if (e.npts >= want) return e;
  return leb_registry[sizeof leb_registry/sizeof leb_registry[0] - 1];
}

}  // namespace

namespace RadialDist {

vector<double> grid(const Options& opt)
{
  const int n = max(3, opt.points);
  vector<double> r(n);
  for (int i = 0; i < n; i++) r[i] = opt.rMax * i / (n - 1);
  return r;
}

double integrate(const vector<double>& r, const vector<double>& p)
{
  const size_t n = min(r.size(), p.size());
  if (n < 2) return 0.0;
  const double h = (r[n-1] - r[0]) / (n - 1);
  if (n % 2 == 1 && n >= 3) {
    double s = p[0] + p[n-1];
    for (size_t i = 1; i + 1 < n; i++) s += (i % 2 ? 4.0 : 2.0) * p[i];
    return s * h / 3.0;
  }
  double s = 0.5 * (p[0] + p[n-1]);
  for (size_t i = 1; i + 1 < n; i++) s += p[i];
  return s * h;
}

void findMaxima(const vector<double>& r, const vector<double>& p,
                vector<double>& maxR, vector<double>& maxP)
{
  maxR.clear();
  maxP.clear();
  const size_t n = min(r.size(), p.size());
  if (n < 3) return;
  double top = 0;
  for (size_t i = 0; i < n; i++) top = max(top, p[i]);
  //  Ripple below 1% of the highest peak is not a shell.
  const double floorValue = 0.01 * top;
  for (size_t i = 1; i + 1 < n; i++) {
    if (!(p[i] > p[i-1] && p[i] >= p[i+1]) || p[i] < floorValue) continue;
    const double a = p[i-1], b = p[i], c = p[i+1];
    const double den = a - 2*b + c;
    const double d = den != 0.0 ? 0.5 * (a - c) / den : 0.0;
    const double h = r[i] - r[i-1];
    maxR.push_back(r[i] + d*h);
    maxP.push_back(b - 0.25 * (a - c) * d);
  }
}

int defaultCentre(const DensityPlane::Evaluator& ev)
{
  const int n = (int)ev.symbols.size();
  if (n <= 1) return 0;
  static const char *names[] = { "H","He","Li","Be","B","C","N","O","F","Ne",
    "Na","Mg","Al","Si","P","S","Cl","Ar","K","Ca","Sc","Ti","V","Cr","Mn",
    "Fe","Co","Ni","Cu","Zn","Ga","Ge","As","Se","Br","Kr" };
  static const double mass[] = { 1.008,4.003,6.94,9.012,10.81,12.011,14.007,
    15.999,18.998,20.18,22.99,24.305,26.98,28.086,30.974,32.06,35.45,39.948,
    39.098,40.078,44.956,47.867,50.942,51.996,54.938,55.845,58.933,58.693,
    63.546,65.38,69.723,72.63,74.922,78.971,79.904,83.798 };
  double cm[3] = { 0, 0, 0 }, mt = 0;
  for (int a = 0; a < n; a++) {
    double m = 12.0;
    for (size_t k = 0; k < sizeof names/sizeof names[0]; k++)
      if (ev.symbols[a] == names[k]) m = mass[k];
    for (int d = 0; d < 3; d++) cm[d] += m * ev.coordsAng[3*a + d];
    mt += m;
  }
  int best = 0;
  double bd = 1e300;
  for (int a = 0; a < n; a++) {
    double d2 = 0;
    for (int d = 0; d < 3; d++) {
      const double x = ev.coordsAng[3*a + d] - cm[d]/mt;
      d2 += x*x;
    }
    if (d2 < bd - 1e-9) { bd = d2; best = a; }
  }
  return best;
}

bool compute(const DensityPlane::Evaluator& ev, const double centre[3],
             const Options& opt, const vector<Spec>& specs,
             vector<Curve>& out, std::function<bool(double)> onStep)
{
  const vector<double> r = grid(opt);
  const LebEntry& leb = lebedevEntry(opt.lebedev);
  out.assign(specs.size(), Curve());
  for (size_t k = 0; k < specs.size(); k++) {
    out[k].spec = specs[k];
    out[k].p.assign(r.size(), 0.0);
  }
  //  The evaluator works in bohr; r^2 dOmega then leaves bohr^-1, turned
  //  into Angstrom^-1 for an Angstrom grid.
  const double rScale = opt.bohr ? BOHR_ANG : 1.0;   // grid unit -> Angstrom
  const double perUnit = opt.bohr ? 1.0 : 1.0 / BOHR_ANG;
  const double fourPi = 4.0 * M_PI;

  for (size_t i = 1; i < r.size(); i++) {
    const double rA = r[i] * rScale;
    const double rb = rA / BOHR_ANG;
    for (int q = 0; q < leb.npts; q++) {
      const double *u = leb.data[q];
      const double x = centre[0] + rA*u[0], y = centre[1] + rA*u[1],
                   z = centre[2] + rA*u[2];
      for (size_t k = 0; k < specs.size(); k++) {
        double f;
        if (specs[k].total) {
          f = ev.density(x, y, z);
        } else {
          const double psi = ev.orbital(specs[k].mo, specs[k].beta, x, y, z);
          f = psi*psi;
        }
        out[k].p[i] += u[3] * f;
      }
    }
    for (size_t k = 0; k < specs.size(); k++)
      out[k].p[i] *= fourPi * rb * rb * perUnit;
    if (onStep && !onStep(double(i) / (r.size() - 1))) return false;
  }
  for (Curve& c : out) {
    c.integral = integrate(r, c.p);
    findMaxima(r, c.p, c.maxR, c.maxP);
  }
  return true;
}

string csv(const vector<double>& r, const vector<Curve>& curves, bool bohr)
{
  string s = bohr ? "# r in bohr; curves in 1/bohr\n"
                  : "# r in Angstrom; curves in 1/Angstrom\n";
  s += "r";
  for (const Curve& c : curves) {
    string l = c.spec.label;
    for (char& ch : l) if (ch == ',') ch = ';';
    s += "," + l;
  }
  s += "\n";
  char buf[64];
  for (size_t i = 0; i < r.size(); i++) {
    snprintf(buf, sizeof buf, "%.6f", r[i]);
    s += buf;
    for (const Curve& c : curves) {
      snprintf(buf, sizeof buf, ",%.8e", c.p[i]);
      s += buf;
    }
    s += "\n";
  }
  return s;
}

}  // namespace RadialDist
