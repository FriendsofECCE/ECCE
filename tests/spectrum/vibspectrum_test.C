//  Unit tests for the wx-free spectrum model (include/tdat/VibSpectrum.H).
//
//      ./vibspectrum_test            (exit 0 when every check holds)
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "tdat/VibSpectrum.H"

static int failures = 0, checks = 0;

#define CHECK(cond) do { checks++; if (!(cond)) { failures++; \
  printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

static bool near(double a, double b, double tol)
{
  return std::fabs(a - b) <= tol;
}

static double integrate(const VibSpectrum& s, double lo, double hi, double dx)
{
  double sum = 0;
  for (double x = lo; x < hi; x += dx)
    sum += s.envelope(VIB_IR, x + 0.5 * dx) * dx;
  return sum;
}

static VibSpectrum water()
{
  //  Gaussian 16 water, tests/parsers/fixtures/gaussian-16/h2o_optfreq.log
  VibSpectrum s;
  std::vector<double> f, ir, ra;
  std::vector<std::string> sym;
  f.push_back(2169.8577); f.push_back(4141.5426); f.push_back(4392.6459);
  ir.push_back(7.2460);   ir.push_back(44.2829);  ir.push_back(29.9543);
  ra.push_back(9.2633);   ra.push_back(47.7911);  ra.push_back(21.5432);
  sym.push_back("A'");    sym.push_back("A'");    sym.push_back("A'");
  s.setModes(f, sym);
  s.setIntensities(VIB_IR, ir, "KM/Mole");
  s.setIntensities(VIB_RAMAN, ra, "A^4/AMU");
  return s;
}

static void testTicks()
{
  SpectrumTicks t = niceTicks(0, 4000, 8);
  CHECK(t.step == 500);
  CHECK(t.values.size() == 9 && t.values.front() == 0 &&
        t.values.back() == 4000);
  CHECK(t.decimals == 0);

  t = niceTicks(1445, 4165, 8);
  CHECK(t.step == 500);
  CHECK(t.values.front() == 1500 && t.values.back() == 4000);

  t = niceTicks(1600, 1700, 6);
  CHECK(t.step == 20);
  CHECK(t.values.front() == 1600 && t.values.back() == 1700);

  t = niceTicks(0, 1, 5);
  CHECK(near(t.step, 0.2, 1e-12) && t.decimals == 1);
  t = niceTicks(0, 0.3, 6);
  CHECK(near(t.step, 0.05, 1e-12) && t.decimals == 2);

  //  Zero is written as 0, not -0.
  t = niceTicks(-300, 300, 6);
  bool zero = false;
  for (size_t i = 0; i < t.values.size(); i++)
    if (t.values[i] == 0.0 && !std::signbit(t.values[i])) zero = true;
  CHECK(zero);

  //  Every tick lies in range, and there are never many more than asked.
  for (double lo = 0; lo < 3000; lo += 137) {
    for (double w = 25; w < 4000; w *= 1.7) {
      t = niceTicks(lo, lo + w, 8);
      CHECK(t.values.empty() ||
            (t.values.front() >= lo - 1e-9 &&
             t.values.back() <= lo + w + 1e-9));
      CHECK(t.values.size() <= 16);
    }
  }

  SpectrumYAxis y = niceYAxis(44.2829, 5);
  CHECK(y.top >= 44.2829 && y.top <= 60);
  CHECK(y.ticks.values.front() == 0 &&
        near(y.ticks.values.back(), y.top, 1e-9));
  y = niceYAxis(100, 5);
  CHECK(y.top == 100);
  y = niceYAxis(0, 5);
  CHECK(y.top > 0);
  //  The tallest feature is never clipped and the axis is not wasteful.
  for (double m = 0.001; m < 1e5; m *= 1.37) {
    y = niceYAxis(m, 5);
    CHECK(y.top >= m * (1 - 1e-12));
    CHECK(y.top <= 1.51 * m);
  }
}

