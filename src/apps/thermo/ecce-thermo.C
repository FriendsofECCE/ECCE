// ecce-thermo: recompute H, S and G from a calculation's stored frequencies
// with the chosen low-frequency treatment, and the spread over treatments.
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "ThermoGather.H"
#include "tdat/thermo/ThermoChem.H"

// JSON string literal: names, URLs and notes may hold quotes or backslashes.
static std::string jstr(const std::string& s)
{
  std::string o = "\"";
  for (unsigned char ch : s) {
    if (ch == '"' || ch == '\\') { o += '\\'; o += (char)ch; }
    else if (ch == '\n') o += "\\n";
    else if (ch == '\t') o += "\\t";
    else if (ch < 0x20) { char b[8]; std::snprintf(b, sizeof b, "\\u%04x", ch); o += b; }
    else o += (char)ch;
  }
  return o + "\"";
}

using std::string;
using std::vector;

namespace {

const double KJ_PER_HARTREE = 4.3597447222071e-18 * 6.02214076e23 / 1000.0;
const double KCAL_PER_KJ = 1.0 / 4.184;

struct TreatmentSpec {
  ThermoTreatment t;
  double w0;
  string cli;   // how the user asked for it
};

struct Calc {
  string url;
  string name;
  ThermoInput in;
  vector<string> notes;
  string error;
};

string lower(const string& s)
{
  string r;
  for (char c : s) r += (char)tolower((unsigned char)c);
  return r;
}

bool parseTreatments(const string& spec, vector<TreatmentSpec>& out, string& err)
{
  out.clear();
  size_t pos = 0;
  while (pos <= spec.size()) {
    size_t end = spec.find(',', pos);
    if (end == string::npos) end = spec.size();
    string item = spec.substr(pos, end - pos);
    pos = end + 1;
    if (item.empty()) continue;
    const string s = lower(item);
    if (s == "all") {
      out.push_back({THERMO_RRHO, 100.0, "rrho"});
      out.push_back({THERMO_GRIMME_QRRHO, 50.0, "qrrho-grimme:50"});
      out.push_back({THERMO_GRIMME_QRRHO, 100.0, "qrrho-grimme:100"});
      out.push_back({THERMO_GRIMME_QRRHO, 200.0, "qrrho-grimme:200"});
      out.push_back({THERMO_TRUHLAR_RAISE, 100.0, "truhlar"});
      out.push_back({THERMO_HEADGORDON_QRRHO, 100.0, "head-gordon"});
    } else {
      ThermoTreatment t;
      double w0 = 100.0;
      if (!parseTreatmentName(item, t, w0, err)) return false;
      out.push_back({t, w0, item});
    }
  }
  if (out.empty()) err = "no treatments";
  return !out.empty();
}

// "[-][N*]name" entries, as in Compare Energies.
struct Term { string name; int coeff; };

bool parseSide(const string& side, vector<Term>& out)
{
  size_t pos = 0;
  while (pos <= side.size()) {
    size_t end = side.find(',', pos);
    if (end == string::npos) end = side.size();
    string item = side.substr(pos, end - pos);
    pos = end + 1;
    if (item.empty()) continue;
    int sign = 1, n = 1;
    if (item[0] == '-') { sign = -1; item.erase(0, 1); }
    const size_t star = item.find('*');
    if (star != string::npos && star > 0 &&
        item.find_first_not_of("0123456789") == star) {
      n = atoi(item.substr(0, star).c_str());
      item.erase(0, star + 1);
    }
    if (item.empty()) return false;
    out.push_back({item, sign * n});
  }
  return !out.empty();
}

string fmtG(double hartree)
{
  char b[64];
  snprintf(b, sizeof b, "%.6f (%.2f kJ/mol, %.2f kcal/mol)", hartree,
           hartree * KJ_PER_HARTREE, hartree * KJ_PER_HARTREE * KCAL_PER_KJ);
  return b;
}

// Compute one treatment; on failure print the reason and return false so the
// caller never shows H=S=G=0 as a real number.
bool computeChecked(const ThermoInput& in, double T, ThermoStandardState ss,
                    const TreatmentSpec& tr, ThermoResult& out)
{
  out = computeThermo(in, T, ss, tr.t, tr.w0);
  if (!out.ok)
    for (const string& n : out.notes) std::printf("error: %s\n", n.c_str());
  return out.ok;
}

int run(const vector<string>& urls, double T, ThermoStandardState ss,
        const vector<TreatmentSpec>& trs, bool json, int sigmaOverride,
        const string& reaction)
{
  vector<Calc> calcs;
  for (const string& u : urls) {
    Calc c;
    c.url = u;
    gatherThermoInput(u, c.in, c.name, c.notes, c.error);
    if (sigmaOverride > 0) c.in.symmetryNumber = sigmaOverride;
    calcs.push_back(c);
  }
  for (const Calc& c : calcs)
    if (!c.error.empty()) {
      std::printf("error: %s: %s\n", c.url.c_str(), c.error.c_str());
      return 1;
    }

  if (!reaction.empty()) {
    // --reaction "reactants -> products": names matched against the calcs.
    const size_t arrow = reaction.find("->");
    if (arrow == string::npos) {
      std::printf("error: --reaction needs 'LHS -> RHS'\n");
      return 1;
    }
    vector<Term> lhs, rhs;
    if (!parseSide(reaction.substr(0, arrow), lhs) ||
        !parseSide(reaction.substr(arrow + 2), rhs)) {
      std::printf("error: cannot parse --reaction\n");
      return 1;
    }
    std::map<string, const Calc*> byName;
    for (const Calc& c : calcs) byName[c.name] = &c;
    for (const Calc& c : calcs)
      for (const string& n : c.notes)
        std::printf("note: %s (%s)\n", n.c_str(), c.name.c_str());
    // delta G = sum(products coeff*G) - sum(reactants coeff*G)
    std::vector<std::pair<string, double> > dg;   // (treatment, hartree)
    for (const TreatmentSpec& tr : trs) {
      double d = 0.0;
      bool missing = false;
      ThermoResult r;
      for (const Term& t : lhs) {
        const Calc* c = byName[t.name];
        if (!c) { missing = true; std::printf("error: no calc named '%s'\n", t.name.c_str()); break; }
        if (!computeChecked(c->in, T, ss, tr, r)) return 1;
        d -= t.coeff * r.G;
      }
      for (const Term& t : rhs) {
        const Calc* c = byName[t.name];
        if (!c) { missing = true; std::printf("error: no calc named '%s'\n", t.name.c_str()); break; }
        if (!computeChecked(c->in, T, ss, tr, r)) return 1;
        d += t.coeff * r.G;
      }
      if (missing) return 1;
      dg.push_back(std::make_pair(tr.cli, d));
    }
    double lo = dg[0].second, hi = dg[0].second;
    for (const auto& e : dg) { lo = std::min(lo, e.second); hi = std::max(hi, e.second); }
    if (json) {
      std::printf("{\n  \"reaction\": %s,\n  \"deltaG\": {\n",
                  jstr(reaction).c_str());
      for (size_t i = 0; i < dg.size(); i++)
        std::printf("    %s: %.6f%s\n", jstr(dg[i].first).c_str(), dg[i].second,
                    i + 1 < dg.size() ? "," : "");
      std::printf("  },\n  \"spread_kJ_per_mol\": %.3f\n}\n", (hi - lo) * KJ_PER_HARTREE);
    } else {
      std::printf("reaction delta G over treatments (T = %.2f K, %s):\n", T,
                  standardStateName(ss));
      for (const auto& e : dg)
        std::printf("  %-18s %s\n", e.first.c_str(), fmtG(e.second).c_str());
      std::printf("  spread: %.3f kJ/mol\n", (hi - lo) * KJ_PER_HARTREE);
    }
    return 0;
  }

  if (json) {
    std::printf("[\n");
    for (size_t i = 0; i < calcs.size(); i++) {
      const Calc& c = calcs[i];
      std::printf("  {\n    \"name\": %s,\n    \"url\": %s,\n",
                  jstr(c.name).c_str(), jstr(c.url).c_str());
      std::printf("    \"T\": %.2f,\n    \"standardState\": \"%s\",\n",
                  T, standardStateName(ss));
      std::printf("    \"symmetryNumber\": %d,\n    \"multiplicity\": %d,\n",
                  c.in.symmetryNumber, c.in.multiplicity);
      std::printf("    \"treatments\": {\n");
      for (size_t j = 0; j < trs.size(); j++) {
        ThermoResult r;
        if (!computeChecked(c.in, T, ss, trs[j], r)) return 1;
        std::printf("      %s: {\"H\": %.6f, \"S\": %.6f, \"G\": %.6f}%s\n",
                    jstr(trs[j].cli).c_str(), r.H, r.S, r.G,
                    j + 1 < trs.size() ? "," : "");
      }
      std::printf("    },\n    \"notes\": [");
      for (size_t j = 0; j < c.notes.size(); j++)
        std::printf("%s%s", j ? ", " : " ", jstr(c.notes[j]).c_str());
      std::printf(" ]\n  }%s\n", i + 1 < calcs.size() ? "," : "");
    }
    std::printf("]\n");
    return 0;
  }

  for (size_t i = 0; i < calcs.size(); i++) {
    const Calc& c = calcs[i];
    std::printf("%s (%s)\n", c.name.c_str(), c.url.c_str());
    std::printf("  T = %.2f K, standard state %s, sigma = %d, multiplicity = %d\n",
                T, standardStateName(ss), c.in.symmetryNumber, c.in.multiplicity);
    if (c.notes.empty())
      std::printf("  %d vibration(s)\n", (int)c.in.frequencies.size());
    for (const string& n : c.notes) std::printf("  note: %s\n", n.c_str());
    std::printf("  %-18s %12s %12s %s\n", "treatment", "H (Hartree)", "S (J/mol/K)", "G");
    for (const TreatmentSpec& tr : trs) {
      ThermoResult r;
      if (!computeChecked(c.in, T, ss, tr, r)) return 1;
      std::printf("  %-18s %12.6f %12.3f %s\n", tr.cli.c_str(), r.H, r.S,
                  fmtG(r.G).c_str());
    }
    if (i + 1 < calcs.size()) std::printf("\n");
  }
  return 0;
}

}

