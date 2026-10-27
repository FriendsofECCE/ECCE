// compareEnergies on constructed entries, differences worked out by hand.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

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

  check(calcNameLess("C2", "C10") && !calcNameLess("C10", "C2"),
        "numbers in names compare as numbers");
  check(calcNameLess("alpha", "Beta") && calcNameLess("Beta", "gamma"),
        "case is ignored");
  check(!calcNameLess("Li", "li") && !calcNameLess("li", "Li"),
        "names equal but for case are equal");
  check(calcNameLess("H2O", "H2O 2") && calcNameLess("x02", "x10") &&
        calcNameLess("x2", "x10a"), "prefix, leading zeros, number then text");
  {
    std::vector<std::string> n = { "O2", "c10", "C2", "b", "C1" };
    std::sort(n.begin(), n.end(), calcNameLess);
    check(n[0] == "b" && n[1] == "C1" && n[2] == "C2" && n[3] == "c10" &&
          n[4] == "O2", "sorted list");
  }

  {
    check(fmtAbsolute(-76.0107465054306) == "-76.010747", "absolute: 6 decimals");
    check(fmtDifference(0.33176911, EU_KJ) == "871.1" &&
          fmtDifference(0.33176911, EU_KCAL) == "208.2" &&
          fmtDifference(0.33176911, EU_EV) == "9.03",
          "differences: 1 decimal kJ/mol and kcal/mol, 2 in eV");
    check(fmtDifference(-1e-9, EU_KJ) == "0.0", "a rounded zero has no minus sign");
    EnergyEntry x = entry("a", -7.43), y = entry("b", -7.24);
    EnergyComparison k = compareEnergies(x, y);
    const std::string shown = comparisonResults(x, y, k);
    check(has(shown, "E(A) -7.430000 Hartree") && has(shown, "498.8 kJ/mol") &&
          !has(shown, "498.844"), "shown text is rounded");
    check(has(comparisonCopyText(x, y, k), "498.8449"), "Copy keeps full values");
  }

  printf("%d failure(s)\n", failures);
  return failures ? 1 : 0;
}
