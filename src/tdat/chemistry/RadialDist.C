#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>

#include "dsm/IPropCalculation.H"
#include "tdat/MoAoBasis.H"
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

double value(const DensityPlane::Evaluator& ev, const Spec& s,
             double x, double y, double z)
{
  if (s.total) return ev.density(x, y, z);
  double f = 0;
  if (s.mo > 0) {
    const double psi = ev.orbital(s.mo, s.beta, x, y, z);
    f += s.weight * psi * psi;
  }
  for (const Member& m : s.more) {
    const double psi = ev.orbital(m.mo, m.beta, x, y, z);
    f += m.weight * psi * psi;
  }
  return f;
}

double amplitude(const DensityPlane::Evaluator& ev, const Spec& s,
                 double x, double y, double z)
{
  return s.weight * ev.orbital(s.mo, s.beta, x, y, z);
}

vector<double> grid(const Options& opt)
{
  const int n = max(3, opt.points);
  vector<double> r(n);
  for (int i = 0; i < n; i++) {
    const double u = double(i) / (n - 1);
    if (opt.spacing == Options::SQRT) r[i] = opt.rMax * u * u;
    else if (opt.spacing == Options::LOG && i > 0) {
      const double rMin = 1e-3 / (opt.bohr ? BOHR_ANG : 1.0);
      r[i] = rMin * pow(opt.rMax / rMin, double(i - 1) / (n - 2));
    } else r[i] = opt.rMax * u;
  }
  return r;
}

double integrate(const vector<double>& r, const vector<double>& p)
{
  const size_t n = min(r.size(), p.size());
  if (n < 2) return 0.0;
  const double h = (r[n-1] - r[0]) / (n - 1);
  bool even = true;
  for (size_t i = 1; i < n && even; i++)
    if (fabs((r[i] - r[i-1]) - h) > 1e-9 * fabs(h)) even = false;
  if (!even) {
    double s = 0;
    for (size_t i = 1; i < n; i++) s += 0.5 * (p[i] + p[i-1]) * (r[i] - r[i-1]);
    return s;
  }
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
    const double h = 0.5 * (r[i+1] - r[i-1]);
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
        const double f = value(ev, specs[k], x, y, z);
        out[k].p[i] += u[3] * f;
      }
    }
    for (size_t k = 0; k < specs.size(); k++) {
      if (opt.radial) out[k].p[i] *= fourPi * rb * rb * perUnit;
      else out[k].p[i] *= perUnit * perUnit * perUnit;   // per length^3
    }
    if (onStep && !onStep(double(i) / (r.size() - 1))) return false;
  }
  for (Curve& c : out) {
    c.integral = integrate(r, c.p);
    findMaxima(r, c.p, c.maxR, c.maxP);
  }
  return true;
}

vector<int> radialNodes(const DensityPlane::Evaluator& ev, bool beta,
                        vector<double> *meanR)
{
  vector<int> out;
  if (meanR) meanR->clear();
  if (ev.symbols.size() != 1) return out;
  const int nMo = ev.orbitals(beta);
  out.assign(nMo, -1);
  if (meanR) meanR->assign(nMo, 0.0);
  const LebEntry& leb = lebedevEntry(26);
  const double c[3] = { ev.coordsAng[0], ev.coordsAng[1], ev.coordsAng[2] };
  const int nR = 360;
  const double dr = 0.04;              // bohr; to 14.4 bohr
  for (int m = 1; m <= nMo; m++) {
    //  psi = R(r) Y(angles): the sign changes along one direction are the
    //  radial nodes, in the direction where psi is largest (Y is not zero).
    int best = 0;
    double bestSum = -1;
    vector< vector<double> > amp(leb.npts, vector<double>(nR));
    for (int q = 0; q < leb.npts; q++) {
      double sum = 0;
      for (int i = 0; i < nR; i++) {
        const double r = (i + 1) * dr * BOHR_ANG;
        const double *u = leb.data[q];
        const double a = ev.orbital(m, beta, c[0] + r*u[0], c[1] + r*u[1], c[2] + r*u[2]);
        amp[q][i] = a;
        sum += a*a;
      }
      if (sum > bestSum) { bestSum = sum; best = q; }
    }
    const vector<double>& a = amp[best];
    double top = 0;
    for (double v : a) top = max(top, fabs(v));
    //  Tail noise of a Gaussian sum is not a node: only sign changes
    //  between clearly non-zero values count.
    int nodes = 0, last = 0;
    for (double v : a) {
      if (fabs(v) < 1e-3 * top) continue;
      const int sg = v > 0 ? 1 : -1;
      if (last != 0 && sg != last) nodes++;
      last = sg;
    }
    out[m-1] = nodes;
    if (meanR) {
      double num = 0, den = 0;
      for (int i = 0; i < nR; i++) {
        const double r = (i + 1) * dr;
        num += r * r * r * a[i] * a[i];
        den += r * r * a[i] * a[i];
      }
      (*meanR)[m-1] = den > 0 ? num / den : 0.0;
    }
  }
  return out;
}

