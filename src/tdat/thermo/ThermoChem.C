#include "tdat/thermo/ThermoChem.H"
#include "util/VibFrequency.H"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {

const double PI = 3.14159265358979323846;

// CODATA 2018.
const double K_B = 1.380649e-23;         // J/K
const double N_A = 6.02214076e23;        // 1/mol
const double R_ = K_B * N_A;             // J/mol/K
const double EH = 4.3597447222071e-18;   // J, one hartree
const double AMU = 1.66053906660e-27;    // kg
const double C_CM = 2.99792458e10;       // cm/s, for h*c*nu and kT/(h c B)
const double P_ATM = 101325.0;           // Pa

// h*c/k in cm*K: the frequency to temperature factor.
const double HC_K = 6.62607015e-34 * 2.99792458e10 / 1.380649e-23;

std::string num(double v, int d)
{
  char b[64];
  snprintf(b, sizeof b, "%.*f", d, v);
  return b;
}

// Harmonic-oscillator entropy of one mode, in units of R.
double sHo(double nu, double T)
{
  const double x = HC_K * nu / T;
  return x / (std::exp(x) - 1.0) - std::log(1.0 - std::exp(-x));
}

// Grimme's free-rotor entropy of one mode (1-D rotor with B = nu), units of R.
double sFr(double nu, double T)
{
  return 0.5 + 0.5 * std::log(PI * K_B * T / (6.62607015e-34 * C_CM * nu));
}

double damping(double nu, double w0)
{
  const double r = w0 / nu;
  return 1.0 / (1.0 + r * r * r * r);   // alpha = 4
}

double zpeOf(const std::vector<double>& nu)
{
  double z = 0.0;
  for (double f : nu) z += 0.5 * 6.62607015e-34 * C_CM * f / EH;
  return z;
}

double vibThermalOf(const std::vector<double>& nu, double T)
{
  double e = 0.0;
  for (double f : nu) {
    const double x = HC_K * f / T;
    e += 6.62607015e-34 * C_CM * f / (std::exp(x) - 1.0) / EH;
  }
  return e;
}

double sTrans(double massAmu, double T, double pressure)
{
  const double m = massAmu * AMU;
  return R_ * (std::log(std::pow(2.0 * PI * m * K_B * T /
                                  (6.62607015e-34 * 6.62607015e-34), 1.5) *
                        K_B * T / pressure) + 2.5);
}

double sRot(const double* b, bool linear, int sigma, double T)
{
  if (linear)
    return R_ * (std::log(K_B * T / (6.62607015e-34 * C_CM * b[0]) / sigma) + 1.0);
  const double q = std::sqrt(PI) / sigma *
      std::pow(K_B * T / (6.62607015e-34 * C_CM), 1.5) /
      std::sqrt(b[0] * b[1] * b[2]);
  return R_ * (std::log(q) + 1.5);
}

bool startsWith(const std::string& s, const std::string& prefix)
{
  return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

std::string lower(const std::string& s)
{
  std::string r;
  for (char c : s) r += (char)std::tolower((unsigned char)c);
  return r;
}

// Truhlar's quasi-harmonic correction raises every mode below the floor
// before any vibrational term is computed (ZPE, thermal energy, entropy alike).
std::vector<double> raisedTo(const std::vector<double>& nu, double floor)
{
  std::vector<double> r = nu;
  for (double& f : r)
    if (f < floor) f = floor;
  return r;
}

}