static void testAxis()
{
  SpectrumAxis a;
  a.setFull(400, 4000);
  CHECK(a.reversed());                         // default: high on the left
  CHECK(near(a.toFraction(4000), 0.0, 1e-12)); // 4000 at the left edge
  CHECK(near(a.toFraction(400), 1.0, 1e-12));
  CHECK(near(a.toFraction(2200), 0.5, 1e-12));
  CHECK(near(a.fromFraction(0.0), 4000, 1e-9));
  CHECK(near(a.fromFraction(a.toFraction(1234.5)), 1234.5, 1e-9));
  a.setReversed(false);
  CHECK(near(a.toFraction(400), 0.0, 1e-12));
  CHECK(near(a.toFraction(4000), 1.0, 1e-12));
  CHECK(near(a.fromFraction(a.toFraction(1234.5)), 1234.5, 1e-9));
  a.setReversed(true);

  CHECK(!a.zoomed());
  a.zoomTo(1800, 1500);                        // either order
  CHECK(a.lo() == 1500 && a.hi() == 1800 && a.zoomed());
  CHECK(near(a.toFraction(1800), 0.0, 1e-12)); // still reversed
  a.reset();
  CHECK(a.lo() == 400 && a.hi() == 4000 && !a.zoomed());

  //  Zoom about the cursor keeps the wavenumber under it fixed.
  a.zoomAbout(1000, 0.5);
  CHECK(near(a.lo(), 700, 1e-9) && near(a.hi(), 2500, 1e-9));
  const double f = a.toFraction(1000);
  a.zoomAbout(1000, 0.5);
  CHECK(near(a.toFraction(1000), f, 1e-9));
  //  Zooming out stops at the data.
  a.zoomAbout(1000, 100);
  CHECK(a.lo() == 400 && a.hi() == 4000);
  //  Narrowest window.
  a.zoomAbout(1000, 1e-9);
  CHECK(near(a.span(), SpectrumAxis::minSpan(), 1e-9));
  //  Pan keeps the width and stops at the edge.
  a.reset();
  a.zoomTo(1000, 1500);
  a.pan(300);
  CHECK(near(a.lo(), 1300, 1e-9) && near(a.hi(), 1800, 1e-9));
  a.pan(1e6);
  CHECK(near(a.hi(), 4000, 1e-9) && near(a.span(), 500, 1e-9));
  a.pan(-1e6);
  CHECK(near(a.lo(), 400, 1e-9) && near(a.span(), 500, 1e-9));
  a.zoomTo(-50, 99999);                        // clipped to the data
  CHECK(a.lo() == 400 && a.hi() == 4000);
}

static void testLines()
{
  for (int shape = 0; shape < 2; shape++) {
    const LineShape ls = shape ? LINE_LORENTZIAN : LINE_GAUSSIAN;
    for (double fwhm = 5; fwhm <= 100; fwhm *= 2) {
      //  Half the peak height half a FWHM away: that is what FWHM means.
      const double peak = VibSpectrum::lineValue(ls, 0, fwhm);
      CHECK(near(VibSpectrum::lineValue(ls, 0.5 * fwhm, fwhm), 0.5 * peak,
                 1e-12));
      CHECK(near(VibSpectrum::lineValue(ls, -0.5 * fwhm, fwhm), 0.5 * peak,
                 1e-12));
    }
    //  Area under the bands is the sum of the stick intensities, at any
    //  width.
    for (double fwhm = 2; fwhm <= 60; fwhm *= 3) {
      VibSpectrum s = water();
      s.setLineShape(ls);
      s.setFwhm(fwhm);
      const double want = 7.2460 + 44.2829 + 29.9543;
      const double got = integrate(s, 4141.5426 - 6000, 4141.5426 + 6000,
                                   fwhm / 20);
      CHECK(near(got / want, 1.0, shape ? 0.004 : 0.0005));
    }
  }

  //  Gaussian peak height of an isolated band: I * 2 sqrt(ln2/pi) / fwhm.
  VibSpectrum one;
  std::vector<double> f(1, 1700.0), i(1, 10.0);
  one.setModes(f, std::vector<std::string>());
  one.setIntensities(VIB_IR, i, "KM/Mole");
  one.setFwhm(20);
  CHECK(near(one.envelope(VIB_IR, 1700), 10.0 * 0.9394372787 / 20, 1e-8));
  CHECK(near(one.maxEnvelope(VIB_IR, 1000, 2000), 10.0 * 0.9394372787 / 20,
             1e-8));
  //  Narrower line, taller peak, same area.
  one.setFwhm(10);
  CHECK(near(one.envelope(VIB_IR, 1700), 10.0 * 0.9394372787 / 10, 1e-8));

  //  Overlapping bands add.
  VibSpectrum two;
  std::vector<double> f2, i2;
  f2.push_back(1700); f2.push_back(1710);
  i2.push_back(10);   i2.push_back(4);
  two.setModes(f2, std::vector<std::string>());
  two.setIntensities(VIB_IR, i2, "KM/Mole");
  two.setFwhm(20);
  const double sum = 10 * VibSpectrum::lineValue(LINE_GAUSSIAN, 5, 20) +
                     4 * VibSpectrum::lineValue(LINE_GAUSSIAN, -5, 20);
  CHECK(near(two.envelope(VIB_IR, 1705), sum, 1e-12));

  //  FWHM 0: sticks only.
  two.setFwhm(0);
  CHECK(two.envelope(VIB_IR, 1700) == 0.0);
  CHECK(two.maxEnvelope(VIB_IR, 1000, 2000) == 0.0);
  CHECK(two.sticks(VIB_IR).size() == 2);
  //  No such spectrum, no envelope.
  CHECK(two.envelope(VIB_RAMAN, 1700) == 0.0 && !two.has(VIB_RAMAN));
  CHECK(two.sticks(VIB_RAMAN).empty());
}

