#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <sstream>

#include "util/MdAnalysis.H"
#include "util/MdTrajectory.H"

namespace {
const double kPi = 3.14159265358979323846;
const double kBoltzmann = 1.380649e-23;   // J/K
const double kAmu = 1.66053906660e-27;    // kg
}

double mbDensity(double v, double massU, double T)
{
  const double a = massU * kAmu / (2 * kBoltzmann * T);
  return 4 * kPi * std::pow(a / kPi, 1.5) * v * v * std::exp(-a * v * v);
}

double mbMostProbable(double massU, double T)
{
  return std::sqrt(2 * kBoltzmann * T / (massU * kAmu));
}

double mbMean(double massU, double T)
{
  return std::sqrt(8 * kBoltzmann * T / (kPi * massU * kAmu));
}

double mbRms(double massU, double T)
{
  return std::sqrt(3 * kBoltzmann * T / (massU * kAmu));
}

DiffusionFit fitDiffusion(const std::vector<double>& time,
                          const std::vector<double>& msd, double from, double to)
{
  DiffusionFit r;
  if (time.size() < 2 || time.size() != msd.size()) return r;
  const double span = time.back();
  if (!(from < to)) { from = 0.25 * span; to = span; }
  double n = 0, sx = 0, sy = 0, sxx = 0, sxy = 0, lo = 1e300, hi = -1e300;
  for (size_t i = 0; i < time.size(); i++) {
    if (time[i] < from || time[i] > to) continue;
    n++;
    sx += time[i]; sy += msd[i];
    sxx += time[i] * time[i]; sxy += time[i] * msd[i];
    lo = std::min(lo, time[i]); hi = std::max(hi, time[i]);
  }
  const double den = n * sxx - sx * sx;
  if (n < 2 || den <= 0) return r;
  r.slope = (n * sxy - sx * sy) / den;
  r.intercept = (sy - r.slope * sx) / n;
  r.D = r.slope / 6;
  r.from = lo; r.to = hi; r.points = (int)n;
  r.ok = true;
  return r;
}

bool speedDistribution(const std::string& trr, const MdTopology& top,
                       double refT, int nbins, std::vector<SpeedCurve>& out,
                       std::string& error)
{
  out.clear();
  if (refT <= 0 || nbins < 4) { error = "needs a reference temperature and bins"; return false; }
  std::vector<double> mass;
  std::vector<int> mol, ti;
  top.expand(mass, mol, ti);
  // One curve per distinct mass: atoms of one mass have one distribution.
  std::map<long, int> curveOf;
  std::vector<int> curve(mass.size());
  std::vector<std::string> typeName;
  std::vector<double> curveMass;
  for (size_t i = 0; i < mass.size(); i++) {
    // Virtual sites (the TIP4P charge site) carry no mass and no speed.
    if (mass[i] <= 0) { curve[i] = -1; continue; }
    const long key = (long)std::floor(mass[i] * 1000 + 0.5);
    if (!curveOf.count(key)) {
      curveOf[key] = (int)curveMass.size();
      curveMass.push_back(mass[i]);
      // the atom's own type name, for a mass no element has
      size_t start = i;
      while (start > 0 && mol[start - 1] == mol[i]) start--;
      typeName.push_back(top.types[ti[i]].atoms[i - start].type);
    }
    curve[i] = curveOf[key];
  }
  double lightest = curveMass[0];
  for (double m : curveMass) lightest = std::min(lightest, m);
  const double vmax = 4.5 * mbMostProbable(lightest, refT);

  std::vector<std::vector<double> > hist(curveMass.size(), std::vector<double>(nbins, 0.0));
  std::vector<double> sum(curveMass.size(), 0), sum2(curveMass.size(), 0), count(curveMass.size(), 0);
  MdTrajectoryReader rd;
  if (!rd.open(trr, error)) return false;
  MdFrame fr;
  int frames = 0;
  while (rd.next(fr, error)) {
    if (fr.v.empty()) { error = trr + " has no velocities"; return false; }
    if (fr.natoms != (int)mass.size()) {
      error = "the topology has a different number of atoms than the trajectory";
      return false;
    }
    for (int i = 0; i < fr.natoms; i++) {
      const double vx = fr.v[3 * i], vy = fr.v[3 * i + 1], vz = fr.v[3 * i + 2];
      const double s = 1000.0 * std::sqrt(vx * vx + vy * vy + vz * vz);  // nm/ps -> m/s
      const int c = curve[i];
      if (c < 0) continue;
      sum[c] += s; sum2[c] += s * s; count[c]++;
      const int b = (int)(s / vmax * nbins);
      if (b < nbins) hist[c][b]++;
    }
    frames++;
  }
  if (!error.empty()) return false;
  if (!frames) { error = trr + " has no frames"; return false; }
  const double width = vmax / nbins;
  for (size_t c = 0; c < curveMass.size(); c++) {
    SpeedCurve s;
    s.label = elementForMass(curveMass[c]);
    if (s.label.empty()) s.label = typeName[c];
    s.mass = curveMass[c];
    s.refT = refT;
    s.vmax = vmax;
    s.samples = count[c];
    s.mean = sum[c] / count[c];
    s.rms = std::sqrt(sum2[c] / count[c]);
    s.density.resize(nbins);
    for (int b = 0; b < nbins; b++) s.density[b] = hist[c][b] / (count[c] * width);
    out.push_back(s);
  }
  return true;
}