vector<string> shellLabels(IPropCalculation *calc,
                           const DensityPlane::Evaluator& ev, bool beta)
{
  vector<string> out;
  if (ev.symbols.size() != 1) return out;
  vector< vector<double> > rows;
  if (!MoAoBasis::coefficients(calc, ev.symbols, beta, rows) || rows.empty())
    return out;
  MoAoBasis basis;
  if (!basis.build(calc, ev.symbols, ev.coordsAng, (int)rows[0].size()))
    return out;
  const vector<double>& occ = beta ? ev.occBeta : ev.occ;
  const vector<double>& energy = beta ? ev.energyBeta : ev.energy;
  static const char *letters = "spdfgh";
  int seen[6] = { 0, 0, 0, 0, 0, 0 };
  double last[6] = { 1e9, 1e9, 1e9, 1e9, 1e9, 1e9 };
  out.assign(rows.size(), string());
  vector<size_t> virt;
  vector<int> virtL;
  for (size_t m = 0; m < rows.size(); m++) {
    MoAoBasis::Composition comp;
    if (!basis.analyse(rows[m], MoAoBasis::MULLIKEN, comp)) continue;
    const MoAoBasis::Share *best = 0;
    for (const MoAoBasis::Share& t : comp.types)
      if (t.l >= 0 && t.l < 6 && (!best || t.share > best->share)) best = &t;
    const bool occupied = m < occ.size() && occ[m] > 0.0;
    //  A virtual is named by shell only when its radial nodes agree with
    //  the occupied ones (see below); otherwise only the angular type.
    if (!occupied) {
      if (best && best->share >= 0.9) {
        out[m] = string(1, letters[best->l]) + " (virtual)";
        virt.push_back(m);
        virtL.push_back(best->l);
      }
      continue;
    }
    if (best && best->share >= 0.9) {
      const int l = best->l;
      const double e = m < energy.size() ? energy[m] : 0.0;
      if (seen[l] == 0 || fabs(e - last[l]) > 1e-4) seen[l]++;
      last[l] = e;
      out[m] = to_string(l + seen[l]) + letters[l];
    }
  }
  //  A virtual is named by shell only if it is a real Rydberg-like orbital:
  //  its radial nodes count up from the occupied shells of its type
  //  (n = l + nodes + 1, steps of 0 or 1 because an IVO set is only
  //  roughly degenerate) and it reaches well beyond the valence shell.  A
  //  canonical virtual of a compact basis is a valence-region function
  //  whose node count is accidental.
  if (!virt.empty()) {
    vector<double> meanR;
    const vector<int> nodes = radialNodes(ev, beta, &meanR);
    int last[6];
    bool started[6] = { false, false, false, false, false, false };
    for (int l = 0; l < 6; l++) last[l] = l;      // none occupied: n starts at l + 1
    double valenceR = 0;
    for (size_t m = 0; m < out.size(); m++)
      if (!out[m].empty() && out[m].find("virtual") == string::npos) {
        const int l = (int)(strchr(letters, out[m][out[m].size() - 1]) - letters);
        last[l] = max(last[l], atoi(out[m].c_str()));
        if (m < meanR.size()) valenceR = meanR[m];     // the highest occupied
      }
    const int first[6] = { last[0] + 1, last[1] + 1, last[2] + 1,
                           last[3] + 1, last[4] + 1, last[5] + 1 };
    for (size_t k = 0; k < virt.size(); k++) {
      const size_t m = virt[k];
      const int l = virtL[k];
      if (m >= nodes.size() || nodes[m] < 0) continue;
      const int n = l + nodes[m] + 1;
      const bool step = started[l] ? (n == last[l] || n == last[l] + 1) : n == first[l];
      if (!step) continue;
      started[l] = true;
      last[l] = n;
      if (meanR[m] >= 2.5 * valenceR)
        out[m] = to_string(n) + letters[l] + " (virtual)";
    }
  }
  return out;
}

