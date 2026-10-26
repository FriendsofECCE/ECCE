// compareEnergies on constructed entries, differences worked out by hand.
#include <cmath>
#include <cstdio>
#include <string>

#include "CompareEnergies.H"

static int failures = 0;
static void check(bool ok, const char *what)
{
  printf("%s: %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) failures++;
}
static bool near(double a, double b, double tol = 1e-9) { return std::fabs(a - b) < tol; }
static bool has(const std::string& s, const char *t) { return s.find(t) != std::string::npos; }

static EnergyEntry entry(const char *name, double e, const char *units = "Hartree")
{
  EnergyEntry s;
  s.name = name;
  s.exists = true;
  s.code = "ECCE-QM";
  s.method = "HF";
  s.basis = "aug-cc-pVTZ";
  s.energy.found = true;
  s.energy.value = e;
  s.energy.units = units;
  return s;
}

int main()
{
  EnergyEntry a = entry("Li", -7.43), b = entry("Li+", -7.24);
  EnergyComparison c = compareEnergies(a, b);
  check(c.ok && near(c.delta, 0.19), "E(B) - E(A) = 0.19 Eh");
  check(near(hartreeTo(c.delta, 1), 0.19 * 2625.4996394799, 1e-6) &&
        near(hartreeTo(c.delta, 2), 0.19 * 627.5094740631, 1e-6) &&
        near(hartreeTo(c.delta, 3), 0.19 * 27.211386245988, 1e-9),
        "kJ/mol, kcal/mol and eV factors");
  check(c.notes.empty(), "same code, method and basis: no note");

  b.method = "B3LYP";
  b.basis = "6-31G*";
  b.code = "ORCA";
  c = compareEnergies(a, b);
  check(c.ok && c.notes.size() == 3 && has(c.notes[0], "code differs") &&
        has(c.notes[1], "method differs") && has(c.notes[2], "basis set differs"),
        "code, method and basis each give a note, and the difference is still made");

  b = entry("Li+", -7.24);
  b.energy.found = false;
  c = compareEnergies(a, b);
  check(!c.ok && c.notes.size() == 1 && has(c.notes[0], "has no final energy"),
        "no final energy: a note, no difference");

  b = entry("x", -4684.0, "kcal/Mole");
  c = compareEnergies(a, b);
  check(c.ok && near(c.b, -4684.0 / (2625.4996394799 / 4.184), 1e-9),
        "kcal/mol energies are converted to hartree");

  b = entry("gone", 0);
  b.exists = false;
  c = compareEnergies(a, b);
  check(!c.ok && has(c.notes[0], "cannot be read"), "unreadable calculation");

  printf("%d failure(s)\n", failures);
  return failures ? 1 : 0;
}
