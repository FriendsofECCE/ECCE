#include "util/RateRun.H"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>

#include "util/Ecce.H"

using std::string;
using std::vector;

namespace {

// Just enough JSON for ecce-rate's output: objects, arrays, numbers, strings, null.
struct J
{
  enum Kind { Null, Num, Str, Arr, Obj } kind = Null;
  double num = NAN;
  string str;
  vector<J> arr;
  std::map<string, J> obj;
  const J& at(const string& k) const
  {
    static const J none;
    auto it = obj.find(k);
    return it == obj.end() ? none : it->second;
  }
  double n() const { return kind == Num ? num : NAN; }
};

struct Parser
{
  const string& s;
  size_t p = 0;
  explicit Parser(const string& t) : s(t) {}
  void ws() { while (p < s.size() && isspace((unsigned char)s[p])) p++; }
  string str()
  {
    string o;
    for (p++; p < s.size() && s[p] != '"'; p++) {
      if (s[p] == '\\' && p + 1 < s.size()) {
        p++;
        if (s[p] == 'u' && p + 4 < s.size()) {
          const unsigned c = (unsigned)strtoul(s.substr(p + 1, 4).c_str(), 0, 16);
          p += 4;
          if (c < 0x80) o += (char)c;
          else if (c < 0x800) { o += (char)(0xC0 | c >> 6); o += (char)(0x80 | (c & 63)); }
          else { o += (char)(0xE0 | c >> 12); o += (char)(0x80 | ((c >> 6) & 63)); o += (char)(0x80 | (c & 63)); }
          continue;
        }
        o += s[p] == 'n' ? '\n' : s[p];
      } else o += s[p];
    }
    p++;
    return o;
  }
  bool value(J& v)
  {
    ws();
    if (p >= s.size()) return false;
    if (s[p] == '{') {
      v.kind = J::Obj;
      p++;
      for (;;) {
        ws();
        if (p < s.size() && s[p] == '}') { p++; return true; }
        if (p >= s.size() || s[p] != '"') return false;
        const string k = str();
        ws();
        if (p >= s.size() || s[p++] != ':') return false;
        if (!value(v.obj[k])) return false;
        ws();
        if (p < s.size() && s[p] == ',') p++;
      }
    }
    if (s[p] == '[') {
      v.kind = J::Arr;
      p++;
      for (;;) {
        ws();
        if (p < s.size() && s[p] == ']') { p++; return true; }
        v.arr.emplace_back();
        if (!value(v.arr.back())) return false;
        ws();
        if (p < s.size() && s[p] == ',') p++;
      }
    }
    if (s[p] == '"') { v.kind = J::Str; v.str = str(); return true; }
    if (s.compare(p, 4, "null") == 0) { p += 4; return true; }
    char *end = 0;
    v.num = strtod(s.c_str() + p, &end);
    if (end == s.c_str() + p) return false;
    v.kind = J::Num;
    p = end - s.c_str();
    return true;
  }
};

string quote(const string& v)
{
  string o;
  for (char c : v) o += c == '\'' ? string("'\\''") : string(1, c);
  return "'" + o + "'";
}

string joined(const vector<string>& v)
{
  string o;
  for (size_t i = 0; i < v.size(); i++) o += (i ? "," : "") + v[i];
  return o;
}

}  // namespace

string rateLocalPath(const string& url)
{
  if (url.compare(0, 7, "file://") == 0) return url.substr(7);
  if (!url.empty() && url[0] == '/') return url;
  return string();
}

