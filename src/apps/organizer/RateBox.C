#include "RateBox.H"

#include <cmath>
#include <cstdio>
#include <cstdlib>

using std::string;
using std::vector;

namespace {

string f(const char *fmt, double v)
{
  char b[64];
  snprintf(b, sizeof b, fmt, v);
  return b;
}

string unitOf(const string& u) { return u.empty() ? string() : " " + u; }

}  // namespace

// Shown as 7.971 × 10⁷ rather than 7.971e+07; plain digits for moderate values.
string rateNum(double v)
{
  if (std::isnan(v)) return "-";
  if (v == 0 || (std::fabs(v) >= 0.01 && std::fabs(v) < 1e4)) return f("%.4g", v);
  const char *sup[] = {"\xE2\x81\xB0", "\xC2\xB9", "\xC2\xB2", "\xC2\xB3", "\xE2\x81\xB4",
                       "\xE2\x81\xB5", "\xE2\x81\xB6", "\xE2\x81\xB7", "\xE2\x81\xB8", "\xE2\x81\xB9"};
  const string e = f("%.3e", v);
  const size_t at = e.find('e');
  int x = atoi(e.c_str() + at + 1);
  string out = e.substr(0, at) + " \xC3\x97 10";
  if (x < 0) { out += "\xE2\x81\xBB"; x = -x; }
  for (char c : std::to_string(x)) out += sup[c - '0'];
  return out;
}

bool rateRequest(const vector<KeyStep>& steps, RateRequest& req, string& waiting)
{
  const KeyStep *re = 0, *ts = 0, *pr = 0;
  for (const KeyStep& s : steps) {
    if (s.keyRole == "reactant" && !re) re = &s;
    else if (s.keyRole == "ts" && !ts) ts = &s;
    else if (s.keyRole == "product" && !pr) pr = &s;
  }
  if (!re || !ts) return false;
  vector<string> missing;
  auto path = [&](const KeyStep *s, const char *role) {
    if (!s) return string();
    if (!s->complete || !s->term.state.problem.empty()) {
      missing.push_back(string(role) + " " + s->name);
      return string();
    }
    const string p = rateLocalPath(s->url);
    if (p.empty()) missing.push_back(string(role) + " " + s->name + " (a server calculation)");
    return p;
  };
  req.reactants = vector<string>(1, path(re, "reactant"));
  req.ts = path(ts, "TS");
  req.products.clear();
  if (pr) req.products.push_back(path(pr, "product"));
  if (!missing.empty()) {
    waiting = "Waiting for ";
    for (size_t i = 0; i < missing.size(); i++)
      waiting += (i ? ", " : "") + missing[i];
    waiting += " to finish with frequencies.";
  }
  return true;
}

RateBoxResult rateBox(const vector<KeyStep>& steps, const string& temperature,
                      const string& treatment, const string& standardState)
{
  RateBoxResult b;
  RateRequest req;
  req.temperatures = temperature;
  req.treatment = treatment;
  req.standardState = standardState;
  string waiting;
  b.applicable = rateRequest(steps, req, waiting);
  if (!b.applicable) return b;
  b.level = treatment + ", " + (standardState == "1M" ? "1 M" : "1 atm") + " standard state";
  if (!waiting.empty()) { b.note = waiting; return b; }
  b.data = runRate(req);
  if (!b.data.ok) { b.note = b.data.error; return b; }
  b.ready = true;
  const RateRow& r = b.data.rows[0];
  const RateResult& d = b.data;
  const string u = unitOf(d.unit);
  b.rows.push_back({"\xCE\x94G\xE2\x80\xA1 at " + f("%.2f", r.T) + " K", f("%.2f", r.dG) + " kJ/mol"});
  b.rows.push_back({"k (TST)", rateNum(r.k) + u});
  b.rows.push_back({"\xCE\xBA Wigner  " + f("%.3f", r.kappaWigner), "k " + rateNum(r.kWigner) + u});
  b.rows.push_back({"\xCE\xBA Eckart  " + (std::isnan(r.kappaEckart) ? string("-")
                                                                     : f("%.3f", r.kappaEckart)),
                    "k " + rateNum(r.kEckart) + u});
  if (d.hasProduct) {
    b.rows.push_back({"k reverse (Eckart)", rateNum(r.kRevEckart) + unitOf(d.unitReverse)});
    b.rows.push_back({"K (equilibrium)", rateNum(r.Keq)});
  }
  for (const string& n : d.notes) b.note += (b.note.empty() ? "" : " ") + n + ".";
  return b;
}

