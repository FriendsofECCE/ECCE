#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <locale>
#include <sstream>

#include "tdat/VibSpectrum.H"

using std::max;
using std::min;


// ---- SpectrumAxis --------------------------------------------------------

SpectrumAxis::SpectrumAxis()
  : p_fullLo(0), p_fullHi(4000), p_lo(0), p_hi(4000), p_reversed(true),
    p_minSpan(minSpan())
{
}

void SpectrumAxis::setFull(double lo, double hi)
{
  if (hi < lo) { double t = lo; lo = hi; hi = t; }
  if (hi - lo < p_minSpan) hi = lo + p_minSpan;
  p_fullLo = p_lo = lo;
  p_fullHi = p_hi = hi;
}

bool SpectrumAxis::zoomed() const
{
  return p_lo > p_fullLo + 1e-9 || p_hi < p_fullHi - 1e-9;
}

double SpectrumAxis::toFraction(double wavenumber) const
{
  const double f = (wavenumber - p_lo) / (p_hi - p_lo);
  return p_reversed ? 1.0 - f : f;
}

double SpectrumAxis::fromFraction(double fraction) const
{
  const double f = p_reversed ? 1.0 - fraction : fraction;
  return p_lo + f * (p_hi - p_lo);
}

//  Keeps the width and slides the window back inside the data, so
//  panning stops at the edge instead of showing empty axis beyond it.
void SpectrumAxis::clamp()
{
  const double full = p_fullHi - p_fullLo;
  double width = p_hi - p_lo;
  if (width < p_minSpan) {
    const double mid = 0.5 * (p_lo + p_hi);
    width = min(p_minSpan, full);
    p_lo = mid - 0.5 * width;
    p_hi = mid + 0.5 * width;
  }
  if (width >= full) {
    p_lo = p_fullLo;
    p_hi = p_fullHi;
    return;
  }
  if (p_lo < p_fullLo) { p_lo = p_fullLo; p_hi = p_lo + width; }
  if (p_hi > p_fullHi) { p_hi = p_fullHi; p_lo = p_hi - width; }
}

void SpectrumAxis::zoomTo(double a, double b)
{
  if (b < a) { double t = a; a = b; b = t; }
  p_lo = a;
  p_hi = b;
  clamp();
}

void SpectrumAxis::zoomAbout(double centre, double factor)
{
  p_lo = centre - (centre - p_lo) * factor;
  p_hi = centre + (p_hi - centre) * factor;
  clamp();
}

void SpectrumAxis::pan(double delta)
{
  p_lo += delta;
  p_hi += delta;
  clamp();
}

void SpectrumAxis::reset()
{
  p_lo = p_fullLo;
  p_hi = p_fullHi;
}


// ---- ticks ---------------------------------------------------------------

namespace {

//  1, 2 or 5 times a power of ten, the smallest not below `raw`.
double niceStep(double raw)
{
  if (!(raw > 0)) return 1.0;
  const double p = std::pow(10.0, std::floor(std::log10(raw)));
  const double m = raw / p;
  double n = 10;
  if (m <= 1.0 + 1e-9) n = 1;
  else if (m <= 2.0 + 1e-9) n = 2;
  else if (m <= 5.0 + 1e-9) n = 5;
  return n * p;
}

int decimalsFor(double step)
{
  int d = 0;
  while (d < 6 && std::fabs(step * std::pow(10.0, d) -
                            std::floor(step * std::pow(10.0, d) + 0.5)) > 1e-9)
    d++;
  return d;
}

}

SpectrumTicks niceTicks(double lo, double hi, int target)
{
  SpectrumTicks t;
  t.step = 1;
  t.decimals = 0;
  if (hi < lo) { double x = lo; lo = hi; hi = x; }
  if (target < 2) target = 2;
  if (!(hi > lo)) { t.values.push_back(lo); return t; }

  t.step = niceStep((hi - lo) / target);
  t.decimals = decimalsFor(t.step);
  double first = std::ceil(lo / t.step - 1e-9) * t.step;
  for (int i = 0; i < 1000; i++) {
    double v = first + i * t.step;
    if (v > hi + t.step * 1e-9) break;
    //  -0 would print as "-0".
    if (std::fabs(v) < t.step * 1e-9) v = 0.0;
    t.values.push_back(v);
  }
  return t;
}

