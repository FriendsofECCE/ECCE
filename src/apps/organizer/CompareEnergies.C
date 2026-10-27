#include "CompareEnergies.H"

#include <cctype>
#include <cstdio>


using std::string;

namespace {
const double KJ_PER_HARTREE = 2625.4996394799;
const double KCAL_PER_HARTREE = KJ_PER_HARTREE / 4.184;
const double EV_PER_HARTREE = 27.211386245988;

// Hartree for a stored energy; false if the unit is not an energy we know.
bool toHartree(const FinalEnergy& e, double *h)
{
  if (e.units.empty() || e.units == "NA" || e.units == "Hartree") *h = e.value;
  else if (e.units == "kcal/Mole") *h = e.value / KCAL_PER_HARTREE;
  else if (e.units == "kJoule/Mole") *h = e.value / KJ_PER_HARTREE;
  else if (e.units == "eV") *h = e.value / EV_PER_HARTREE;
  else return false;
  return true;
}

string num(double v, int decimals)
{
  char buf[64];
  snprintf(buf, sizeof buf, "%.*f", decimals, v);
  return buf;
}
}

EnergyComparison compareEnergies(const EnergyEntry& a, const EnergyEntry& b)
{
  EnergyComparison r;
  const EnergyEntry *e[2] = {&a, &b};
  const char *who[2] = {"A", "B"};
  double h[2] = {0, 0};
  bool have[2] = {false, false};
  for (int i = 0; i < 2; i++) {
    if (!e[i]->exists)
      r.notes.push_back(string("Calculation ") + who[i] + " cannot be read.");
    else if (!e[i]->energy.found)
      r.notes.push_back(string(e[i]->name) + " (" + who[i] +
                        ") has no final energy.");
    else if (!toHartree(e[i]->energy, &h[i]))
      r.notes.push_back(string(e[i]->name) + " (" + who[i] +
                        ") gives its energy in " + e[i]->energy.units +
                        ", which cannot be converted.");
    else {
      have[i] = true;
      if (e[i]->energy.heat)
        r.notes.push_back(string(e[i]->name) + " (" + who[i] +
                          ") gives a heat of formation, not a total energy.");
    }
  }
  r.ok = have[0] && have[1];
  if (r.ok) {
    r.a = h[0];
    r.b = h[1];
    r.delta = h[1] - h[0];
  }
  if (a.exists && b.exists) {
    struct { const char *what; const string& x; const string& y; } d[3] = {
      {"code", a.code, b.code}, {"method", a.method, b.method},
      {"basis set", a.basis, b.basis}};
    for (int i = 0; i < 3; i++)
      if (d[i].x != d[i].y)
        r.notes.push_back(string("The ") + d[i].what + " differs: A " +
                          (d[i].x.empty() ? "(none)" : d[i].x) + ", B " +
                          (d[i].y.empty() ? "(none)" : d[i].y) + ".");
    if (r.ok && (a.energy.scanned != b.energy.scanned))
      r.notes.push_back("One energy is the last point of a relaxed scan, "
                        "the other is not.");
  }
  return r;
}

double hartreeTo(double hartree, int unit)
{
  switch (unit) {
    case 1: return hartree * KJ_PER_HARTREE;
    case 2: return hartree * KCAL_PER_HARTREE;
    case 3: return hartree * EV_PER_HARTREE;
  }
  return hartree;
}

const char *energyUnitName(int unit)
{
  static const char *const n[4] = {"Hartree", "kJ/mol", "kcal/mol", "eV"};
  return n[unit < 0 || unit > 3 ? 0 : unit];
}

string comparisonResults(const EnergyEntry& a, const EnergyEntry& b,
                         const EnergyComparison& c)
{
  string t;
  if (!c.ok) return t;
  t += "E(A) " + num(c.a, 8) + " Hartree  " + a.name + "\n";
  t += "E(B) " + num(c.b, 8) + " Hartree  " + b.name + "\n";
  t += "E(B) - E(A):\n";
  for (int u = 0; u < 4; u++)
    t += "  " + num(hartreeTo(c.delta, u), u == 0 ? 8 : 4) + " " +
         energyUnitName(u) + "\n";
  return t;
}

string comparisonNotes(const EnergyComparison& c)
{
  string t;
  for (size_t i = 0; i < c.notes.size(); i++) t += c.notes[i] + "\n";
  return t;
}

string comparisonText(const EnergyEntry& a, const EnergyEntry& b,
                      const EnergyComparison& c)
{
  string t = comparisonResults(a, b, c);
  for (size_t i = 0; i < c.notes.size(); i++) t += "Note: " + c.notes[i] + "\n";
  return t;
}


bool calcNameLess(const string& a, const string& b)
{
  size_t i = 0, j = 0;
  while (i < a.size() && j < b.size()) {
    if (isdigit((unsigned char)a[i]) && isdigit((unsigned char)b[j])) {
      size_t i2 = i, j2 = j;
      while (i2 < a.size() && isdigit((unsigned char)a[i2])) i2++;
      while (j2 < b.size() && isdigit((unsigned char)b[j2])) j2++;
      // Leading zeros do not count; the longer number is the larger.
      size_t ti = i, tj = j;
      while (ti + 1 < i2 && a[ti] == '0') ti++;
      while (tj + 1 < j2 && b[tj] == '0') tj++;
      if (i2 - ti != j2 - tj) return i2 - ti < j2 - tj;
      int c = a.compare(ti, i2 - ti, b, tj, j2 - tj);
      if (c != 0) return c < 0;
      i = i2;
      j = j2;
    } else {
      int x = tolower((unsigned char)a[i]), y = tolower((unsigned char)b[j]);
      if (x != y) return x < y;
      i++;
      j++;
    }
  }
  return a.size() - i < b.size() - j;
}
