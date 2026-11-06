#include "util/ThermoRun.H"

#include <cstdio>
#include <cstdlib>

#include "util/Ecce.H"

using std::string;
using std::vector;

const ThermoValue *ThermoRunResult::find(const string& cli) const
{
  for (size_t i = 0; i < treatments.size(); i++)
    if (treatments[i] == cli) return &values[i];
  return 0;
}

const vector<ThermoTreatmentInfo>& thermoTreatments()
{
  static const vector<ThermoTreatmentInfo> t = {
    {"rrho", "RRHO"},
    {"qrrho-grimme:50", "Grimme qRRHO, \xCF\x89" "0 = 50 cm-1"},
    {"qrrho-grimme:100", "Grimme qRRHO, \xCF\x89" "0 = 100 cm-1"},
    {"qrrho-grimme:200", "Grimme qRRHO, \xCF\x89" "0 = 200 cm-1"},
    {"truhlar", "Truhlar (modes raised to 100 cm-1)"},
    {"head-gordon", "Head-Gordon qRRHO (enthalpy and entropy)"}};
  return t;
}

double thermoHartreeToKJ(double hartree)
{
  return hartree * 4.3597447222071e-18 * 6.02214076e23 / 1000.0;
}

namespace {
// The notes array holds JSON strings; only \" and \\ are unescaped.
vector<string> notesOf(const string& text)
{
  vector<string> out;
  size_t p = text.find("\"notes\"");
  if (p == string::npos) return out;
  p = text.find('[', p);
  const size_t end = text.find(']', p == string::npos ? 0 : p);
  while (p != string::npos && end != string::npos) {
    p = text.find('"', p + 1);
    if (p == string::npos || p > end) break;
    string s;
    for (++p; p < text.size() && text[p] != '"'; p++) {
      if (text[p] == '\\' && p + 1 < text.size()) p++;
      s += text[p];
    }
    out.push_back(s);
  }
  return out;
}

string stringField(const string& text, const char *key)
{
  size_t p = text.find(string("\"") + key + "\": \"");
  if (p == string::npos) return string();
  p += string(key).size() + 5;
  const size_t e = text.find('"', p);
  return e == string::npos ? string() : text.substr(p, e - p);
}
}

ThermoRunResult runThermo(const string& url, double kelvin, bool oneMolar)
{
  ThermoRunResult r;
  const char *home = getenv("ECCE_HOME");
  const string exe = (home && *home ? string(home) + "/bin/" : string()) + "ecce-thermo";
  string quoted;
  for (char c : url) quoted += c == '\'' ? string("'\\''") : string(1, c);
  char opt[64];
  snprintf(opt, sizeof opt, " --T %.4f --standard-state %s --json", kelvin,
           oneMolar ? "1M" : "1atm");
#ifdef _WIN32
  const string cmd = "\"" + exe + ".exe\" \"" + url + "\"" + opt;
#else
  const string cmd = "'" + exe + "' '" + quoted + "'" + opt + " 2>/dev/null";
#endif
  string text;
  const int status = Ecce::readCommand(cmd, text);
  if (status != 0 || text.find("\"treatments\"") == string::npos) {
    // ecce-thermo says why in a line "error: ..." (after any partial JSON).
    const size_t e = text.find("error: ");
    if (e == string::npos) r.error = "ecce-thermo could not be run.";
    else {
      r.error = text.substr(e + 7, text.find('\n', e) - e - 7);
    }
    return r;
  }
  r.name = stringField(text, "name");
  const size_t tp = text.find("\"T\": ");
  if (tp != string::npos) r.T = atof(text.c_str() + tp + 5);
  const size_t from = text.find("\"treatments\"");
  for (const ThermoTreatmentInfo& t : thermoTreatments()) {
    const size_t p = text.find(string("\"") + t.cli + "\": {", from);
    ThermoValue v;
    if (p == string::npos ||
        sscanf(text.c_str() + p + string(t.cli).size() + 5,
               "\"H\": %lf, \"S\": %lf, \"G\": %lf", &v.H, &v.S, &v.G) != 3)
      continue;
    r.treatments.push_back(t.cli);
    r.values.push_back(v);
  }
  r.notes = notesOf(text.substr(from));
  r.ok = !r.treatments.empty();
  if (!r.ok) r.error = "ecce-thermo gave no values.";
  return r;
}

bool runThermoReactions(const string& file, const string& treatment, string& csv,
                        string& error, double kelvin)
{
  const char *home = getenv("ECCE_HOME");
  const string exe = (home && *home ? string(home) + "/bin/" : string()) + "ecce-thermo";
  char tbuf[32];
  snprintf(tbuf, sizeof tbuf, " --T %.4f", kelvin);
  auto q = [](const string& v) {
    string o;
    for (char c : v) o += c == '\'' ? string("'\\''") : string(1, c);
    return "'" + o + "'";
  };
#ifdef _WIN32
  const string cmd = "\"" + exe + ".exe\" --reactions \"" + file + "\" --chosen " + treatment +
                     " --format csv" + tbuf;
#else
  const string cmd = q(exe) + " --reactions " + q(file) + " --chosen " + q(treatment) +
                     " --format csv" + tbuf + " 2>/dev/null";
#endif
  const int status = Ecce::readCommand(cmd, csv);
  if (status != 0 || csv.compare(0, 8, "reaction") != 0) {
    const size_t e = csv.find("error: ");
    error = e == string::npos ? "ecce-thermo could not be run."
                              : csv.substr(e + 7, csv.find('\n', e) - e - 7);
    return false;
  }
  return true;
}