RateResult runRate(const RateRequest& q)
{
  RateResult r;
#ifdef _WIN32
  r.error = "Rate constants are not available on this platform yet.";
  return r;
#else
  const char *home = getenv("ECCE_HOME");
  const string exe = (home && *home ? string(home) + "/bin/" : string()) + "ecce-rate";
  string cmd = quote(exe) + " " + quote(joined(q.reactants)) + " " + quote(q.ts);
  if (!q.products.empty()) cmd += " " + quote(joined(q.products));
  cmd += " --T " + quote(q.temperatures) + " --standard-state " + quote(q.standardState) +
         " --treatment " + quote(q.treatment) + " --json 2>&1";
  string text;
  const int status = Ecce::readCommand(cmd, text);
  const size_t b = text.find("{\n");
  J j;
  if (status == 0 && b != string::npos) {
    const string body = text.substr(b);
    Parser pr(body);
    j = J();
    if (!pr.value(j)) j = J();
  }
  if (status != 0 || j.kind != J::Obj) {
    const size_t e = text.find("error: ");
    r.error = e == string::npos ? "ecce-rate could not be run."
                                : text.substr(e + 7, text.find('\n', e) - e - 7);
    return r;
  }
  for (const J& n : j.at("notes").arr) r.notes.push_back(n.str);
  r.unit = j.at("unit").str;
  r.unitReverse = j.at("unit_reverse").str;
  r.treatment = j.at("treatment").str;
  r.standardState = j.at("standard_state").str;
  r.hasProduct = !j.at("products").arr.empty();
  r.nu = j.at("imaginary_frequency_cm-1").n();
  r.barrier = j.at("zpe_barrier_forward_kJ_per_mol").n();
  for (const J& t : j.at("temperatures").arr) {
    RateRow w;
    w.T = t.at("T").n(); w.dG = t.at("dG_act").n(); w.dH = t.at("dH_act").n();
    w.dS = t.at("dS_act").n();
    w.kappaWigner = t.at("kappa").at("wigner").n();
    w.kappaEckart = t.at("kappa").at("eckart").n();
    w.k = t.at("k").at("none").n();
    w.kWigner = t.at("k").at("wigner").n();
    w.kEckart = t.at("k").at("eckart").n();
    w.dGrxn = t.at("dG_rxn").n(); w.dGreverse = t.at("dG_act_reverse").n();
    w.Keq = t.at("K_eq").n();
    w.kRev = t.at("k_reverse").at("none").n();
    w.kRevWigner = t.at("k_reverse").at("wigner").n();
    w.kRevEckart = t.at("k_reverse").at("eckart").n();
    r.rows.push_back(w);
  }
  static const char *const names[3] = {"none", "wigner", "eckart"};
  static const char *const labels[3] = {"TST", "Wigner", "Eckart"};
  for (int i = 0; i < 3; i++) {
    const J& f = j.at("fits").at(names[i]);
    if (f.kind != J::Obj) continue;
    RateFit x;
    x.method = labels[i];
    x.A = f.at("A").n(); x.Ea = f.at("Ea_kJ_per_mol").n();
    x.dH = f.at("dH_act_kJ_per_mol").n(); x.dS = f.at("dS_act_J_per_mol_K").n();
    r.fits.push_back(x);
  }
  r.ok = !r.rows.empty();
  if (!r.ok) r.error = "ecce-rate gave no values.";
  return r;
#endif
}

MorseResult runMorse(const string& calcFolder, double windowKj)
{
  MorseResult r;
#ifdef _WIN32
  r.error = "The Morse fit is not available on this platform yet.";
  return r;
#else
  const char *home = getenv("ECCE_HOME");
  const string exe = (home && *home ? string(home) + "/bin/" : string()) + "ecce-morse";
  string cmd = quote(exe) + " " + quote(calcFolder) + " --json";
  if (windowKj > 0) {
    char buf[64];
    snprintf(buf, sizeof buf, " --window %.6g", windowKj);
    cmd += buf;
  }
  cmd += " 2>/dev/null";
  string text;
  const int status = Ecce::readCommand(cmd, text);
  const size_t b = text.find('{');
  J j;
  if (b != string::npos) {
    Parser pr(text.substr(b));
    if (!pr.value(j)) j = J();
  }
  if (j.kind != J::Obj) {
    r.error = "ecce-morse could not be run.";
    return r;
  }
  if (status != 0 || j.at("error").kind == J::Str) {
    r.error = j.at("error").str.empty() ? "ecce-morse failed." : j.at("error").str;
    return r;
  }
  r.report = j.at("report").str;
  r.bond = j.at("bond").str;
  r.windowMode = j.at("window_mode").str;
  r.De_eV = j.at("De_eV").n(); r.De_kJ = j.at("De_kJ_per_mol").n();
  r.a = j.at("a_per_A").n(); r.re = j.at("re_A").n();
  r.omega = j.at("omega_e_cm-1").n(); r.omegaX = j.at("omega_e_xe_cm-1").n();
  r.D0_eV = j.at("D0_eV").n(); r.D0_kJ = j.at("D0_kJ_per_mol").n();
  r.rms = j.at("rms_kJ_per_mol").n(); r.window = j.at("window_kJ_per_mol").n();
  r.used = (int)j.at("points_used").n(); r.total = (int)j.at("points_total").n();
  for (const J& v : j.at("curve_r_A").arr) r.curveR.push_back(v.n());
  for (const J& v : j.at("curve_E_hartree").arr) r.curveE.push_back(v.n());
  r.ok = !r.curveR.empty() && r.curveR.size() == r.curveE.size();
  if (!r.ok) r.error = "ecce-morse gave no curve.";
  return r;
#endif
}