SpectrumYAxis niceYAxis(double maximum, int target)
{
  SpectrumYAxis y;
  if (!(maximum > 0)) maximum = 1.0;
  if (target < 2) target = 2;
  const double step = niceStep(maximum / target);
  y.top = std::ceil(maximum / step - 1e-9) * step;
  y.ticks = niceTicks(0, y.top, (int)std::ceil(y.top / step - 1e-9) + 1);
  y.ticks.step = step;
  y.ticks.decimals = decimalsFor(step);
  return y;
}


// ---- VibSpectrum ---------------------------------------------------------

VibSpectrum::VibSpectrum()
  : p_x(X_WAVENUMBER), p_scale(1.0), p_fwhm(15.0), p_shape(LINE_GAUSSIAN)
{
  for (int k = 0; k < 2; k++) { p_has[k] = false; p_relative[k] = false; }
}

void VibSpectrum::setModes(const vector<double>& wavenumbers,
                           const vector<string>& irreps)
{
  p_freq = wavenumbers;
  p_irrep.assign(p_freq.size(), string());
  for (size_t i = 0; i < irreps.size() && i < p_irrep.size(); i++)
    p_irrep[i] = irreps[i];
  for (int k = 0; k < 2; k++) {
    p_inten[k].clear();
    p_has[k] = false;
    p_relative[k] = false;
    p_units[k].clear();
  }
}

static string upper(const string& s)
{
  string u;
  for (size_t i = 0; i < s.size(); i++)
    if (!std::isspace((unsigned char)s[i]))
      u += (char)std::toupper((unsigned char)s[i]);
  return u;
}

bool VibSpectrum::knownIntensityUnits(VibKind kind, const string& units)
{
  const string u = upper(units);
  if (kind == VIB_IR) return u == "KM/MOLE" || u == "KM/MOL";
  //  Gaussian writes A^4/AMU, ORCA A**4/AMU.
  return u == "A^4/AMU" || u == "A**4/AMU" || u == "A4/AMU";
}

void VibSpectrum::setIntensities(VibKind kind, const vector<double>& values,
                                 const string& units)
{
  p_inten[kind] = values;
  p_inten[kind].resize(p_freq.size(), 0.0);
  //  All zeros is a code saying "not computed" (Gaussian writes zero
  //  Raman activities without Raman=), not a spectrum with no bands.
  p_has[kind] = false;
  for (size_t i = 0; i < values.size(); i++)
    if (values[i] != 0.0) p_has[kind] = true;
  p_units[kind] = units;
  p_relative[kind] = electronic() ? upper(units) != "F"
                                  : !knownIntensityUnits(kind, units);
}

string VibSpectrum::unitLabel(VibKind kind) const
{
  if (p_relative[kind]) return "relative";
  if (electronic()) return "f";
  return kind == VIB_IR ? "km/mol" : "\xC3\x85\xE2\x81\xB4/amu";
}

string VibSpectrum::axisLabel(VibKind kind) const
{
  if (electronic())
    return p_relative[kind] ? "Absorption (relative)" : "Oscillator strength f";
  const string name = kind == VIB_IR ? "IR" : "Raman";
  if (p_relative[kind]) return name + " intensity (relative)";
  return name + (kind == VIB_IR ? " intensity (" : " activity (") +
         unitLabel(kind) + ")";
}

bool VibSpectrum::isVibration(int i) const
{
  if (electronic()) return p_freq[i] > 0;
  return std::fabs(p_freq[i]) >= zeroTolerance();
}

bool VibSpectrum::isImaginary(int i) const
{
  return !electronic() && p_freq[i] <= -zeroTolerance();
}

//  What is drawn for mode i: the code's number, or, when its unit is
//  not known, the number divided by the largest so the pane still has
//  a scale to read.
double VibSpectrum::drawn(VibKind kind, int i) const
{
  const double v = p_inten[kind][i];
  if (!p_relative[kind]) return v;
  double top = 0;
  for (size_t j = 0; j < p_inten[kind].size(); j++)
    if (isVibration((int)j)) top = max(top, std::fabs(p_inten[kind][j]));
  return top > 0 ? v / top : 0.0;
}

vector<VibStick> VibSpectrum::sticks(VibKind kind) const
{
  vector<VibStick> out;
  if (!p_has[kind]) return out;
  for (int i = 0; i < (int)p_freq.size(); i++) {
    if (!isVibration(i)) continue;
    VibStick s;
    s.mode = i;
    s.wavenumber = wavenumber(i);
    s.intensity = drawn(kind, i);
    s.irrep = p_irrep[i];
    s.imaginary = isImaginary(i);
    out.push_back(s);
  }
  return out;
}

