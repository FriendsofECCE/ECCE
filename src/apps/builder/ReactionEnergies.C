#include <cmath>
#include <locale>
#include <sstream>

#include "ReactionEnergies.H"

namespace {
const char *const ROLE[3] = {"reactant", "transition state", "product"};

void compare(RxnResult& r, const char *what, const std::string v[3])
{
  if (v[0] == v[1] && v[1] == v[2]) return;
  std::string t = std::string(what) + " differs: ";
  for (int i = 0; i < 3; i++)
    t += std::string(i ? "; " : "") + ROLE[i] + " " + (v[i].empty() ? "(none)" : v[i]);
  r.problems.push_back(t + ".");
}
}

double rxnToUnit(double hartree, bool kcal)
{
  return hartree * (kcal ? 627.5095 : 2625.4996);
}

RxnResult rxnAnalyse(const RxnState s[3])
{
  RxnResult r;
  for (int i = 0; i < 3; i++) r.names[i] = s[i].name;
  for (int i = 0; i < 3; i++)
    if (!s[i].problem.empty())
      r.problems.push_back(std::string("The ") + ROLE[i] + " (" + s[i].name +
                           "): " + s[i].problem);
  if (!r.problems.empty()) return r;

  const std::string m[3] = {s[0].method, s[1].method, s[2].method};
  const std::string b[3] = {s[0].basis, s[1].basis, s[2].basis};
  const std::string sv[3] = {s[0].solvent, s[1].solvent, s[2].solvent};
  const std::string mu[3] = {s[0].multiplicity, s[1].multiplicity, s[2].multiplicity};
  std::string ch[3];
  for (int i = 0; i < 3; i++) ch[i] = std::to_string(s[i].charge);
  compare(r, "method", m);
  compare(r, "basis set", b);
  compare(r, "solvent model", sv);
  compare(r, "charge", ch);
  compare(r, "multiplicity", mu);
  // Energies from different settings are not comparable: show none.
  if (!r.problems.empty()) return r;

  RxnLevel e;
  e.label = "E";
  for (int i = 0; i < 3; i++) e.value[i] = s[i].energy;
  r.levels.push_back(e);

  if (!(s[0].thermo && s[1].thermo && s[2].thermo)) {
    std::string who;
    for (int i = 0; i < 3; i++)
      if (!s[i].thermo) who += std::string(who.empty() ? "" : ", ") + ROLE[i];
    r.notes.push_back("No thermochemistry for the " + who +
                      " (needs a frequency calculation): only the electronic "
                      "energies are compared.");
    return r;
  }
  if (std::fabs(s[0].temperature - s[1].temperature) > 0.01 ||
      std::fabs(s[1].temperature - s[2].temperature) > 0.01)
    r.warnings.push_back("The thermochemistry temperatures differ, so "
                         "H and G are not comparable.");
  RxnLevel z = e, h = e, g = e;
  z.label = "E + ZPE";
  h.label = "H";
  g.label = "G";
  for (int i = 0; i < 3; i++) {
    z.value[i] = s[i].energy + s[i].zpe;
    h.value[i] = s[i].enthalpy;
    g.value[i] = s[i].gibbs;
  }
  r.levels.push_back(z);
  r.levels.push_back(h);
  r.levels.push_back(g);
  return r;
}

std::string rxnCsv(const RxnResult& r, bool kcal, const std::string& calculation)
{
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out.precision(10);
  const std::string u = kcal ? "kcal/mol" : "kJ/mol";
  out << "# ECCE reaction energies\n# calculation: " << calculation << "\n";
  out << "# reactant: " << r.names[0] << "\n# transition state: " << r.names[1]
      << "\n# product: " << r.names[2] << "\n";
  for (const std::string& p : r.problems) out << "# NO ENERGIES: " << p << "\n";
  for (const std::string& w : r.warnings) out << "# WARNING: " << w << "\n";
  for (const std::string& n : r.notes) out << "# note: " << n << "\n";
  out << "quantity,reactant_hartree,ts_hartree,product_hartree,barrier_" << u
      << ",reaction_" << u << "\n";
  for (const RxnLevel& l : r.levels)
    out << l.label << "," << l.value[0] << "," << l.value[1] << "," << l.value[2]
        << "," << rxnToUnit(l.barrier(), kcal) << ","
        << rxnToUnit(l.reaction(), kcal) << "\n";
  return out.str();
}
