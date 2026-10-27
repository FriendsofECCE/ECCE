// sumEnergies on constructed terms, differences worked out by hand (#573, #612).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "CompareEnergies.H"
#include "tdat/EnergyFormat.H"

static int failures = 0;
static void check(bool ok, const char *what)
{
  printf("%s: %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) failures++;
}
static bool near(double a, double b, double tol = 1e-9) { return std::fabs(a - b) < tol; }
static bool has(const std::string& s, const char *t) { return s.find(t) != std::string::npos; }
static bool anyHas(const std::vector<std::string>& v, const char *t)
{
  for (const std::string& s : v) if (has(s, t)) return true;
  return false;
}

typedef std::vector<EnergyTerm> Side;

static EnergyTerm term(const char *name, double e, const char *atoms = "", int coeff = 1)
{
  EnergyTerm t;
  t.state.name = name;
  t.state.energy = e;
  t.state.method = "B3LYP";
  t.state.basis = "6-31G*";
  t.state.multiplicity = "Singlet";
  t.code = "ORCA";
  t.coeff = coeff;
  // atoms "C4H6": element letters followed by a count
  for (const char *p = atoms; *p; ) {
    std::string el(1, *p++);
    int n = 0;
    while (*p >= '0' && *p <= '9') n = n * 10 + (*p++ - '0');
    t.atoms[el] += n ? n : 1;
  }
  return t;
}

static void freq(EnergyTerm& t, double zpe, double h, double g, double T = 298.15)
{
  t.state.thermo = true;
  t.state.zpe = zpe;
  t.state.enthalpy = h;
  t.state.gibbs = g;
  t.state.temperature = T;
}

int main()
{
  // Diels-Alder: butadiene + ethene -> cyclohexene
  Side a = {term("butadiene", -155.9, "C4H6"), term("ethene", -78.5, "C2H4")};
  Side b = {term("cyclohexene", -234.5, "C6H10")};
  SumResult r = sumEnergies(a, b);
  check(r.ok && r.rows.size() == 4 && r.rows[0].label == "E", "four rows");
  check(near(r.rows[0].a, -234.4) && near(r.rows[0].b, -234.5) &&
        near(r.rows[0].delta(), -0.1), "sum of A, B and B - A");
  check(r.warnings.empty(), "balanced, same settings: no warning");
  check(!r.rows[1].available && r.rows[1].reason == "needs frequencies" &&
        !r.rows[3].available && anyHas(r.notes, "butadiene"),
        "no frequencies: the rows say so");
  const std::string table = sumTable(r, false);
  check(has(table, "-234.400000") && has(table, "-262.5 ") &&
        has(table, "needs frequencies") && has(table, "Solvent: none / none"),
        "table: 6 decimals, 1 decimal kJ/mol, solvent line");
  check(has(sumTable(r, true), "-262.5499639"), "Copy: full precision");

  // coefficients and signs
  Side a2 = {term("H2", -1.17, "H2", 2), term("O2", -150.3, "O2", 1)};
  Side b2 = {term("H2O", -76.4, "H2O", 2)};
  r = sumEnergies(a2, b2);
  check(near(r.rows[0].a, 2 * -1.17 - 150.3) && near(r.rows[0].b, 2 * -76.4),
        "coefficients multiply");
  check(r.warnings.empty(), "2 H2 + O2 -> 2 H2O is balanced");
  Side a3 = {term("H2", -1.17, "H2", 2), term("H2O2", -151.5, "H2O2", -1)};
  check(near(sumEnergies(a3, b2).rows[0].a, -2.34 + 151.5), "a '-' entry subtracts");
  // unbalanced
  Side b4 = {term("H2O", -76.4, "H2O", 1)};
  r = sumEnergies(a2, b4);
  check(anyHas(r.warnings, "atoms on the two sides differ") &&
        anyHas(r.warnings, "A H4O2, B H2O"), "unbalanced reaction is warned about");

  // frequencies on every calculation
  for (EnergyTerm& t : a) freq(t, 0.1, -155.0, -155.1);
  freq(b[0], 0.15, -233.0, -233.2);
  r = sumEnergies(a, b);
  check(r.rows[1].available && r.rows[2].label == "H (298 K)" &&
        r.rows[3].label == "G (298 K)", "thermo rows with the temperature");
  check(near(r.rows[1].a, -155.8 - 78.4) && near(r.rows[2].b, -233.0) &&
        near(r.rows[3].delta(), -233.2 - (-155.1 + -155.1)),
        "E+ZPE, H and G sums");
  check(r.notes.empty(), "no note when all have frequencies");
  freq(b[0], 0.15, -233.0, -233.2, 350);
  r = sumEnergies(a, b);
  check(anyHas(r.warnings, "temperatures differ") && r.rows[2].label == "H (T)",
        "different temperatures warn");

  // different settings
  Side x = {term("a", -1)}, y = {term("b", -2)};
  y[0].state.method = "HF";
  y[0].state.basis = "def2-SVP";
  y[0].code = "NWChem";
  y[0].dispersion = "D3BJ";
  r = sumEnergies(x, y);
  check(r.ok && r.warnings.size() == 4 && has(r.warnings[0], "code differs") &&
        has(r.warnings[1], "method differs") && has(r.warnings[2], "basis set differs") &&
        has(r.warnings[3], "dispersion correction differs: A none, B D3BJ"),
        "code, method, basis and dispersion each warn");

  // gas phase against solvent of one molecule: solvation energy
  Side gas = {term("water gas", -76.40, "H2O")};
  Side sol = {term("water cpcm", -76.41, "H2O")};
  sol[0].state.solvent = "CPCM";
  r = sumEnergies(gas, sol);
  check(has(r.deltaLabel, "the solvation energy") && r.warnings.empty() &&
        r.solventA == "none" && r.solventB == "CPCM",
        "gas against solvent: solvation energy, solvents listed");
  check(has(sumTable(r, false), "B - A is the solvation energy"), "label in the table");
  sol[0].state.method = "HF";
  r = sumEnergies(gas, sol);
  check(r.deltaLabel.empty() && anyHas(r.warnings, "solvent model differs"),
        "another method: no label, a solvent warning");
  Side sol2 = {term("x", -1, "H2O")};
  sol2[0].state.solvent = "SMD";
  sol[0].state.method = "B3LYP";
  sol[0].state.solvent = "CPCM";
  r = sumEnergies(sol, sol2);
  check(r.deltaLabel.empty() && anyHas(r.warnings, "solvent model differs: A CPCM, B SMD"),
        "two solvent models: a warning, no label");

  // problems
  Side bad = {term("gone", 0)};
  bad[0].state.problem = "it has no final energy.";
  r = sumEnergies(a, bad);
  check(!r.ok && r.rows.empty() && anyHas(r.notes, "gone: it has no final energy"),
        "a calculation without energy: a note, no rows");

  // units, side syntax, names
  FinalEnergy f;
  f.found = true;
  f.units = "kcal/Mole";
  f.value = -4684.0;
  double h = 0;
  check(finalEnergyHartree(f, &h) && near(h, -4684.0 / (2625.4996394799 / 4.184)),
        "kcal/mol energies are converted to hartree");
  f.units = "furlongs";
  check(!finalEnergyHartree(f, &h), "an unknown unit is refused");
  std::vector<std::pair<std::string, int> > p = parseSide("file:///a,-2*file:///b,3*file:///c");
  check(p.size() == 3 && p[0].second == 1 && p[1].second == -2 &&
        p[1].first == "file:///b" && p[2].second == 3, "side syntax");

  check(fmtAbsolute(-76.0107465054306) == "-76.010747", "absolute: 6 decimals");
  check(fmtDifference(0.33176911, EU_KJ) == "871.1" &&
        fmtDifference(0.33176911, EU_KCAL) == "208.2" &&
        fmtDifference(0.33176911, EU_EV) == "9.03",
        "differences: 1 decimal kJ/mol and kcal/mol, 2 in eV");
  check(fmtDifference(-1e-9, EU_KJ) == "0.0", "a rounded zero has no minus sign");

  check(calcNameLess("C2", "C10") && !calcNameLess("C10", "C2"),
        "numbers in names compare as numbers");
  check(calcNameLess("alpha", "Beta") && calcNameLess("Beta", "gamma"), "case is ignored");
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

  printf("%d failure(s)\n", failures);
  return failures ? 1 : 0;
}
