//  Unit tests for the wx-free UV-vis model (include/tdat/ExcitedStates.H and
//  the electronic mode of VibSpectrum.H).
//
//      ./uvvis_test            (exit 0 when every check holds)
//
//  The numbers are ORCA 6.1.1's own table "ABSORPTION SPECTRUM VIA TRANSITION
//  ELECTRIC DIPOLE MOMENTS" in tests/parsers/fixtures/orca/h2co_tddft.out
//  (eV, nm, fosc), and the transitions are the "STATE n" lines of the same
//  file.  Nothing here is compared with another program.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "tdat/ExcitedStates.H"

static int failures = 0, checks = 0;

#define CHECK(cond) do { checks++; if (!(cond)) { failures++; \
  printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

static bool near(double a, double b, double tol)
{
  return std::fabs(a - b) <= tol;
}

static const double EV[10] = { 4.018493, 8.255707, 9.154494, 9.697642,
    10.280808, 11.085267, 11.610178, 11.620493, 13.143637, 13.187152 };
static const double NM[10] = { 308.5, 150.2, 135.4, 127.8, 120.6, 111.8,
    106.8, 106.7, 94.3, 94.0 };                      // printed, one decimal
static const double F[10] = { 0.0, 0.152342124, 0.002342139, 0.004444079, 0.0,
    0.012484217, 0.002710603, 0.538095669, 0.0, 0.131140061 };

static ExcitedStates formaldehyde()
{
  std::vector<double> ev(EV, EV + 10), f(F, F + 10);
  std::vector<std::string> label(10, "Singlet"), trans(10, "-");
  trans[0] = "8a>9a:0.99901186";
  trans[3] = "5a>11a:-0.10717351,7a>9a:0.54516175,8a>11a:-0.82003823";
  ExcitedStates s;
  s.set(ev, f, label, trans);
  return s;
}

static double integralEv(const VibSpectrum& s, double lo, double hi, double dx)
{
  double sum = 0;
  for (double x = lo; x < hi; x += dx) sum += s.envelope(VIB_IR, x + 0.5 * dx) * dx;
  return sum;
}

static void testWavelength()
{
  for (int i = 0; i < 10; i++)
    CHECK(near(ExcitedStates::nm(EV[i]), NM[i], 0.06));
  CHECK(ExcitedStates::nm(0) == 0 && ExcitedStates::nm(-1) == 0);
}

static void testTransitions()
{
  std::vector<ExcitedTransition> t =
      ExcitedStates::parseTransitions("5a>11a:-0.10717351,7a>9a:0.54516175");
  CHECK(t.size() == 2);
  CHECK(t[0].from == "5a" && t[0].to == "11a" && near(t[0].coefficient, -0.10717351, 1e-12));
  CHECK(t[1].from == "7a" && t[1].to == "9a");
  CHECK(ExcitedStates::parseTransitions("-").empty());
  CHECK(ExcitedStates::parseTransitions("").empty());
  //  Largest coefficient first, in the order a person reads.
  const ExcitedStates s = formaldehyde();
  const std::vector<std::string> lines = s.transitionLines(3);
  CHECK(lines.size() == 3 && lines[0].find("8a -> 11a") == 0);
  CHECK(lines[0].find("-0.820") != std::string::npos);
  CHECK(s.transitionLines(1).empty());
}

static void testSticksAndAxis()
{
  const ExcitedStates s = formaldehyde();
  VibSpectrum e = s.spectrum(X_EV);
  CHECK(e.electronic() && e.xUnit() == X_EV);
  std::vector<VibStick> st = e.sticks(VIB_IR);
  CHECK(st.size() == 10);
  for (int i = 0; i < 10; i++) {
    CHECK(st[i].mode == i && near(st[i].wavenumber, EV[i], 1e-12));
    CHECK(near(st[i].intensity, F[i], 1e-12));
    CHECK(!st[i].imaginary);
  }
  CHECK(e.axisLabel(VIB_IR) == "Oscillator strength f");
  CHECK(e.xTitle() == "Excitation energy (eV)");
  CHECK(e.itemWord() == "State");
  CHECK(e.positionText(st[1]) == "8.256 eV (150.2 nm)");

  VibSpectrum n = s.spectrum(X_NM);
  st = n.sticks(VIB_IR);
  CHECK(st.size() == 10);
  for (int i = 0; i < 10; i++)
    CHECK(near(st[i].wavenumber, VibSpectrum::evNm() / EV[i], 1e-9));
  CHECK(n.xTitle() == "Wavelength (nm)");
  CHECK(n.positionText(st[1]) == "8.256 eV (150.2 nm)");

  double lo, hi;
  e.defaultRange(&lo, &hi);
  CHECK(lo <= EV[0] - 0.9 && hi >= EV[9] + 0.9 && lo >= 0);
  CHECK(near(lo * 2, std::floor(lo * 2), 1e-9));      // round numbers
  n.defaultRange(&lo, &hi);
  CHECK(lo < NM[9] && hi > NM[0] && lo >= 0);
  CHECK(near(std::fmod(lo, 10), 0, 1e-9) && near(std::fmod(hi, 10), 0, 1e-9));
}