int main(int argc, char** argv)
{
  vector<string> urls;
  double T = 298.15;
  ThermoStandardState ss = THERMO_GAS_1ATM;
  string trSpec = "all";
  bool json = false;
  int sigmaOverride = 0;
  string reaction;
  for (int i = 1; i < argc; i++) {
    const string a = argv[i];
    if (a == "--json") json = true;
    else if (a == "--T" && i + 1 < argc) T = atof(argv[++i]);
    else if (a == "--standard-state" && i + 1 < argc) {
      const string v = lower(argv[++i]);
      if (v == "1m" || v == "1m" || v == "solution") ss = THERMO_SOLN_1M;
      else if (v == "1atm" || v == "gas" || v == "1atm") ss = THERMO_GAS_1ATM;
      else { std::printf("error: --standard-state is 1atm or 1M\n"); return 1; }
    }
    else if (a == "--treatment" && i + 1 < argc) trSpec = argv[++i];
    else if (a == "--symmetry-number" && i + 1 < argc) sigmaOverride = atoi(argv[++i]);
    else if (a == "--reaction" && i + 1 < argc) reaction = argv[++i];
    else if (a == "--help" || a == "-h") {
      std::printf("ecce-thermo <calc> [<calc> ...]\n"
                  "  [--T 298.15] [--standard-state 1atm|1M]\n"
                  "  [--treatment all|rrho|qrrho-grimme:100|truhlar|head-gordon,...]\n"
                  "  [--symmetry-number N] [--reaction \"LHS -> RHS\"] [--json]\n");
      return 0;
    }
    else if (!a.empty() && a[0] == '-') { std::printf("error: unknown option %s\n", a.c_str()); return 1; }
    else urls.push_back(a);
  }
  if (urls.empty()) { std::printf("error: at least one calculation is required\n"); return 1; }
  vector<TreatmentSpec> trs;
  string err;
  if (!parseTreatments(trSpec, trs, err)) { std::printf("error: %s\n", err.c_str()); return 1; }
  return run(urls, T, ss, trs, json, sigmaOverride, reaction);
}