ThermoResult computeThermo(const ThermoInput& in, double T,
                           ThermoStandardState ss, ThermoTreatment tr, double w0)
{
  ThermoResult r;
  if (T <= 0.0) { r.notes.push_back("the temperature must be positive."); return r; }
  if (in.mass <= 0.0) { r.notes.push_back("the molecular mass is unknown."); return r; }
  if (in.symmetryNumber < 1) { r.notes.push_back("the symmetry number must be >= 1."); return r; }
  if (in.multiplicity < 1) { r.notes.push_back("the multiplicity must be >= 1."); return r; }
  if (in.frequencies.empty()) { r.notes.push_back("there are no frequencies."); return r; }

  const double pressure = ss == THERMO_SOLN_1M ? 1000.0 * R_ * T : P_ATM;

  r.ok = true;
  r.zpe = zpeOf(in.frequencies);
  r.thermalVib = vibThermalOf(in.frequencies, T);
  r.thermalRot = (in.linear ? 1.0 : 1.5) * K_B * T / EH;
  r.thermalTrans = 1.5 * K_B * T / EH;
  r.H = in.electronicEnergy + r.zpe + r.thermalVib + r.thermalRot + r.thermalTrans
      + K_B * T / EH;

  r.sElec = R_ * std::log((double)in.multiplicity);
  r.sTrans = sTrans(in.mass, T, pressure);
  r.sRot = sRot(in.rotConstants, in.linear, in.symmetryNumber, T);
  r.sVib = 0.0;
  for (double f : in.frequencies)
    r.sVib += R_ * sHo(f, T);

  switch (tr) {
    case THERMO_RRHO:
      break;
    case THERMO_GRIMME_QRRHO:
      r.sVib = 0.0;
      for (double f : in.frequencies) {
        const double w = damping(f, w0);
        r.sVib += R_ * (w * sHo(f, T) + (1.0 - w) * sFr(f, T));
      }
      break;
    case THERMO_TRUHLAR_RAISE: {
      const std::vector<double> nu = raisedTo(in.frequencies, 100.0);
      r.zpe = zpeOf(nu);
      r.thermalVib = vibThermalOf(nu, T);
      r.H = in.electronicEnergy + r.zpe + r.thermalVib + r.thermalRot +
            r.thermalTrans + K_B * T / EH;
      r.sVib = 0.0;
      for (double f : nu)
        r.sVib += R_ * sHo(f, T);
      break;
    }
    case THERMO_HEADGORDON_QRRHO: {
      // Quasi-RRHO for the thermal enthalpy only: a low mode's thermal energy
      // is damped from the harmonic value toward the free rotor's kT/2; the
      // zero point energy and the entropy stay harmonic.
      double vib = 0.0;
      for (double f : in.frequencies) {
        const double w = damping(f, 100.0);
        const double x = HC_K * f / T;
        const double ho = 6.62607015e-34 * C_CM * f / (std::exp(x) - 1.0) / EH;
        vib += w * ho + (1.0 - w) * 0.5 * K_B * T / EH;
      }
      r.thermalVib = vib;
      r.H = in.electronicEnergy + r.zpe + r.thermalVib + r.thermalRot +
            r.thermalTrans + K_B * T / EH;
      break;
    }
  }

  // Per-mode terms, with the same damping as the totals above.
  for (double f : in.frequencies) {
    ThermoMode m;
    m.freq = f;
    const double hnu = 6.62607015e-34 * C_CM * f / EH;
    const double x = HC_K * f / T;
    const double ho = hnu / (std::exp(x) - 1.0);
    double zp = 0.5 * hnu, th = ho, s = R_ * sHo(f, T);
    if (tr == THERMO_GRIMME_QRRHO) {
      const double w = damping(f, w0);
      s = R_ * (w * sHo(f, T) + (1.0 - w) * sFr(f, T));
    } else if (tr == THERMO_TRUHLAR_RAISE && f < 100.0) {
      const double g = 100.0, y = HC_K * g / T;
      const double hg = 6.62607015e-34 * C_CM * g / EH;
      zp = 0.5 * hg; th = hg / (std::exp(y) - 1.0); s = R_ * sHo(g, T);
    } else if (tr == THERMO_HEADGORDON_QRRHO) {
      const double w = damping(f, 100.0);
      th = w * ho + (1.0 - w) * 0.5 * K_B * T / EH;
    }
    m.S = s;
    m.Gcorr = zp + th - T * s / (EH * N_A);
    r.modes.push_back(m);
  }

  r.S = r.sElec + r.sTrans + r.sRot + r.sVib;
  r.G = r.H - T * r.S / (EH * N_A);
  return r;
}

bool parseTreatmentName(const std::string& name, ThermoTreatment& t,
                        double& w0, std::string& err)
{
  const std::string s = lower(name);
  if (s == "rrho") { t = THERMO_RRHO; w0 = 100.0; return true; }
  if (s == "truhlar") { t = THERMO_TRUHLAR_RAISE; w0 = 100.0; return true; }
  if (s == "head-gordon" || s == "headgordon") {
    t = THERMO_HEADGORDON_QRRHO; w0 = 100.0; return true;
  }
  if (startsWith(s, "qrrho-grimme:") || startsWith(s, "qrrho:")) {
    const size_t colon = s.find(':');
    const double v = std::atof(s.substr(colon + 1).c_str());
    if (v < 1.0) { err = "bad w0 in '" + name + "'"; return false; }
    t = THERMO_GRIMME_QRRHO; w0 = v; return true;
  }
  err = "unknown treatment '" + name + "'";
  return false;
}