double VibSpectrum::lineValue(LineShape shape, double offset, double fwhm)
{
  if (!(fwhm > 0)) return 0.0;
  const double pi = 3.14159265358979323846;
  if (shape == LINE_LORENTZIAN) {
    const double g = 0.5 * fwhm;
    return (g / pi) / (offset * offset + g * g);
  }
  const double ln2 = std::log(2.0);
  const double norm = 2.0 * std::sqrt(ln2 / pi) / fwhm;
  const double x = offset / fwhm;
  return norm * std::exp(-4.0 * ln2 * x * x);
}

double VibSpectrum::envelope(VibKind kind, double wn) const
{
  if (!p_has[kind] || !(p_fwhm > 0)) return 0.0;
  double sum = 0;
  for (int i = 0; i < (int)p_freq.size(); i++) {
    if (!isVibration(i) || isImaginary(i)) continue;
    sum += drawn(kind, i) *
           lineValue(p_shape, nativeOf(wn) - p_freq[i] * p_scale, p_fwhm);
  }
  return sum;
}

void VibSpectrum::setCompare(const VibSpectrum& other, const string& label)
{
  p_cmp.reset(new VibSpectrum(other));
  p_cmp->p_cmp.reset();
  p_cmp->p_scale = p_scale;
  p_cmp->p_fwhm = p_fwhm;
  p_cmp->p_shape = p_shape;
  p_cmpLabel = label;
}

//  Both known units (km/mol) or both relative: as printed.  Otherwise the
//  second is stretched so its tallest stick equals this one's.
double VibSpectrum::compareFactor(VibKind kind) const
{
  if (!hasCompare(kind) || !p_has[kind]) return 1.0;
  if (p_relative[kind] == p_cmp->p_relative[kind]) return 1.0;
  double mine = 0, theirs = 0;
  const vector<VibStick> a = sticks(kind), b = p_cmp->sticks(kind);
  for (size_t i = 0; i < a.size(); i++) mine = max(mine, a[i].intensity);
  for (size_t i = 0; i < b.size(); i++) theirs = max(theirs, b[i].intensity);
  return theirs > 0 && mine > 0 ? mine / theirs : 1.0;
}

vector<VibStick> VibSpectrum::compareSticks(VibKind kind) const
{
  vector<VibStick> out;
  if (!hasCompare(kind)) return out;
  out = p_cmp->sticks(kind);
  const double f = compareFactor(kind);
  for (size_t i = 0; i < out.size(); i++) out[i].intensity *= f;
  return out;
}

double VibSpectrum::compareEnvelope(VibKind kind, double wn) const
{
  if (!hasCompare(kind)) return 0.0;
  return p_cmp->envelope(kind, wn) * compareFactor(kind);
}

double VibSpectrum::maxStick(VibKind kind, double lo, double hi) const
{
  double m = 0;
  const vector<VibStick> s = sticks(kind);
  for (size_t i = 0; i < s.size(); i++)
    if (s[i].wavenumber >= lo && s[i].wavenumber <= hi)
      m = max(m, s[i].intensity);
  const vector<VibStick> c = compareSticks(kind);
  for (size_t i = 0; i < c.size(); i++)
    if (c[i].wavenumber >= lo && c[i].wavenumber <= hi)
      m = max(m, c[i].intensity);
  return m;
}

double VibSpectrum::maxEnvelope(VibKind kind, double lo, double hi) const
{
  if (!(p_fwhm > 0) || !(hi > lo) || (!p_has[kind] && !hasCompare(kind)))
    return 0.0;
  //  Sampled finely enough to find the top of the narrowest line, and at
  //  each band centre, which is where a lone line peaks.
  //  The width is in eV on an nm axis too, so there it cannot set the grid.
  const int n = electronic() ? 4000
      : (int)min(20000.0, max(200.0, 8.0 * (hi - lo) / p_fwhm));
  double m = 0;
  for (int i = 0; i <= n; i++) {
    const double x = lo + (hi - lo) * i / n;
    m = max(m, max(envelope(kind, x), compareEnvelope(kind, x)));
  }
  for (int i = 0; i < (int)p_freq.size(); i++) {
    const double c = wavenumber(i);
    if (c >= lo && c <= hi) m = max(m, envelope(kind, c));
  }
  if (p_cmp)
    for (int i = 0; i < (int)p_cmp->p_freq.size(); i++) {
      const double c = p_cmp->wavenumber(i);
      if (c >= lo && c <= hi) m = max(m, compareEnvelope(kind, c));
    }
  return m;
}