string RateBoxResult::text() const
{
  if (!applicable) return "";
  string t = "Rate\n";
  if (!level.empty()) t += level + "\n";
  for (size_t i = 0; i < rows.size(); i++) t += rows[i].first + ": " + rows[i].second + "\n";
  if (!note.empty()) t += note + "\n";
  return t;
}

string rateDetailsText(const RateResult& r)
{
  string t;
  const string u = unitOf(r.unit);
  t += "Forward, k in" + u + " (" + r.treatment + ", " + r.standardState + ")\n";
  t += "     T/K  dG\xE2\x80\xA1/kJ/mol    k TST       k Wigner    k Eckart\n";
  for (const RateRow& w : r.rows)
    t += f("%8.2f", w.T) + f("  %10.2f", w.dG) + "  " + rateNum(w.k) + "  " + rateNum(w.kWigner) +
         "  " + rateNum(w.kEckart) + "\n";
  if (r.hasProduct) {
    t += "\nReverse, k in" + unitOf(r.unitReverse) + "\n";
    t += "     T/K  dG_rxn/kJ/mol  K_eq        k TST       k Wigner    k Eckart\n";
    for (const RateRow& w : r.rows)
      t += f("%8.2f", w.T) + f("  %12.2f", w.dGrxn) + "  " + rateNum(w.Keq) + "  " +
           rateNum(w.kRev) + "  " + rateNum(w.kRevWigner) + "  " + rateNum(w.kRevEckart) + "\n";
  }
  if (r.fits.empty()) t += "\nNo fit: it needs two or more temperatures with a rate constant at each.\n";
  else t += "\nFits over the range (tunnelling included in k)\n";
  for (const RateFit& x : r.fits)
    t += "  " + x.method + ": Arrhenius A = " + rateNum(x.A) + u + ", Ea = " + f("%.2f", x.Ea) +
         " kJ/mol; Eyring dH\xE2\x80\xA1 = " + f("%.2f", x.dH) + " kJ/mol, dS\xE2\x80\xA1 = " +
         f("%.2f", x.dS) + " J/mol/K\n";
  for (const string& n : r.notes) t += "note: " + n + "\n";
  return t;
}

string rateDetailsCsv(const RateResult& r)
{
  auto c = [](double v) { return std::isnan(v) ? string() : f("%.6g", v); };
  string t = "T_K,dG_act_kJ_mol,dH_act_kJ_mol,dS_act_J_mol_K,kappa_wigner,kappa_eckart,"
             "k_tst,k_wigner,k_eckart,k_unit";
  if (r.hasProduct) t += ",dG_rxn_kJ_mol,K_eq,k_rev_tst,k_rev_wigner,k_rev_eckart,k_rev_unit";
  t += "\n";
  for (const RateRow& w : r.rows) {
    t += c(w.T) + "," + c(w.dG) + "," + c(w.dH) + "," + c(w.dS) + "," + c(w.kappaWigner) + "," +
         c(w.kappaEckart) + "," + c(w.k) + "," + c(w.kWigner) + "," + c(w.kEckart) + "," + r.unit;
    if (r.hasProduct)
      t += "," + c(w.dGrxn) + "," + c(w.Keq) + "," + c(w.kRev) + "," + c(w.kRevWigner) + "," +
           c(w.kRevEckart) + "," + r.unitReverse;
    t += "\n";
  }
  for (const RateFit& x : r.fits)
    t += "\n# " + x.method + " fit: A=" + c(x.A) + " Ea_kJ_mol=" + c(x.Ea) + " dH_act_kJ_mol=" +
         c(x.dH) + " dS_act_J_mol_K=" + c(x.dS) + "\n";
  return t;
}