static void testSticks()
{
  VibSpectrum s = water();
  std::vector<VibStick> ir = s.sticks(VIB_IR);
  CHECK(ir.size() == 3);
  CHECK(ir[1].mode == 1 && near(ir[1].wavenumber, 4141.5426, 1e-9));
  CHECK(near(ir[1].intensity, 44.2829, 1e-9) && ir[1].irrep == "A'");
  CHECK(near(s.sticks(VIB_RAMAN)[2].intensity, 21.5432, 1e-9));

  //  Scaling moves the sticks, not the intensities.
  s.setScale(0.96);
  ir = s.sticks(VIB_IR);
  CHECK(near(ir[1].wavenumber, 4141.5426 * 0.96, 1e-9));
  CHECK(near(ir[1].intensity, 44.2829, 1e-9));
  s.setScale(0);                               // nonsense -> 1
  CHECK(s.scale() == 1.0);

  //  Translations/rotations (ORCA, NWChem) get no stick; the mode numbers
  //  that remain still address rows of the table.
  VibSpectrum o;
  const double of[] = {0, 0, 0, 0, 0, 0, 1789.30, 3978.84, 4068.57};
  const double oi[] = {0, 0, 0, 0, 0, 0, 80.07, 25.44, 68.75};
  o.setModes(std::vector<double>(of, of + 9), std::vector<std::string>());
  o.setIntensities(VIB_IR, std::vector<double>(oi, oi + 9), "KM/Mole");
  ir = o.sticks(VIB_IR);
  CHECK(ir.size() == 3 && ir[0].mode == 6 && ir[2].mode == 8);
  CHECK(ir[0].irrep.empty());
  const double nf[] = {-1.09717e-05, 0, 0, 0, 8.33e-06, 5.05e-05,
                       2043.289, 4488.45, 4767.585};
  o.setModes(std::vector<double>(nf, nf + 9), std::vector<std::string>());
  o.setIntensities(VIB_IR, std::vector<double>(oi, oi + 9), "KM/Mole");
  CHECK(o.sticks(VIB_IR).size() == 3);

  //  Imaginary modes are kept and flagged, and do not feed the envelope.
  VibSpectrum im;
  const double mf[] = {-250.0, 1500.0};
  const double mi[] = {30.0, 10.0};
  im.setModes(std::vector<double>(mf, mf + 2), std::vector<std::string>());
  im.setIntensities(VIB_IR, std::vector<double>(mi, mi + 2), "KM/Mole");
  ir = im.sticks(VIB_IR);
  CHECK(ir.size() == 2 && ir[0].imaginary && !ir[1].imaginary);
  CHECK(near(ir[0].wavenumber, -250.0, 1e-12));
  im.setFwhm(15);
  CHECK(im.envelope(VIB_IR, -250.0) < 1e-12);
  CHECK(im.envelope(VIB_IR, 1500.0) > 0);
  double lo, hi;
  im.defaultRange(&lo, &hi);
  CHECK(lo <= -250 && hi >= 1500);

  //  Unknown units: relative, largest stick 1.
  VibSpectrum r;
  r.setModes(std::vector<double>(mf + 1, mf + 2), std::vector<std::string>());
  const double ri[] = {37.0};
  r.setIntensities(VIB_RAMAN, std::vector<double>(ri, ri + 1), "ARBITRARY");
  CHECK(r.relative(VIB_RAMAN));
  CHECK(near(r.sticks(VIB_RAMAN)[0].intensity, 1.0, 1e-12));
  CHECK(r.axisLabel(VIB_RAMAN) == "Raman intensity (relative)");

  //  All-zero intensities mean the code did not compute them.
  VibSpectrum z;
  const double zero[] = {0.0, 0.0};
  z.setModes(std::vector<double>(mf, mf + 2), std::vector<std::string>());
  z.setIntensities(VIB_RAMAN, std::vector<double>(zero, zero + 2), "A^4/AMU");
  CHECK(!z.has(VIB_RAMAN) && z.sticks(VIB_RAMAN).empty());

  //  Units as the codes write them.
  VibSpectrum u = water();
  CHECK(u.axisLabel(VIB_IR) == "IR intensity (km/mol)");
  CHECK(u.unitLabel(VIB_RAMAN) == "\xC3\x85\xE2\x81\xB4/amu");
  CHECK(VibSpectrum::knownIntensityUnits(VIB_RAMAN, "A**4/AMU"));
  CHECK(VibSpectrum::knownIntensityUnits(VIB_RAMAN, "A^4/AMU"));
  CHECK(VibSpectrum::knownIntensityUnits(VIB_IR, "KM/Mole"));
  CHECK(!VibSpectrum::knownIntensityUnits(VIB_IR, "NA"));
  CHECK(!u.relative(VIB_IR));

  //  Maxima within a window.
  CHECK(near(u.maxStick(VIB_IR, 0, 3000), 7.2460, 1e-12));
  CHECK(near(u.maxStick(VIB_IR, 3000, 5000), 44.2829, 1e-12));
  CHECK(u.maxStick(VIB_IR, 100, 200) == 0.0);

  //  Default range: modes plus a margin, on round numbers.
  double l, h;
  water().defaultRange(&l, &h);
  CHECK(l == 2000 && h == 4500);
}

int main()
{
  testTicks();
  testAxis();
  testLines();
  testSticks();
  printf("%d checks, %d failed\n", checks, failures);
  return failures ? 1 : 0;
}