//  Round numbers around the states: 0.5 eV or 10 nm steps, at least 3 widths
//  of margin so a band at the edge is not cut off.
void VibSpectrum::electronicRange(double minPos, double maxPos, bool any,
                                  double *lo, double *hi) const
{
  //  minPos/maxPos are positions in the axis unit, as defaultRange collects.
  if (!any) { *lo = p_x == X_NM ? 100 : 0; *hi = p_x == X_NM ? 800 : 10; return; }
  const double pad = max(3.0 * p_fwhm, 0.3);         // eV
  if (p_x == X_EV) {
    *lo = max(0.0, std::floor((minPos - pad) / 0.5) * 0.5);
    *hi = std::ceil((maxPos + pad) / 0.5) * 0.5;
  } else {
    //  nm: the energy margin, mapped; lowest energy gives the longest wave.
    const double eLo = max(0.2, toEv(maxPos) - pad), eHi = toEv(minPos) + pad;
    *lo = std::floor(fromEv(eHi) / 10.0) * 10.0;
    *hi = std::ceil(fromEv(eLo) / 10.0) * 10.0;
    if (*lo < 0) *lo = 0;
  }
}

void VibSpectrum::defaultRange(double *lo, double *hi) const
{
  double minReal = 1e30, maxReal = -1e30, minImag = 0;
  bool any = false;
  for (int i = 0; i < (int)p_freq.size(); i++) {
    if (!isVibration(i)) continue;
    const double w = wavenumber(i);
    any = true;
    if (isImaginary(i)) minImag = min(minImag, w);
    else { minReal = min(minReal, w); maxReal = max(maxReal, w); }
  }
  if (p_cmp)
    for (int i = 0; i < (int)p_cmp->p_freq.size(); i++) {
      if (!p_cmp->isVibration(i)) continue;
      const double w = p_cmp->wavenumber(i);
      any = true;
      if (p_cmp->isImaginary(i)) minImag = min(minImag, w);
      else { minReal = min(minReal, w); maxReal = max(maxReal, w); }
    }
  if (electronic()) { electronicRange(minReal, maxReal, any, lo, hi); return; }
  if (!any || maxReal < 0) { *lo = 0; *hi = 4000; return; }
  //  The margin grows with the range: 100 cm-1 is a few pixels in the
  //  docked panel, and a band at the edge looked cut off.
  const double pad = max(max(100.0, 4.0 * p_fwhm), 0.1 * (maxReal - minReal));
  //  The whole usual IR range (0 to 3400 at least), so a diatomic is not
  //  blown up to a narrow window around its one band.
  double h = max(3400.0, std::ceil((maxReal + pad) / 100.0) * 100.0);
  double l = 0;
  if (minImag < 0) l = std::floor((minImag - pad) / 100.0) * 100.0;
  *lo = l;
  *hi = h;
}

//  A calculation's name as a column heading: nothing that splits a field.
static string headingOf(const string& name, const char *fallback)
{
  string t;
  for (size_t i = 0; i < name.size(); i++)
    t += (name[i] == ',' || name[i] == '"' || name[i] == '\n') ? '_' : name[i];
  return t.empty() ? string(fallback) : t;
}

