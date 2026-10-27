#include "tdat/EnergySums.H"

#include <cmath>
#include <cstdio>
#include <functional>
#include <set>

#include "tdat/EnergyFormat.H"

using std::string;
using std::vector;

namespace {

string num(double v, int decimals)
{
  char buf[64];
  snprintf(buf, sizeof buf, "%.*f", decimals, v);
  return buf;
}

string padLeft(const string& s, size_t w)
{
  return s.size() >= w ? s : string(w - s.size(), ' ') + s;
}

string padRight(const string& s, size_t w)
{
  return s.size() >= w ? s : s + string(w - s.size(), ' ');
}

string noneName(const string& s)
{
  return (s.empty() || s == "None" || s == "none" || s == "NONE") ? "none" : s;
}

typedef std::function<string(const EnergyTerm&)> Getter;

// The distinct values among the terms of one side, joined with "/".
string valuesOf(const vector<EnergyTerm>& t, const Getter& get)
{
  std::set<string> seen;
  string r;
  for (const EnergyTerm& e : t) {
    const string x = get(e);
    if (!seen.insert(x).second) continue;
    r += (r.empty() ? "" : "/") + (x.empty() ? string("(none)") : x);
  }
  return r;
}

size_t distinctCount(const vector<EnergyTerm>& a, const vector<EnergyTerm>& b,
                     const Getter& get)
{
  std::set<string> v;
  for (const EnergyTerm& e : a) v.insert(get(e));
  for (const EnergyTerm& e : b) v.insert(get(e));
  return v.size();
}

double sumOf(const vector<EnergyTerm>& t, double RxnState::*f, bool addZpe)
{
  double s = 0;
  for (const EnergyTerm& e : t)
    s += e.coeff * (e.state.*f + (addZpe ? e.state.zpe : 0));
  return s;
}

std::map<string, int> atomSum(const vector<EnergyTerm>& t)
{
  std::map<string, int> m;
  for (const EnergyTerm& e : t)
    for (const auto& kv : e.atoms) m[kv.first] += e.coeff * kv.second;
  for (auto it = m.begin(); it != m.end(); )
    it = it->second == 0 ? m.erase(it) : std::next(it);
  return m;
}

}

string atomFormula(const std::map<string, int>& atoms)
{
  string f;
  for (const auto& kv : atoms)
    f += kv.first + (kv.second == 1 ? string() : std::to_string(kv.second));
  return f.empty() ? "(none)" : f;
}

