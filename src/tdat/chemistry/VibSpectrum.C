#include <algorithm>
#include <cctype>
#include <cmath>

#include "tdat/VibSpectrum.H"

using std::max;
using std::min;


// ---- SpectrumAxis --------------------------------------------------------

SpectrumAxis::SpectrumAxis()
  : p_fullLo(0), p_fullHi(4000), p_lo(0), p_hi(4000), p_reversed(true)
{
}

void SpectrumAxis::setFull(double lo, double hi)
{
  if (hi < lo) { double t = lo; lo = hi; hi = t; }
  if (hi - lo < minSpan()) hi = lo + minSpan();
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
  if (width < minSpan()) {
    const double mid = 0.5 * (p_lo + p_hi);
    width = min(minSpan(), full);
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
  : p_scale(1.0), p_fwhm(15.0), p_shape(LINE_GAUSSIAN)
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
  p_relative[kind] = !knownIntensityUnits(kind, units);
}

string VibSpectrum::unitLabel(VibKind kind) const
{
  if (p_relative[kind]) return "relative";
  return kind == VIB_IR ? "km/mol" : "\xC3\x85\xE2\x81\xB4/amu";
}

string VibSpectrum::axisLabel(VibKind kind) const
{
  const string name = kind == VIB_IR ? "IR" : "Raman";
  if (p_relative[kind]) return name + " intensity (relative)";
  return name + (kind == VIB_IR ? " intensity (" : " activity (") +
         unitLabel(kind) + ")";
}

bool VibSpectrum::isVibration(int i) const
{
  return std::fabs(p_freq[i]) >= zeroTolerance();
}

bool VibSpectrum::isImaginary(int i) const
{
  return p_freq[i] <= -zeroTolerance();
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
    sum += drawn(kind, i) * lineValue(p_shape, wn - wavenumber(i), p_fwhm);
  }
  return sum;
}

double VibSpectrum::maxStick(VibKind kind, double lo, double hi) const
{
  double m = 0;
  const vector<VibStick> s = sticks(kind);
  for (size_t i = 0; i < s.size(); i++)
    if (s[i].wavenumber >= lo && s[i].wavenumber <= hi)
      m = max(m, s[i].intensity);
  return m;
}

double VibSpectrum::maxEnvelope(VibKind kind, double lo, double hi) const
{
  if (!p_has[kind] || !(p_fwhm > 0) || !(hi > lo)) return 0.0;
  //  Sampled finely enough to find the top of the narrowest line, and at
  //  each band centre, which is where a lone line peaks.
  const int n = (int)min(20000.0, max(200.0, 8.0 * (hi - lo) / p_fwhm));
  double m = 0;
  for (int i = 0; i <= n; i++)
    m = max(m, envelope(kind, lo + (hi - lo) * i / n));
  for (int i = 0; i < (int)p_freq.size(); i++) {
    const double c = wavenumber(i);
    if (c >= lo && c <= hi) m = max(m, envelope(kind, c));
  }
  return m;
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
  if (!any || maxReal < 0) { *lo = 0; *hi = 4000; return; }
  const double pad = max(100.0, 4.0 * p_fwhm);
  double h = std::ceil((maxReal + pad) / 100.0) * 100.0;
  double l = std::floor((minReal - pad) / 100.0) * 100.0;
  if (l < 0) l = 0;
  if (minImag < 0) l = std::floor((minImag - pad) / 100.0) * 100.0;
  *lo = l;
  *hi = h;
}