string vibSpectrumCsv(const VibSpectrum& s, VibKind kind,
                      const SpectrumCsvInfo& info,
                      double lo, double hi, double step)
{
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out.precision(8);
  const bool curve = s.fwhm() > 0 && step > 0 && hi > lo;
  const bool second = s.hasCompare(kind);
  const string nameA = headingOf(info.calculation, "first");
  const string nameB = headingOf(s.compareLabel(), "second");

  out << "# ECCE vibrational spectrum\n";
  out << "# calculation: " << info.calculation << "\n";
  out << "# code: " << info.code << "\n";
  out << "# spectrum: " << (kind == VIB_IR ? "IR" : "Raman") << "\n";
  out << "# intensity unit: " << s.unitLabel(kind) << "\n";
  out << "# frequency scaling factor: " << s.scale() << "\n";
  out << "# broadening: "
      << (curve ? (s.lineShape() == LINE_LORENTZIAN ? "Lorentzian" : "Gaussian")
                : "none")
      << ", FWHM " << s.fwhm() << " cm-1\n";
  if (second) {
    out << "# compared with: " << s.compareLabel();
    if (!info.compareCode.empty()) out << " (" << info.compareCode << ")";
    out << ", same scaling factor and broadening\n";
  }
  out << "# wavenumbers are scaled; the curve is intensity per cm-1\n";
  out << "# sticks\n";
  out << "mode,frequency_cm-1";
  if (s.has(VIB_IR)) out << ",ir_intensity";
  if (s.has(VIB_RAMAN)) out << ",raman_activity";
  out << ",symmetry\n";

  const vector<VibStick> ir = s.has(VIB_IR) ? s.sticks(VIB_IR) : vector<VibStick>();
  const vector<VibStick> ra = s.has(VIB_RAMAN) ? s.sticks(VIB_RAMAN) : vector<VibStick>();
  const vector<VibStick>& base = s.has(kind) ? (kind == VIB_IR ? ir : ra) : ir;
  for (size_t i = 0; i < base.size(); i++) {
    out << base[i].mode + 1 << "," << base[i].wavenumber;
    if (s.has(VIB_IR))
      for (size_t j = 0; j < ir.size(); j++)
        if (ir[j].mode == base[i].mode) out << "," << ir[j].intensity;
    if (s.has(VIB_RAMAN))
      for (size_t j = 0; j < ra.size(); j++)
        if (ra[j].mode == base[i].mode) out << "," << ra[j].intensity;
    out << "," << base[i].irrep << "\n";
  }

  if (second) {
    out << "\n# sticks of " << s.compareLabel() << "\n";
    out << "mode,frequency_cm-1,intensity,symmetry\n";
    const vector<VibStick> cs = s.compareSticks(kind);
    for (size_t i = 0; i < cs.size(); i++)
      out << cs[i].mode + 1 << "," << cs[i].wavenumber << ","
          << cs[i].intensity << "," << cs[i].irrep << "\n";
  }

  if (curve) {
    out << "\n# broadened curve, " << step << " cm-1 grid from " << lo
        << " to " << hi << "\n";
    if (second)
      out << "wavenumber_cm-1,intensity_" << nameA << ",intensity_" << nameB
          << "\n";
    else
      out << "wavenumber_cm-1,intensity\n";
    const long n = (long)std::floor((hi - lo) / step + 1e-9);
    for (long i = 0; i <= n; i++) {
      const double x = lo + i * step;
      out << x << "," << s.envelope(kind, x);
      if (second) out << "," << s.compareEnvelope(kind, x);
      out << "\n";
    }
  }
  return out.str();
}


// ---- units and wording for the electronic spectrum (#383) ------------------

double VibSpectrum::toEv(double x) const
{
  if (p_x != X_NM) return x;
  return x > 1e-9 ? evNm() / x : 1e9;
}

double VibSpectrum::fromEv(double ev) const
{
  if (p_x != X_NM) return ev;
  return ev > 1e-9 ? evNm() / ev : 1e9;
}

string VibSpectrum::positionUnit() const
{
  if (p_x == X_EV) return "eV";
  if (p_x == X_NM) return "nm";
  return "cm\xE2\x81\xBB\xC2\xB9";
}

string VibSpectrum::xTitle() const
{
  if (p_x == X_EV) return "Excitation energy (eV)";
  if (p_x == X_NM) return "Wavelength (nm)";
  string t = "Wavenumber (" + positionUnit();
  if (std::fabs(p_scale - 1.0) > 1e-9) {
    char b[48];
    snprintf(b, sizeof b, ", scaled by %g", p_scale);
    t += b;
  }
  return t + ")";
}

string VibSpectrum::envelopeTitle(VibKind kind) const
{
  if (electronic())
    return string("Absorption envelope (") +
           (p_relative[kind] ? "relative" : "f") + " per eV)";
  const string name = kind == VIB_IR ? "IR" : "Raman";
  return name + " envelope (" + (p_relative[kind] ? string("relative")
                                                  : unitLabel(kind)) +
         " per cm\xE2\x81\xBB\xC2\xB9)";
}

string VibSpectrum::itemWord() const { return electronic() ? "State" : "Mode"; }

string VibSpectrum::positionText(const VibStick& stick) const
{
  char b[96];
  if (electronic()) {
    const double ev = toEv(stick.wavenumber);
    snprintf(b, sizeof b, "%.3f eV (%.1f nm)", ev, evNm() / ev);
  } else {
    snprintf(b, sizeof b, "%.1f cm\xE2\x81\xBB\xC2\xB9", stick.wavenumber);
  }
  return b;
}

string VibSpectrum::intensityName(VibKind kind) const
{
  if (electronic()) return "f  ";
  return kind == VIB_IR ? "IR  " : "Raman  ";
}

string VibSpectrum::emptyMessage() const
{
  return electronic() ? "No excited state has a nonzero oscillator strength."
                      : "This calculation has no infrared or Raman intensities.";
}