static void testEnvelope()
{
  const ExcitedStates s = formaldehyde();
  VibSpectrum e = s.spectrum(X_EV);
  VibSpectrum n = s.spectrum(X_NM);
  double total = 0;
  for (int i = 0; i < 10; i++) total += F[i];

  //  Area normalised: the integral over energy is the sum of f.
  e.setFwhm(0.3);
  CHECK(near(integralEv(e, -2, 18, 0.001), total, 2e-4 * total));
  e.setFwhm(0.05);
  CHECK(near(integralEv(e, -2, 18, 0.0005), total, 2e-4 * total));

  //  The same curve on either axis: evaluated in energy.
  n.setFwhm(0.3);
  e.setFwhm(0.3);
  for (int i = 0; i < 10; i++) {
    const double x = EV[i] + 0.07;
    CHECK(near(n.envelope(VIB_IR, VibSpectrum::evNm() / x),
               e.envelope(VIB_IR, x), 1e-12));
  }
  //  At a lone strong band's centre the height is f * 2 sqrt(ln2/pi) / fwhm.
  VibSpectrum lone;
  lone.setXUnit(X_EV);
  lone.setModes(std::vector<double>(1, 5.0), std::vector<std::string>());
  lone.setIntensities(VIB_IR, std::vector<double>(1, 0.5), "f");
  lone.setFwhm(0.3);
  CHECK(near(lone.envelope(VIB_IR, 5.0),
             0.5 * 2 * std::sqrt(std::log(2.0) / 3.14159265358979323846) / 0.3,
             1e-9));
  lone.setFwhm(0);
  CHECK(lone.envelope(VIB_IR, 5.0) == 0);
  //  The y range must reach the tallest curve value.
  e.setFwhm(0.3);
  CHECK(e.maxEnvelope(VIB_IR, 0, 18) > 0.5 * 0.5);
}

static void testCsv()
{
  const ExcitedStates s = formaldehyde();
  VibSpectrum n = s.spectrum(X_NM);
  n.setFwhm(0.3);
  SpectrumCsvInfo info;
  info.calculation = "h2co";
  info.code = "ORCA";
  const std::string csv = excitedSpectrumCsv(s, n, info, 100, 400, 10);
  CHECK(csv.find("# ECCE UV-vis spectrum") == 0);
  CHECK(csv.find("FWHM 0.3 eV") != std::string::npos);
  CHECK(csv.find("state,energy_eV,wavelength_nm,oscillator_strength,label,"
                 "transitions\n") != std::string::npos);
  CHECK(csv.find("\n2,8.255707,150.1") != std::string::npos);
  CHECK(csv.find(",0.152342124,Singlet,-\n") != std::string::npos ||
        csv.find(",0.152342124,Singlet,") != std::string::npos);
  CHECK(csv.find("8a>9a:0.99901186") != std::string::npos);
  CHECK(csv.find("5a>11a:-0.10717351;7a>9a:0.54516175;8a>11a:-0.82003823") !=
        std::string::npos);
  CHECK(csv.find("wavelength_nm,f_per_eV\n100,") != std::string::npos);
  //  31 grid rows from 100 to 400 nm.
  size_t at = csv.find("wavelength_nm,f_per_eV\n"), rows = 0;
  for (size_t p = csv.find('\n', at); p != std::string::npos && p + 1 < csv.size();
       p = csv.find('\n', p + 1)) rows++;
  CHECK(rows == 31);
  n.setFwhm(0);
  CHECK(excitedSpectrumCsv(s, n, info, 100, 400, 10).find("f_per_eV") ==
        std::string::npos);
}

static void testVibrationalUnchanged()
{
  VibSpectrum v;
  CHECK(!v.electronic() && v.xUnit() == X_WAVENUMBER);
  CHECK(v.xTitle() == "Wavenumber (cm\xE2\x81\xBB\xC2\xB9)");
  v.setScale(0.96);
  CHECK(v.xTitle() == "Wavenumber (cm\xE2\x81\xBB\xC2\xB9, scaled by 0.96)");
  CHECK(v.envelopeTitle(VIB_IR) ==
        "IR envelope (km/mol per cm\xE2\x81\xBB\xC2\xB9)");
  CHECK(v.itemWord() == "Mode");
  SpectrumAxis a;
  a.setFull(0, 5);
  CHECK(near(a.span(), SpectrumAxis::minSpan(), 1e-9));
  a.setMinSpan(0.5);
  a.setFull(1, 1.1);
  CHECK(near(a.span(), 0.5, 1e-9));
}

int main()
{
  testWavelength();
  testTransitions();
  testSticksAndAxis();
  testEnvelope();
  testCsv();
  testVibrationalUnchanged();
  printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
