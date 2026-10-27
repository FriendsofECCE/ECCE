#include "tdat/EnergyFormat.H"

#include <cstdio>

namespace {
const double KJ_PER_HARTREE = 2625.4996394799;
const double KCAL_PER_HARTREE = KJ_PER_HARTREE / 4.184;
const double EV_PER_HARTREE = 27.211386245988;

std::string num(double v, int decimals)
{
  char buf[64];
  snprintf(buf, sizeof buf, "%.*f", decimals, v);
  std::string s = buf;
  // A value that rounds to zero is shown without a minus sign.
  if (s.size() > 1 && s[0] == '-' && s.find_first_not_of("-0.") == std::string::npos)
    s.erase(0, 1);
  return s;
}
}

double hartreeTo(double hartree, int unit)
{
  switch (unit) {
    case EU_KJ: return hartree * KJ_PER_HARTREE;
    case EU_KCAL: return hartree * KCAL_PER_HARTREE;
    case EU_EV: return hartree * EV_PER_HARTREE;
  }
  return hartree;
}

const char *energyUnitName(int unit)
{
  static const char *const n[4] = {"Hartree", "kJ/mol", "kcal/mol", "eV"};
  return n[unit < 0 || unit > 3 ? 0 : unit];
}

std::string fmtAbsolute(double hartree) { return num(hartree, 6); }

std::string fmtDifference(double hartree, int unit)
{
  if (unit == EU_HARTREE) return num(hartree, 6);
  return num(hartreeTo(hartree, unit), unit == EU_EV ? 2 : 1);
}