namespace {

// Nearest periodic image of d along box vector rows (rectangular or triclinic,
// lower-triangular as GROMACS stores it).
void minimumImage(double d[3], const float box[3][3])
{
  for (int k = 2; k >= 0; k--) {
    if (box[k][k] <= 0) continue;
    const double n = std::floor(d[k] / box[k][k] + 0.5);
    for (int j = 0; j <= k; j++) d[j] -= n * box[k][j];
  }
}

}  // namespace

bool displacement(const std::string& traj, const MdTopology& top, int maxLags,
                  std::vector<MsdCurve>& out, std::string& error)
{
  out.clear();
  std::vector<double> mass;
  std::vector<int> mol, ti;
  top.expand(mass, mol, ti);
  const int nmol = mol.empty() ? 0 : mol.back() + 1;
  // atoms of a molecule are consecutive; first atom and count per molecule
  std::vector<int> first(nmol, -1), cnt(nmol, 0), molType(nmol, 0);
  std::vector<double> molMass(nmol, 0);
  for (size_t i = 0; i < mol.size(); i++) {
    if (first[mol[i]] < 0) first[mol[i]] = (int)i;
    cnt[mol[i]]++;
    molMass[mol[i]] += mass[i];
    molType[mol[i]] = ti[i];
  }
  MdTrajectoryReader rd;
  if (!rd.open(traj, error)) return false;
  MdFrame fr;
  std::vector<std::vector<float> > pos;   // per frame, 3*nmol unwrapped COM
  std::vector<double> times;
  std::vector<double> prev(3 * nmol, 0.0), shift(3 * nmol, 0.0);
  while (rd.next(fr, error)) {
    if (fr.x.empty()) { error = traj + " has no positions"; return false; }
    if (fr.natoms != (int)mass.size()) {
      error = "the topology has a different number of atoms than the trajectory";
      return false;
    }
    std::vector<double> com(3 * nmol);
    for (int m = 0; m < nmol; m++) {
      const float *r0 = &fr.x[3 * first[m]];
      double acc[3] = {0, 0, 0};
      for (int a = 0; a < cnt[m]; a++) {
        const float *r = &fr.x[3 * (first[m] + a)];
        double d[3] = {r[0] - r0[0], r[1] - r0[1], r[2] - r0[2]};
        minimumImage(d, fr.box);
        const double w = mass[first[m] + a];
        for (int k = 0; k < 3; k++) acc[k] += w * (r0[k] + d[k]);
      }
      for (int k = 0; k < 3; k++) com[3 * m + k] = acc[k] / molMass[m];
    }
    std::vector<float> un(3 * nmol);
    for (int m = 0; m < nmol; m++) {
      double c[3] = {com[3 * m], com[3 * m + 1], com[3 * m + 2]};
      if (!pos.empty()) {
        // follow the molecule through the box: the step is the nearest-image one
        double d[3] = {c[0] - prev[3 * m], c[1] - prev[3 * m + 1], c[2] - prev[3 * m + 2]};
        minimumImage(d, fr.box);
        for (int k = 0; k < 3; k++) {
          shift[3 * m + k] += d[k] - (c[k] - prev[3 * m + k]);
        }
      }
      for (int k = 0; k < 3; k++) {
        prev[3 * m + k] = c[k];
        un[3 * m + k] = (float)(c[k] + shift[3 * m + k]);
      }
    }
    pos.push_back(un);
    times.push_back(fr.time);
  }
  if (!error.empty()) return false;
  const int F = (int)pos.size();
  if (F < 4) { error = traj + " has too few frames"; return false; }
  const double dt = (times.back() - times.front()) / (F - 1);
  const int maxLag = F / 2;
  std::vector<int> lags;
  for (int i = 0; i < std::max(2, maxLags); i++) {
    int l = (int)std::floor(std::pow((double)maxLag, (double)i / (std::max(2, maxLags) - 1)) + 0.5);
    l = std::max(1, std::min(maxLag, l));
    if (lags.empty() || l > lags.back()) lags.push_back(l);
  }
  const int ntypes = (int)top.types.size();
  std::vector<int> typeMols(ntypes, 0);
  for (int m = 0; m < nmol; m++) typeMols[molType[m]]++;
  std::vector<MsdCurve> curves(ntypes);
  for (int t = 0; t < ntypes; t++) {
    curves[t].label = top.types[t].name;
    curves[t].molecules = typeMols[t];
  }
  for (int lag : lags) {
    // at most ~2000 origins per lag, evenly spread
    const int nOrg = F - lag;
    const int step = std::max(1, nOrg / 2000);
    std::vector<double> acc(ntypes, 0.0);
    long used = 0;
    for (int o = 0; o < nOrg; o += step, used++) {
      const float *a = &pos[o][0], *b = &pos[o + lag][0];
      for (int m = 0; m < nmol; m++) {
        const double dx = b[3 * m] - a[3 * m], dy = b[3 * m + 1] - a[3 * m + 1],
                     dz = b[3 * m + 2] - a[3 * m + 2];
        acc[molType[m]] += dx * dx + dy * dy + dz * dz;
      }
    }
    for (int t = 0; t < ntypes; t++) {
      if (!typeMols[t]) continue;
      curves[t].time.push_back(lag * dt);
      curves[t].msd.push_back(acc[t] / ((double)used * typeMols[t]));
    }
  }
  for (int t = 0; t < ntypes; t++)
    if (typeMols[t]) out.push_back(curves[t]);
  return true;
}