vector<string> atomOrbitalLabels(IPropCalculation *calc, bool beta)
{
  DensityPlane::Evaluator ev;
  if (!calc || !ev.build(calc) || ev.symbols.size() != 1) return vector<string>();
  return shellLabels(calc, ev, beta);
}

vector<Spec> orbitalSpecs(const DensityPlane::Evaluator& ev,
                          const vector<int>& mos, Spin spin, bool weighted,
                          bool combine, const vector<string>& labels, bool tagsUnique)
{
  vector<Spec> out;
  vector<double> groupEnergy;
  vector<string> groupTag;
  const bool both = spin == BOTH && ev.unrestricted;
  for (int m : mos) {
    if (m < 1 || m > ev.orbitals(false)) continue;
    const string tag = (size_t)m <= labels.size() ? labels[m-1] : string();
    const double e = (size_t)m <= ev.energy.size() ? ev.energy[m-1] : 0.0;
    vector<Member> mem;
    auto add = [&](bool beta) {
      const vector<double>& occ = beta ? ev.occBeta : ev.occ;
      if (m > ev.orbitals(beta)) return;
      const double w = weighted ? ((size_t)m <= occ.size() ? occ[m-1] : 0.0) : 1.0;
      if (weighted && w <= 0.0) return;
      Member x = { m, beta, w };
      mem.push_back(x);
    };
    if (both) { add(false); add(true); } else add(spin == BETA);
    if (mem.empty()) continue;
    if (combine && !out.empty()) {
      Spec& g = out.back();
      //  Virtuals share a name by type only, so they join by energy.
      const bool virt = tag.find("(virtual)") != string::npos;
      const bool sameTag = !tag.empty() && groupTag.back() == tag &&
                           ((tagsUnique && !virt) || fabs(e - groupEnergy.back()) < 1e-4);
      const bool sameE = tag.empty() && g.label.compare(0, 3, "MO ") == 0 &&
                         fabs(e - groupEnergy.back()) < 1e-4 &&
                         g.label.find('(') == string::npos;
      if (sameTag || sameE) {
        for (const Member& x : mem) g.more.push_back(x);
        if (sameTag && !tagsUnique) {
          g.label = tag + " (MO " + g.label.substr(g.label.find("MO ") + 3, g.label.find(')') -
                    g.label.find("MO ") - 3) + "-" + to_string(m) + ")";
        } else if (sameE) {
          const size_t dash = g.label.find('-');
          g.label = g.label.substr(0, dash) + "-" + to_string(m);
        }
        continue;
      }
    }
    Spec s;
    s.total = false;
    s.mo = mem[0].mo;
    s.beta = mem[0].beta;
    s.weight = mem[0].weight;
    for (size_t k = 1; k < mem.size(); k++) s.more.push_back(mem[k]);
    if (!tag.empty() && tagsUnique) s.label = tag;
    else if (!tag.empty()) s.label = tag + " (MO " + to_string(m) + ")";
    else s.label = "MO " + to_string(m);
    if (spin == BETA) s.label += " beta";
    out.push_back(s);
    groupEnergy.push_back(e);
    groupTag.push_back(tag);
  }
  //  Two virtual curves of one type: tell them apart by orbital number.
  for (size_t a = 0; a < out.size(); a++) {
    const size_t at = out[a].label.find("(virtual)");
    if (at == string::npos) continue;
    bool dup = false;
    for (size_t b = 0; b < out.size(); b++)
      if (b != a && out[b].label == out[a].label) dup = true;
    if (dup) out[a].label.insert(at + 8, ", MO " + to_string(out[a].mo));
  }
  return out;
}

double maxSumError(const vector<Curve>& curves)
{
  const Curve *total = 0;
  for (const Curve& c : curves) if (c.spec.total) total = &c;
  if (!total) return -1.0;
  double worst = -1.0;
  for (size_t i = 0; i < total->p.size(); i++) {
    double sum = 0;
    bool any = false;
    for (const Curve& c : curves) if (!c.spec.total) { sum += c.p[i]; any = true; }
    if (any) worst = max(worst, fabs(sum - total->p[i]));
  }
  return worst;
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