SumResult sumEnergies(const vector<EnergyTerm>& a, const vector<EnergyTerm>& b)
{
  SumResult r;
  const Getter solv = [](const EnergyTerm& e) { return noneName(e.state.solvent); };
  r.solventA = valuesOf(a, solv);
  r.solventB = valuesOf(b, solv);
  if (a.empty() || b.empty()) {
    r.notes.push_back("Each side needs at least one calculation.");
    return r;
  }
  for (const vector<EnergyTerm> *side : {&a, &b})
    for (const EnergyTerm& e : *side)
      if (!e.state.problem.empty())
        r.notes.push_back(e.state.name + ": " + e.state.problem);
  if (!r.notes.empty()) return r;
  r.ok = true;
  bool scans[2] = {false, false};
  for (int i = 0; i < 2; i++)
    for (const EnergyTerm& e : i ? b : a) {
      if (e.heat)
        r.warnings.push_back(e.state.name + " gives a heat of formation, not "
                             "a total energy.");
      if (e.scanned) scans[i] = true;
    }
  if (scans[0] != scans[1])
    r.warnings.push_back("One side has the last point of a relaxed scan, the "
                         "other does not.");

  const struct { const char *what; Getter get; } d[4] = {
    {"code", [](const EnergyTerm& e) { return e.code; }},
    {"method", [](const EnergyTerm& e) { return e.state.method; }},
    {"basis set", [](const EnergyTerm& e) { return e.state.basis; }},
    {"dispersion correction",
     [](const EnergyTerm& e) { return noneName(e.dispersion); }}};
  for (int i = 0; i < 4; i++)
    if (distinctCount(a, b, d[i].get) > 1)
      r.warnings.push_back(string("The ") + d[i].what + " differs: A " +
          valuesOf(a, d[i].get) + ", B " + valuesOf(b, d[i].get) + ".");

  // Gas phase against solvent for one molecule is a solvation energy.
  const bool solvDiffers = distinctCount(a, b, solv) > 1;
  bool solvation = false;
  if (a.size() == 1 && b.size() == 1 && solvDiffers) {
    const RxnState& x = a[0].state;
    const RxnState& y = b[0].state;
    const bool oneGas = (noneName(x.solvent) == "none") !=
                        (noneName(y.solvent) == "none");
    solvation = oneGas && x.charge == y.charge &&
        x.multiplicity == y.multiplicity && x.method == y.method &&
        x.basis == y.basis && !a[0].atoms.empty() && a[0].atoms == b[0].atoms &&
        noneName(a[0].dispersion) == noneName(b[0].dispersion);
    if (solvation)
      r.deltaLabel = noneName(y.solvent) == "none"
          ? "minus the solvation energy (A is in solvent, B in gas phase)"
          : "the solvation energy (B in solvent, A in gas phase)";
  }
  if (solvDiffers && !solvation)
    r.warnings.push_back("The solvent model differs: A " + r.solventA + ", B " +
                         r.solventB + ".");

  bool atomsKnown = true;
  for (const vector<EnergyTerm> *side : {&a, &b})
    for (const EnergyTerm& e : *side)
      if (e.atoms.empty()) atomsKnown = false;
  if (atomsKnown) {
    const std::map<string, int> aa = atomSum(a), ab = atomSum(b);
    if (aa != ab)
      r.warnings.push_back("The atoms on the two sides differ (not a balanced "
                           "reaction): A " + atomFormula(aa) + ", B " +
                           atomFormula(ab) + ".");
  }

  SumRow e;
  e.label = "E";
  e.available = true;
  e.a = sumOf(a, &RxnState::energy, false);
  e.b = sumOf(b, &RxnState::energy, false);
  r.rows.push_back(e);

  string without;
  double tMin = 0, tMax = 0;
  bool first = true;
  for (const vector<EnergyTerm> *side : {&a, &b})
    for (const EnergyTerm& t : *side) {
      if (!t.state.thermo) {
        without += (without.empty() ? "" : ", ") + t.state.name;
        continue;
      }
      if (t.state.temperature <= 0) continue;
      if (first || t.state.temperature < tMin) tMin = t.state.temperature;
      if (first || t.state.temperature > tMax) tMax = t.state.temperature;
      first = false;
    }
  const bool thermo = without.empty();
  const bool tDiffer = !first && std::fabs(tMax - tMin) > 0.01;
  string tag = " (T)";
  if (!first && !tDiffer) {
    char buf[32];
    snprintf(buf, sizeof buf, " (%.0f K)", tMin);
    tag = buf;
  }
  if (!thermo)
    r.notes.push_back("No frequencies for " + without +
                      ": E + ZPE, H and G need a frequency calculation on "
                      "every calculation.");
  if (thermo && tDiffer)
    r.warnings.push_back("The thermochemistry temperatures differ, so H and G "
                         "are not comparable.");
  struct { const char *label; double RxnState::*f; bool zpe; } lv[3] = {
    {"E + ZPE", &RxnState::energy, true}, {"H", &RxnState::enthalpy, false},
    {"G", &RxnState::gibbs, false}};
  for (int i = 0; i < 3; i++) {
    SumRow w;
    w.label = lv[i].label + (i ? tag : string());
    w.available = thermo;
    if (!thermo) w.reason = "needs frequencies";
    else {
      w.a = sumOf(a, lv[i].f, lv[i].zpe);
      w.b = sumOf(b, lv[i].f, lv[i].zpe);
    }
    r.rows.push_back(w);
  }
  return r;
}

string sumTable(const SumResult& r, bool full)
{
  if (!r.ok) return string();
  auto abs = [&](double v) { return full ? num(v, 10) : fmtAbsolute(v); };
  auto diff = [&](double v, int u) {
    return full ? num(hartreeTo(v, u), 10) : fmtDifference(v, u);
  };
  const size_t LW = 12, AW = 15, DW = full ? 18 : 11, EW = full ? 16 : 10;
  string t = padRight("", LW) + padLeft("A (sum)", AW) + padLeft("B (sum)", AW) +
             padLeft("B - A", DW) + "\n";
  t += padRight("", LW) + padLeft("Hartree", AW) + padLeft("Hartree", AW) +
       padLeft("kJ/mol", DW) + padLeft("kcal/mol", DW) + padLeft("eV", EW) + "\n";
  for (const SumRow& w : r.rows) {
    t += padRight(w.label, LW);
    if (!w.available) {
      t += w.reason + "\n";
      continue;
    }
    t += padLeft(abs(w.a), AW) + padLeft(abs(w.b), AW) +
         padLeft(diff(w.delta(), EU_KJ), DW) +
         padLeft(diff(w.delta(), EU_KCAL), DW) +
         padLeft(diff(w.delta(), EU_EV), EW) + "\n";
  }
  t += "Solvent: " + r.solventA + " / " + r.solventB + "\n";
  if (!r.deltaLabel.empty()) t += "B - A is " + r.deltaLabel + ".\n";
  return t;
}

string sumNotes(const SumResult& r)
{
  string t;
  for (const string& w : r.warnings) t += "Warning: " + w + "\n";
  for (const string& n : r.notes) t += n + "\n";
  return t;
}

string sumText(const SumResult& r, bool full)
{
  return sumTable(r, full) + sumNotes(r);
}
