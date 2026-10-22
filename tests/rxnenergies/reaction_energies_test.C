// rxnAnalyse and rxnCsv (no wx) with constructed energies whose differences
// are worked out by hand.
#include <cmath>
#include <cstdio>
#include <string>

#include "ReactionEnergies.H"

static int failures = 0;
static void check(bool ok, const char *what)
{
  printf("%s: %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) failures++;
}
static bool near(double a, double b) { return std::fabs(a - b) < 1e-9; }
static bool has(const std::string& s, const char *t) { return s.find(t) != std::string::npos; }

static RxnState state(const char *name, double e)
{
  RxnState s;
  s.name = name;
  s.energy = e;
  s.method = "B3LYP";
  s.basis = "6-31G*";
  s.multiplicity = "Singlet";
  return s;
}

static void thermo(RxnState& s, double zpe, double h, double g)
{
  s.thermo = true;
  s.zpe = zpe;
  s.enthalpy = h;
  s.gibbs = g;
  s.temperature = 298.15;
}

int main()
{
  RxnState s[3] = {state("hcn", -93.40), state("ts", -93.33), state("hnc", -93.39)};
  RxnResult r = rxnAnalyse(s);
  check(r.ok() && r.levels.size() == 1, "energies only: one level");
  check(near(r.levels[0].barrier(), 0.07) && near(r.levels[0].reaction(), 0.01),
        "barrier 0.07 Eh, reaction 0.01 Eh");
  check(near(rxnToUnit(0.07, false), 0.07 * 2625.4996) &&
        near(rxnToUnit(0.07, true), 0.07 * 627.5095), "unit factors");
  check(r.warnings.empty() && r.notes.size() == 1 &&
        has(r.notes[0], "No thermochemistry"), "missing thermo is a note, not a warning");

  thermo(s[0], 0.010, -93.385, -93.410);
  thermo(s[1], 0.008, -93.320, -93.345);
  thermo(s[2], 0.011, -93.375, -93.400);
  r = rxnAnalyse(s);
  check(r.ok() && r.levels.size() == 4 && r.notes.empty(), "thermo: four levels");
  check(near(r.levels[1].barrier(), 0.07 - 0.002) && near(r.levels[1].reaction(), 0.011),
        "E + ZPE uses the correction");
  check(near(r.levels[2].barrier(), 0.065) && near(r.levels[3].reaction(), 0.01),
        "H and G are absolute");
  const std::string csv = rxnCsv(r, true, "ts-job");
  check(has(csv, "# calculation: ts-job") && has(csv, "barrier_kcal/mol"), "csv header");
  check(has(csv, "G,-93.41,-93.345,-93.4,40.7"), "csv G row (0.065 Eh = 40.79 kcal/mol)");

  s[2].basis = "cc-pVDZ";
  s[1].charge = 1;
  r = rxnAnalyse(s);
  check(!r.ok() && r.levels.empty() && r.problems.size() == 2,
        "two differences, two reasons, no energies");
  check(has(r.problems[0], "basis set differs") && has(r.problems[0], "product cc-pVDZ") &&
        has(r.problems[1], "charge differs"), "reasons name what differs");
  check(has(rxnCsv(r, false, "x"), "# NO ENERGIES: basis set differs"), "reason in the csv");

  s[2].basis = "6-31G*";
  s[1].charge = 0;
  s[2].temperature = 400;
  r = rxnAnalyse(s);
  check(r.warnings.size() == 1 && has(r.warnings[0], "temperatures"), "temperature mismatch");

  s[0].problem = "it has only an input structure, no optimisation";
  r = rxnAnalyse(s);
  check(!r.ok() && r.levels.empty() && r.problems.size() == 1 &&
        has(r.problems[0], "reactant (hcn)") && has(r.problems[0], "no optimisation"),
        "a problem gives no energies and says why");
  check(has(rxnCsv(r, false, "x"), "# NO ENERGIES"), "problem in the csv");

  printf(failures ? "FAILED\n" : "PASSED\n");
  return failures ? 1 : 0;
}