int symmetryNumberForPointGroup(const std::string& pointGroup)
{
  std::string g;
  for (char ch : pointGroup) {
    if (ch == '*') g += "INF";   // autosym's C*v / D*h
    else if (ch != ' ' && ch != '\t') g += (char)toupper((unsigned char)ch);
  }
  if (g == "C1" || g == "CS" || g == "CI" || g == "CINFV") return 1;
  if (g == "C2" || g == "C2V" || g == "C2H") return 2;
  if (g == "C3" || g == "C3V" || g == "C3H" || g == "S6") return 3;
  if (g == "C4" || g == "C4V" || g == "C4H" || g == "S4" || g == "D2" ||
      g == "D2H" || g == "D2D") return 4;
  if (g == "C5" || g == "C5V" || g == "C5H") return 5;
  if (g == "C6" || g == "C6V" || g == "C6H" || g == "D3" || g == "D3H" ||
      g == "D3D") return 6;
  if (g == "C7" || g == "C7V" || g == "C7H") return 7;
  if (g == "C8" || g == "C8V" || g == "C8H" || g == "S8" || g == "D4" ||
      g == "D4D") return 8;
  if (g == "D5" || g == "D5H" || g == "D5D") return 10;
  if (g == "D6" || g == "D6H" || g == "D6D") return 12;
  if (g == "D7" || g == "D7H" || g == "D7D") return 14;
  if (g == "D8" || g == "D8H" || g == "D8D") return 16;
  if (g == "T" || g == "TD" || g == "TH") return 12;
  if (g == "O" || g == "OH") return 24;
  if (g == "I" || g == "IH") return 60;
  if (g == "DINFH") return 2;
  return 1;
}

std::vector<double> realVibrations(const std::vector<double>& storedFreqs,
                                   std::vector<std::string>& notes)
{
  std::vector<double> out;
  int dropped = 0, imag = 0;
  for (double f : storedFreqs) {
    if (VibFrequency::isZero(f)) { dropped++; continue; }
    if (VibFrequency::isImaginary(f)) { imag++; continue; }
    if (VibFrequency::isReal(f)) out.push_back(f);
  }
  if (dropped)
    notes.push_back(num(dropped, 0) + " translation/rotation mode(s) below 1 cm-1 "
                    "were not counted as vibrations.");
  if (imag)
    notes.push_back(num(imag, 0) + " imaginary mode(s) were skipped.");
  return out;
}

const char* treatmentName(ThermoTreatment t)
{
  switch (t) {
    case THERMO_RRHO: return "RRHO";
    case THERMO_GRIMME_QRRHO: return "Grimme quasi-RRHO entropy";
    case THERMO_TRUHLAR_RAISE: return "Truhlar raise-to-100 cm-1";
    case THERMO_HEADGORDON_QRRHO: return "Head-Gordon quasi-RRHO enthalpy";
  }
  return "unknown";
}

const char* standardStateName(ThermoStandardState s)
{
  return s == THERMO_SOLN_1M ? "1 M" : "1 atm";
}

bool pointGroupIsLinear(const std::string& pointGroup)
{
  std::string g;
  for (char ch : pointGroup) {
    if (ch == '*') g += "INF";
    else if (ch != ' ' && ch != '\t') g += (char)toupper((unsigned char)ch);
  }
  return g == "CINFV" || g == "DINFH";
}

std::string symmetrySummary(const ThermoInput& in)
{
  std::string s = in.pointGroup.empty() ? std::string("Point group not stored")
                                        : "Point group " + in.pointGroup;
  s += ", \xCF\x83 = " + std::to_string(in.symmetryNumber);
  if (in.sigmaSource == "code")
    s += " (printed by " + (in.code.empty() ? std::string("the code") : in.code) + ")";
  else if (in.sigmaSource == "override")
    s += " (set by you)";
  else
    s += " (from the point group; the symmetry number is not read from " +
         (in.code.empty() ? std::string("the code") : in.code) + "'s output)";
  s += in.linear ? ", linear rotor." : ", non-linear rotor.";
  return s;
}

std::string symmetryWarning(const ThermoInput& in)
{
  if (in.nearGroup.empty()) return std::string();
  const int sig = symmetryNumberForPointGroup(in.nearGroup);
  const bool lin = pointGroupIsLinear(in.nearGroup);
  if (sig <= in.symmetryNumber && !(lin && !in.linear)) return std::string();
  char tol[32];
  snprintf(tol, sizeof tol, "%g", in.nearTolerance);
  return "The structure is within " + std::string(tol) + " A of point group " +
         in.nearGroup + ", which has rotational symmetry number " +
         std::to_string(sig) + (lin ? " and a linear rotor" : "") +
         "; the thermochemistry used \xCF\x83 = " + std::to_string(in.symmetryNumber) +
         (in.linear ? " and a linear rotor." : " and a non-linear rotor.") +
         " Set the symmetry number and rotor type if the higher symmetry is meant.";
}

bool applyRotorChoice(ThermoInput& in, int choice, std::string& err)
{
  if (choice == 1 && !in.linear) {
    const double b = 0.5 * (in.rotConstants[1] + in.rotConstants[2]);
    in.linear = true;
    in.rotConstants[0] = in.rotConstants[1] = in.rotConstants[2] = b;
  } else if (choice == 2 && in.linear) {
    err = "a linear molecule has no third rotational constant, so a non-linear rotor cannot be forced.";
    return false;
  }
  return true;
}