namespace {

std::string number(double v)
{
  char b[40];
  snprintf(b, sizeof b, "%.6g", v);
  return b;
}

std::string list(const std::vector<double>& v)
{
  std::string s;
  for (size_t i = 0; i < v.size(); i++) s += (i ? "," : "") + number(v[i]);
  return s;
}

std::vector<std::string> split(const std::string& s, char sep)
{
  std::vector<std::string> r;
  std::string cur;
  for (char c : s) {
    if (c == sep) { r.push_back(cur); cur.clear(); }
    else cur += c;
  }
  r.push_back(cur);
  return r;
}

std::vector<double> numbers(const std::string& s)
{
  std::vector<double> v;
  if (s.empty()) return v;
  for (const std::string& f : split(s, ',')) v.push_back(atof(f.c_str()));
  return v;
}

}  // namespace

std::vector<std::string> encodeSpeed(const std::vector<SpeedCurve>& curves)
{
  std::vector<std::string> r;
  for (const SpeedCurve& c : curves)
    r.push_back(c.label + ";" + number(c.mass) + ";" + number(c.refT) + ";" +
                number(c.vmax) + ";" + number(c.mean) + ";" + number(c.rms) + ";" +
                number(c.samples) + ";" + list(c.density));
  return r;
}

std::vector<std::string> encodeMsd(const std::vector<MsdCurve>& curves)
{
  std::vector<std::string> r;
  for (const MsdCurve& c : curves)
    r.push_back(c.label + ";" + std::to_string(c.molecules) + ";" + list(c.time) +
                ";" + list(c.msd));
  return r;
}

bool decodeSpeed(const std::vector<std::string>& records, std::vector<SpeedCurve>& out)
{
  out.clear();
  for (const std::string& rec : records) {
    std::vector<std::string> f = split(rec, ';');
    if (f.size() != 8) return false;
    SpeedCurve c;
    c.label = f[0]; c.mass = atof(f[1].c_str()); c.refT = atof(f[2].c_str());
    c.vmax = atof(f[3].c_str()); c.mean = atof(f[4].c_str());
    c.rms = atof(f[5].c_str()); c.samples = atof(f[6].c_str());
    c.density = numbers(f[7]);
    out.push_back(c);
  }
  return true;
}

bool decodeMsd(const std::vector<std::string>& records, std::vector<MsdCurve>& out)
{
  out.clear();
  for (const std::string& rec : records) {
    std::vector<std::string> f = split(rec, ';');
    if (f.size() != 4) return false;
    MsdCurve c;
    c.label = f[0]; c.molecules = atoi(f[1].c_str());
    c.time = numbers(f[2]); c.msd = numbers(f[3]);
    if (c.time.size() != c.msd.size()) return false;
    out.push_back(c);
  }
  return true;
}

std::string speedCsv(const std::vector<SpeedCurve>& curves)
{
  std::string out = "# speed distributions; density per m/s, Maxwell-Boltzmann at the run's reference temperature\n";
  for (const SpeedCurve& c : curves) {
    out += "# " + c.label + " mass_u=" + number(c.mass) + " T_ref_K=" + number(c.refT) +
           " mean_m/s=" + number(c.mean) + " rms_m/s=" + number(c.rms) + "\n";
    out += "speed_m/s," + c.label + "_measured," + c.label + "_MB\n";
    const size_t n = c.density.size();
    for (size_t b = 0; b < n; b++) {
      const double v = (b + 0.5) * c.vmax / n;
      out += number(v) + "," + number(c.density[b]) + "," + number(mbDensity(v, c.mass, c.refT)) + "\n";
    }
  }
  return out;
}

std::string msdCsv(const std::vector<MsdCurve>& curves, double fitFrom, double fitTo)
{
  std::string out = "# mean-square displacement of molecule centres of mass\n";
  for (const MsdCurve& c : curves) {
    const DiffusionFit f = fitDiffusion(c.time, c.msd, fitFrom, fitTo);
    out += "# " + c.label + " molecules=" + std::to_string(c.molecules);
    if (f.ok)
      out += " D_1e-5cm2/s=" + number(toE5cm2s(f.D)) + " fit_ps=" + number(f.from) + "-" + number(f.to);
    out += "\ntime_ps," + c.label + "_msd_nm2\n";
    for (size_t i = 0; i < c.time.size(); i++)
      out += number(c.time[i]) + "," + number(c.msd[i]) + "\n";
  }
  return out;
}
