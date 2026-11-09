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
int g_rotor = 0;            // --rotor: 0 as found, 1 linear, 2 non-linear
double g_symTol = 0.05;     // --symmetry-tolerance (A); 0 turns the check off

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

// --symmetry-number and --rotor; a refused rotor choice stays visible as a note.
void applyOverrides(Calc& c, int sigmaOverride)
{
  if (sigmaOverride > 0) {
    c.in.symmetryNumber = sigmaOverride;
    c.in.sigmaSource = "override";
  }
  string err;
  if (!applyRotorChoice(c.in, g_rotor, err)) c.notes.push_back(err);
}

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
    // "a -> b" and "a, b" are written with spaces
    const size_t first = item.find_first_not_of(" \t");
    item = first == string::npos ? string()
         : item.substr(first, item.find_last_not_of(" \t") - first + 1);
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
    gatherThermoInput(u, c.in, c.name, c.notes, c.error, g_symTol);
    applyOverrides(c, sigmaOverride);
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
      std::printf("    \"symmetry\": %s,\n", jstr(symmetrySummary(c.in)).c_str());
      std::printf("    \"treatments\": {\n");
      for (size_t j = 0; j < trs.size(); j++) {
        ThermoResult r;
        if (!computeChecked(c.in, T, ss, trs[j], r)) return 1;
        std::printf("      %s: {\"H\": %.6f, \"S\": %.6f, \"G\": %.6f}%s\n",
                    jstr(trs[j].cli).c_str(), r.H, r.S, r.G,
                    j + 1 < trs.size() ? "," : "");
      }
      vector<string> notes = c.notes;
      const string warn = symmetryWarning(c.in);
      if (!warn.empty()) notes.push_back(warn);
      std::printf("    },\n    \"notes\": [");
      for (size_t j = 0; j < notes.size(); j++)
        std::printf("%s%s", j ? ", " : " ", jstr(notes[j]).c_str());
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
    std::printf("  %s\n", symmetrySummary(c.in).c_str());
    if (c.notes.empty())
      std::printf("  %d vibration(s)\n", (int)c.in.frequencies.size());
    for (const string& n : c.notes) std::printf("  note: %s\n", n.c_str());
    const string warn = symmetryWarning(c.in);
    if (!warn.empty()) std::printf("  warning: %s\n", warn.c_str());
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

// ---- --reactions FILE: reaction overview (#709) ----
struct Species {
  string token, url, name, key;
  int coeff = 0;          // signed: products positive
  Calc calc;
  bool ok = false;
};

const double SOFT_CM = 100.0;

string csvq(const string& s)
{
  if (s.find_first_of(",\"\n") == string::npos) return s;
  string o = "\"";
  for (char c : s) { if (c == '"') o += '"'; o += c; }
  return o + "\"";
}

struct ReactionOut {
  string text, status = "ok", reason;
  double dE = 0, dG = 0, lo = 0, hi = 0;
  vector<std::pair<string, double> > dgs;
  vector<Species> sp;
  vector<ThermoResult> chosen;   // per species, chosen treatment
};

