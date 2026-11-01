#include "KeyData.H"

#include <cstdio>
#include <set>

#include "tdat/ReactionEnergies.H"

using std::string;
using std::vector;

namespace {

string fmt(const char *f, double v)
{
  char buf[64];
  snprintf(buf, sizeof buf, f, v);
  return buf;
}

string levelOf(const EnergyTerm& t)
{
  string l = t.code.empty() ? string() : t.code + "  ";
  l += t.xc.empty() ? t.state.method : t.xc;
  if (!t.state.basis.empty()) l += "/" + t.state.basis;
  if (!t.state.solvent.empty() && t.state.solvent != "none")
    l += " (" + t.state.solvent + ")";
  return l;
}

const KeyStep *withRole(const vector<KeyStep>& s, const char *role)
{
  for (size_t i = 0; i < s.size(); i++)
    if (s[i].keyRole == role && s[i].complete && s[i].term.state.problem.empty())
      return &s[i];
  return 0;
}

}  // namespace


KeyDataResult keyData(const vector<KeyStep>& steps)
{
  KeyDataResult r;
  size_t done = 0;
  std::set<string> levels;
  for (size_t i = 0; i < steps.size(); i++)
    if (steps[i].complete && steps[i].term.state.problem.empty()) {
      done++;
      levels.insert(levelOf(steps[i].term));
    }
  r.note = std::to_string(done) + " of " + std::to_string(steps.size()) +
           " steps complete";
  if (levels.size() > 1) {
    r.shown = false;
    r.note = "The steps differ in code or level of theory; energies of "
             "different methods are not compared.";
    return r;
  }
  if (!levels.empty()) r.level = *levels.begin();

  for (size_t i = 0; i < steps.size(); i++) {
    const KeyStep& s = steps[i];
    if (!s.complete || !s.term.state.problem.empty()) {
      r.rows.push_back({s.name, "waiting"});
      continue;
    }
    string v = "E = " + fmt("%.6f", s.term.state.energy) + " Eh";
    if (s.thermo && s.term.state.thermo)
      v += ",  G = " + fmt("%.6f", s.term.state.gibbs) + " Eh";
    r.rows.push_back({s.name, v});
  }

  const KeyStep *re = withRole(steps, "reactant");
  const KeyStep *pr = withRole(steps, "product");
  const KeyStep *ts = withRole(steps, "ts");
  auto kj = [](double h) { return fmt("%+.1f", rxnToUnit(h, false)) + " kJ/mol"; };
  auto gOf = [](const KeyStep *s) {
    return s->thermo && s->term.state.thermo;
  };
  if (re && pr) {
    r.rows.push_back({"Reaction energy  E(product) - E(reactant)",
                      kj(pr->term.state.energy - re->term.state.energy)});
    if (gOf(re) && gOf(pr))
      r.rows.push_back({"Reaction free energy  G(product) - G(reactant)",
                        kj(pr->term.state.gibbs - re->term.state.gibbs)});
  }
  if (re && ts) {
    r.rows.push_back({"Barrier  E(TS) - E(reactant)",
                      kj(ts->term.state.energy - re->term.state.energy)});
    if (gOf(re) && gOf(ts))
      r.rows.push_back({"Free-energy barrier  G(TS) - G(reactant)",
                        kj(ts->term.state.gibbs - re->term.state.gibbs)});
  }
  return r;
}


string KeyDataResult::text() const
{
  string t = "Key data\n";
  if (!level.empty()) t += level + "\n";
  for (size_t i = 0; i < rows.size(); i++)
    t += rows[i].first + ": " + rows[i].second + "\n";
  t += note + "\n";
  return t;
}