int runReactions(const string& file, const vector<string>& urls, double T,
                 ThermoStandardState ss, const string& chosenSpec,
                 const string& format, int sigmaOverride)
{
  std::FILE* f = std::fopen(file.c_str(), "r");
  if (!f) { std::printf("error: cannot read %s\n", file.c_str()); return 1; }
  vector<string> lines;
  char buf[4096];
  while (std::fgets(buf, sizeof buf, f)) {
    string l = buf;
    while (!l.empty() && (l.back() == '\n' || l.back() == '\r' || l.back() == ' ')) l.pop_back();
    const size_t b = l.find_first_not_of(" \t");
    if (b == string::npos || l[b] == '#') continue;
    lines.push_back(l.substr(b));
  }
  std::fclose(f);

  vector<TreatmentSpec> six, chosenV;
  string err;
  parseTreatments("all", six, err);
  if (!parseTreatments(chosenSpec, chosenV, err)) { std::printf("error: %s\n", err.c_str()); return 1; }
  const TreatmentSpec chosen = chosenV[0];

  std::map<string, string> named;   // positional calcs by name
  std::map<string, Calc> cache;
  for (const string& u : urls) {
    Calc c; c.url = u;
    gatherThermoInput(u, c.in, c.name, c.notes, c.error, 0.0);
    named[c.name] = u;
    cache[u] = c;
  }
  auto resolve = [&](const string& tok) {
    if (named.count(tok)) return named[tok];
    if (!tok.empty() && tok[0] == '/') return "file://" + tok;
    return tok;
  };

  vector<ReactionOut> outs;
  for (const string& line : lines) {
    ReactionOut ro;
    ro.text = line;
    const size_t arrow = line.find("->");
    vector<Term> lhs, rhs;
    if (arrow == string::npos || !parseSide(line.substr(0, arrow), lhs) ||
        !parseSide(line.substr(arrow + 2), rhs)) {
      ro.status = "refused"; ro.reason = "cannot parse; the form is 'a, 2*b -> c'";
      outs.push_back(ro);
      continue;
    }
    for (int side = 0; side < 2; side++)
      for (const Term& t : side ? rhs : lhs) {
        Species sp;
        sp.token = t.name;
        sp.url = resolve(t.name);
        sp.coeff = side ? t.coeff : -t.coeff;
        if (!cache.count(sp.url)) {
          Calc c; c.url = sp.url;
          gatherThermoInput(sp.url, c.in, c.name, c.notes, c.error, 0.0);
          applyOverrides(c, sigmaOverride);
          cache[sp.url] = c;
        }
        sp.calc = cache[sp.url];
        sp.name = sp.calc.name.empty() ? t.name : sp.calc.name;
        ro.sp.push_back(sp);
      }
    for (Species& sp : ro.sp) {
      if (!sp.calc.error.empty()) {
        ro.status = "refused";
        if (ro.reason.empty()) ro.reason = sp.name + ": " + sp.calc.error;
        continue;
      }
      sp.ok = true;
      sp.key = gatherMethodKey(sp.url);
    }
    if (ro.status == "ok")
      for (const Species& sp : ro.sp)
        if (sp.key != ro.sp[0].key) {
          ro.status = "refused";
          ro.reason = "the species differ in method (" + ro.sp[0].name + ": " + ro.sp[0].key +
                      "; " + sp.name + ": " + sp.key + "); energies are never compared across methods";
          break;
        }
    if (ro.status == "ok") {
      vector<TreatmentSpec> all = six;
      all.push_back(chosen);   // the last entry is the chosen one
      for (size_t k = 0; k < all.size() && ro.status == "ok"; k++) {
        double d = 0;
        for (const Species& sp : ro.sp) {
          ThermoResult r = computeThermo(sp.calc.in, T, ss, all[k].t, all[k].w0);
          if (!r.ok) {
            ro.status = "refused";
            ro.reason = sp.name + ": " + (r.notes.empty() ? string("no thermochemistry") : r.notes[0]);
            break;
          }
          d += sp.coeff * r.G;
          if (k + 1 == all.size()) ro.chosen.push_back(r);
        }
        if (ro.status == "ok" && k + 1 < all.size()) ro.dgs.push_back(std::make_pair(all[k].cli, d));
        if (ro.status == "ok" && k + 1 == all.size()) ro.dG = d;
      }
    }
    if (ro.status == "ok") {
      ro.lo = ro.hi = ro.dgs[0].second;
      for (const auto& e : ro.dgs) { ro.lo = std::min(ro.lo, e.second); ro.hi = std::max(ro.hi, e.second); }
      for (const Species& sp : ro.sp) ro.dE += sp.coeff * sp.calc.in.electronicEnergy;
    }
    outs.push_back(ro);
  }

  const bool json = format == "json", csv = format == "csv";
  auto softOf = [&](const ReactionOut& ro, size_t i, vector<size_t>& idx) {
    const ThermoResult& r = ro.chosen[i];
    for (size_t m = 0; m < r.modes.size(); m++)
      if (r.modes[m].freq < SOFT_CM) idx.push_back(m);
  };
  if (csv) {
    std::printf("reaction,status,dE_el_kJ_mol,dG_kJ_mol,dG_spread_kJ_mol,dG_min_kJ_mol,"
                "dG_max_kJ_mol,reason\n");
    for (const ReactionOut& ro : outs) {
      if (ro.status == "ok")
        std::printf("%s,ok,%.3f,%.3f,%.3f,%.3f,%.3f,\n", csvq(ro.text).c_str(),
                    ro.dE * KJ_PER_HARTREE, ro.dG * KJ_PER_HARTREE,
                    (ro.hi - ro.lo) * KJ_PER_HARTREE, ro.lo * KJ_PER_HARTREE,
                    ro.hi * KJ_PER_HARTREE);
      else
        std::printf("%s,refused,,,,,,%s\n", csvq(ro.text).c_str(), csvq(ro.reason).c_str());
    }
    std::printf("\nreaction,species,freq_cm1,S_J_mol_K,S_share,Gcorr_kJ_mol,Gcorr_share\n");
  } else if (json) {
    std::printf("{\n  \"T\": %.2f,\n  \"standardState\": \"%s\",\n  \"treatment\": %s,\n"
                "  \"softModeLimit_cm1\": %.0f,\n  \"reactions\": [\n",
                T, standardStateName(ss), jstr(chosen.cli).c_str(), SOFT_CM);
  } else {
    std::printf("reaction overview (T = %.2f K, %s, treatment %s)\n", T,
                standardStateName(ss), chosen.cli.c_str());
  }
  for (size_t n = 0; n < outs.size(); n++) {
    const ReactionOut& ro = outs[n];
    const bool ok = ro.status == "ok";
    if (json) {
      std::printf("    {\n      \"reaction\": %s,\n      \"status\": \"%s\",\n",
                  jstr(ro.text).c_str(), ro.status.c_str());
      if (ok) {
        std::printf("      \"dE_el_Eh\": %.6f,\n      \"dE_el_kJ\": %.3f,\n"
                    "      \"dG_Eh\": %.6f,\n      \"dG_kJ\": %.3f,\n"
                    "      \"spread_kJ\": %.3f,\n      \"dG_min_kJ\": %.3f,\n"
                    "      \"dG_max_kJ\": %.3f,\n      \"deltaG\": {",
                    ro.dE, ro.dE * KJ_PER_HARTREE, ro.dG, ro.dG * KJ_PER_HARTREE,
                    (ro.hi - ro.lo) * KJ_PER_HARTREE, ro.lo * KJ_PER_HARTREE,
                    ro.hi * KJ_PER_HARTREE);
        for (size_t k = 0; k < ro.dgs.size(); k++)
          std::printf("%s%s: %.6f", k ? ", " : "", jstr(ro.dgs[k].first).c_str(), ro.dgs[k].second);
        std::printf("},\n");
      } else {
        std::printf("      \"reason\": %s,\n", jstr(ro.reason).c_str());
      }
      std::printf("      \"species\": [");
    } else if (!csv) {
      if (ok)
        std::printf("\n%s\n  dE_el = %.2f kJ/mol   dG = %.2f kJ/mol   spread = %.2f kJ/mol\n",
                    ro.text.c_str(), ro.dE * KJ_PER_HARTREE, ro.dG * KJ_PER_HARTREE,
                    (ro.hi - ro.lo) * KJ_PER_HARTREE);
      else
        std::printf("\n%s\n  refused: %s\n", ro.text.c_str(), ro.reason.c_str());
    }
    for (size_t i = 0; i < ro.sp.size(); i++) {
      const Species& sp = ro.sp[i];
      vector<size_t> soft;
      double sTot = 0, sSoft = 0, gTot = 0, gSoft = 0;
      if (ok) {
        softOf(ro, i, soft);
        const ThermoResult& r = ro.chosen[i];
        sTot = r.S;
        gTot = r.G - sp.calc.in.electronicEnergy;
        for (size_t m : soft) { sSoft += r.modes[m].S; gSoft += r.modes[m].Gcorr; }
      }
      auto share = [](double a, double b) { return b != 0 ? a / b : 0.0; };
      if (json) {
        std::printf("%s\n        {\"name\": %s, \"url\": %s, \"coeff\": %d, \"method\": %s",
                    i ? "," : "", jstr(sp.name).c_str(), jstr(sp.url).c_str(), sp.coeff,
                    jstr(sp.key).c_str());
        if (ok) {
          std::printf(", \"S_J_mol_K\": %.4f, \"S_soft_J_mol_K\": %.4f, \"Gcorr_kJ\": %.3f, "
                      "\"Gcorr_soft_kJ\": %.3f, \"soft_modes\": [", sTot, sSoft,
                      gTot * KJ_PER_HARTREE, gSoft * KJ_PER_HARTREE);
          for (size_t q = 0; q < soft.size(); q++) {
            const ThermoMode& m = ro.chosen[i].modes[soft[q]];
            std::printf("%s{\"freq\": %.2f, \"S\": %.4f, \"S_share\": %.6f, "
                        "\"Gcorr_kJ\": %.4f, \"G_share\": %.6f}", q ? ", " : "", m.freq, m.S,
                        share(m.S, sTot), m.Gcorr * KJ_PER_HARTREE, share(m.Gcorr, gTot));
          }
          std::printf("]");
        }
        std::printf("}");
      } else if (csv) {
        if (ok)
          for (size_t m : soft) {
            const ThermoMode& md = ro.chosen[i].modes[m];
            std::printf("%s,%s,%.2f,%.4f,%.6f,%.4f,%.6f\n", csvq(ro.text).c_str(),
                        csvq(sp.name).c_str(), md.freq, md.S, share(md.S, sTot),
                        md.Gcorr * KJ_PER_HARTREE, share(md.Gcorr, gTot));
          }
      } else if (ok) {
        std::printf("  %s (%+d)  %s\n", sp.name.c_str(), sp.coeff, sp.key.c_str());
        if (soft.empty()) std::printf("    no modes below %.0f cm-1\n", SOFT_CM);
        else std::printf("    soft modes (< %.0f cm-1): %.2f of %.2f J/mol/K of S (%.1f%%), "
                         "%.2f of %.2f kJ/mol of the G correction (%.1f%%)\n", SOFT_CM, sSoft,
                         sTot, 100 * share(sSoft, sTot), gSoft * KJ_PER_HARTREE,
                         gTot * KJ_PER_HARTREE, 100 * share(gSoft, gTot));
        for (size_t m : soft) {
          const ThermoMode& md = ro.chosen[i].modes[m];
          std::printf("      %8.2f cm-1  S %8.3f J/mol/K (%5.1f%%)  G %8.3f kJ/mol (%5.1f%%)\n",
                      md.freq, md.S, 100 * share(md.S, sTot), md.Gcorr * KJ_PER_HARTREE,
                      100 * share(md.Gcorr, gTot));
        }
      }
    }
    if (json) std::printf("\n      ]\n    }%s\n", n + 1 < outs.size() ? "," : "");
  }
  if (json) std::printf("  ]\n}\n");
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
  string reaction, reactionsFile, format = "text", chosenSpec = "qrrho-grimme:100";
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
    else if (a == "--rotor" && i + 1 < argc) {
      const string v = lower(argv[++i]);
      g_rotor = v == "linear" ? 1 : v == "nonlinear" || v == "non-linear" ? 2 : 0;
    }
    else if (a == "--symmetry-tolerance" && i + 1 < argc) g_symTol = atof(argv[++i]);
    else if (a == "--reactions" && i + 1 < argc) reactionsFile = argv[++i];
    else if (a == "--format" && i + 1 < argc) format = lower(argv[++i]);
    else if (a == "--chosen" && i + 1 < argc) chosenSpec = argv[++i];
    else if (a == "--reaction" && i + 1 < argc) reaction = argv[++i];
    else if (a == "--help" || a == "-h") {
      std::printf("ecce-thermo <calc> [<calc> ...]\n"
                  "  [--T 298.15] [--standard-state 1atm|1M]\n"
                  "  [--treatment all|rrho|qrrho-grimme:100|truhlar|head-gordon,...]\n"
                  "  [--symmetry-number N] [--rotor auto|linear|nonlinear]\n"
                  "  [--symmetry-tolerance ANGSTROM] [--reaction \"LHS -> RHS\"] [--json]\n"
                  "ecce-thermo --reactions FILE [<calc> ...] [--chosen qrrho-grimme:100]\n"
                  "  [--format text|json|csv] [--T K] [--standard-state 1atm|1M]\n"
                  "  one reaction per line, 'a, 2*b -> c' (calc names or URLs)\n");
      return 0;
    }
    else if (!a.empty() && a[0] == '-') { std::printf("error: unknown option %s\n", a.c_str()); return 1; }
    else urls.push_back(a);
  }
  if (!reactionsFile.empty()) {
    if (json) format = "json";
    if (format != "text" && format != "json" && format != "csv") {
      std::printf("error: --format is text, json or csv\n");
      return 1;
    }
    return runReactions(reactionsFile, urls, T, ss, chosenSpec, format, sigmaOverride);
  }
  if (urls.empty()) { std::printf("error: at least one calculation is required\n"); return 1; }
  vector<TreatmentSpec> trs;
  string err;
  if (!parseTreatments(trSpec, trs, err)) { std::printf("error: %s\n", err.c_str()); return 1; }
  return run(urls, T, ss, trs, json, sigmaOverride, reaction);
}
