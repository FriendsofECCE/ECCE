// Recipe parsing, the rules and the key data (#710).  The key data of the SN2
// example is checked against numbers worked out by hand from its Gaussian
// outputs ("SCF Done" and "Sum of electronic and thermal Free Energies"):
//   reactant E -3071.92730726  G -3071.918268
//   product  E -3071.93096916  G -3071.922962
//   QST3 (TS) E -3071.91691215 G -3071.908499
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "CompareEnergies.H"
#include "KeyData.H"
#include "Recipe.H"

static int failures = 0;
static void check(bool ok, const char *what)
{
  printf("%s: %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) failures++;
}
static bool near(double a, double b, double tol = 1e-6) { return std::fabs(a - b) < tol; }
static bool has(const std::string& s, const std::string& t) { return s.find(t) != std::string::npos; }

static const double KJ = 2625.4996394799;

static KeyStep step(const char *name, const char *role, double e, double g, bool thermo,
                    const char *method = "B3LYP", const char *code = "G16")
{
  KeyStep k;
  k.name = name; k.keyRole = role; k.thermo = thermo; k.complete = true;
  k.term.code = code;
  k.term.state.energy = e;
  k.term.state.method = method;
  k.term.state.basis = "6-31+G(d)";
  if (g != 0) { k.term.state.thermo = true; k.term.state.gibbs = g; }
  return k;
}

int main(int argc, char **argv)
{
  const std::string root = argc > 1 ? argv[1] : ".";

  std::vector<std::string> problems;
  std::vector<Recipe> all = loadRecipes({root + "/data/client/recipes"}, &problems);
  check(problems.empty() && all.size() == 2, "both built-in recipes load");
  const Recipe *ts = 0, *conf = 0;
  for (const Recipe& r : all) { if (r.id == "ts-irc") ts = &r; if (r.id == "conformers") conf = &r; }
  check(ts && ts->variants.size() == 2 && ts->variants[0].steps.size() == 5 &&
        ts->variants[1].steps.size() == 4, "TS search: G16 five steps, ORCA four");
  check(ts && ts->variants[0].links.size() == 6, "TS search: six links in the G16 variant");
  check(conf && conf->variants[0].steps[0].code == "CREST" &&
        conf->variants[0].steps[0].needsStructure, "Conformers start with a CREST box that needs a structure");

  Recipe again;
  std::string err;
  check(ts && parseRecipe(recipeText(*ts), again, err) &&
        recipeText(again) == recipeText(*ts), "recipeText reads back as the same recipe");
  Recipe bad;
  check(!parseRecipe("recipe: x\nvariant: a | A | G16\nlink: a | p | q | geometry\n", bad, err) &&
        has(err, "unknown steps"), "a link between unknown steps is refused");
  check(!parseRecipe("recipe: x\nbogus: 1\n", bad, err) && has(err, "unknown key"),
        "an unknown key is refused");

  check(ts && conf && ts->roles.back().id == "solvent" &&
        conf->roles.back().id == "solvent" && ts->roles.back().basis == "none",
        "legacy solvent role and missing role become the shared gas-phase row");
  Recipe legacy;
  check(parseRecipe("recipe: old\nrole: solvent | Solvent | PCM | DMSO | solution\n"
                    "variant: a | A | G16\nstep: a | opt | G16 | | Geometry\n", legacy, err)
        && legacy.roles.back().basis == "DMSO" && legacy.roles.back().method.empty(),
        "old solvent model/basis fields map to the solvent selector");
  check(recipeSolventSettings("G16", "DMSO")[2].second == "Dimethylsulfoxide" &&
        recipeSolventSettings("xtb", "chloroform")[1].second == "chcl3" &&
        recipeSolventSettings("ORCA", "chloroform")[2].second == "chloroform" &&
        recipeSolventSettings("ECCE-QM", "water")[0].second == "Water",
        "solvent aliases match each code's existing editor");
  check(recipeSolventSettings("G16", "none")[0].second == "0" &&
        recipeSolventSettings("ORCA", "none")[0].second == "0" &&
        recipeSolventSettings("CREST", "none")[0].second == "None" &&
        recipeSolventSettings("ECCE-QM", "none")[0].second == "None",
        "none explicitly switches off each solvent model");
  const auto names = recipeSolvents(conf->variants[0]);
  check(std::find(names.begin(), names.end(), "ethanol") == names.end() &&
        std::find(names.begin(), names.end(), "water") != names.end(),
        "Conformers offers only solvents accepted by all three code editors");
  check(recipeSolventModel("NWChem").empty() &&
        recipeSolventSettings("NWChem", "water").empty(),
        "an unmapped code has no invented solvent settings");

  // rules
  const RecipeRole role = ts->roles[0];
  StructureFacts anion; anion.known = true; anion.charge = -1; anion.atoms = 5; anion.elements = {6, 17, 35};
  RoleChoice c = recommend(*ts, role, anion);
  check(c.basis == "6-31+G(d)", "anion: the recipe's diffuse default is kept");
  RecipeRole plain = role; plain.basis = "6-31G(d)";
  c = recommend(*ts, plain, anion);
  check(c.basis == "6-31+G(d)" && has(c.why, "anion"), "anion: diffuse functions added with the reason");
  StructureFacts heavy; heavy.known = true; heavy.atoms = 4; heavy.elements = {6, 53};
  c = recommend(*ts, plain, heavy);
  check(c.basis == "def2-SVP" && has(c.why, "beyond Kr"), "iodine: a def2 basis for the ECP");
  StructureFacts big; big.known = true; big.atoms = 120; big.elements = {1, 6};
  check(!recommend(*ts, role, big).warnings.empty() &&
        has(recommend(*ts, role, big).warnings[0], "RI"), "size: RI or a smaller basis");
  check(checkLevel(*ts, "B3LYP", "6-31G(d)", anion).size() == 1 &&
        checkLevel(*ts, "B3LYP", "6-31+G(d)", anion).empty(), "checkLevel warns, never blocks");
  check(recommend(*ts, plain, StructureFacts()).basis == "6-31G(d)", "no structure: the default");
  check(!codeAvailable("NWChem", {"G16", "ORCA"}) && codeAvailable("G16", {"G16"}) &&
        codeAvailable("anything", {}), "unregistered codes are not available");

  // key data on constructed values
  std::vector<KeyStep> s = {step("react", "reactant", -10.0, -10.1, true),
                            step("prod", "product", -10.2, -10.3, true),
                            step("ts", "ts", -9.9, -10.0, true)};
  KeyDataResult k = keyData(s);
  check(k.shown && k.rows.size() == 7, "three steps and four differences");
  check(has(k.text(), "Reaction energy") && has(k.text(), "-525.1 kJ/mol") &&
        has(k.text(), "Barrier") && has(k.text(), "+262.5 kJ/mol") &&
        has(k.text(), "Free-energy barrier") && has(k.text(), "3 of 3 steps"),
        "reaction energy -0.2 Eh, barrier +0.1 Eh (x2625.5 kJ/mol)");
  s[2].complete = false;
  k = keyData(s);
  check(k.shown && has(k.text(), "ts: waiting") && !has(k.text(), "Barrier") &&
        has(k.text(), "Reaction energy") && has(k.text(), "2 of 3 steps"),
        "a step still running shows as waiting; the barrier waits for it");
  s[2] = step("ts", "ts", -9.9, 0, true, "PBE0");
  k = keyData(s);
  check(!k.shown && k.rows.empty() && has(k.note, "not compared"),
        "two levels of theory: no numbers");
  s[2] = step("ts", "ts", -9.9, 0, true);
  k = keyData(s);
  check(has(k.text(), "Barrier") && !has(k.text(), "Free-energy barrier"),
        "no G for a step: no free-energy barrier");
  s[2] = step("ts", "ts", -9.9, -10.0, false);
  check(!has(keyData(s).text(), "Free-energy barrier"), "G only where the recipe asks for it");

  // the SN2 example's stored results
  const std::string ex = "file://" + root + "/data/client/examples/workflows/sn2-cl-ch3br/";
  std::vector<KeyStep> sn;
  const char *dirs[3][3] = {{"g16-reactant", "sn2_reactant", "reactant"},
                            {"g16-product", "sn2_product", "product"},
                            {"g16-qst3", "sn2_qst3", "ts"}};
  for (int i = 0; i < 3; i++) {
    KeyStep ks;
    ks.name = dirs[i][2]; ks.keyRole = dirs[i][2]; ks.thermo = true; ks.complete = true;
    ks.term = gatherEnergyTerm(ex + dirs[i][0] + "/stored/" + dirs[i][1], 1);
    sn.push_back(ks);
  }
  check(sn[0].term.state.problem.empty() && near(sn[0].term.state.energy, -3071.92730726, 1e-7) &&
        near(sn[1].term.state.energy, -3071.93096916, 1e-7) &&
        near(sn[2].term.state.energy, -3071.91691215, 1e-7), "stored energies read back");
  check(near(sn[0].term.state.gibbs, -3071.918268, 1e-6) &&
        near(sn[1].term.state.gibbs, -3071.922962, 1e-6) &&
        near(sn[2].term.state.gibbs, -3071.908499, 1e-6), "stored free energies read back");
  k = keyData(sn);
  printf("level: %s\n", k.level.c_str());
  check(k.shown && has(k.level, "B3LYP") && has(k.level, "6-31+G") && has(k.level, "Gaussian"), "SN2: one code and level, shown");
  char want[4][80];
  snprintf(want[0], 80, "%+.1f kJ/mol", (-3071.93096916 + 3071.92730726) * KJ);   // -9.6
  snprintf(want[1], 80, "%+.1f kJ/mol", (-3071.91691215 + 3071.92730726) * KJ);   // +27.3
  snprintf(want[2], 80, "%+.1f kJ/mol", (-3071.922962 + 3071.918268) * KJ);       // -12.3
  snprintf(want[3], 80, "%+.1f kJ/mol", (-3071.908499 + 3071.918268) * KJ);       // +25.6
  const std::string t = k.text();
  check(has(t, "Reaction energy  E(product) - E(reactant): -9.6 kJ/mol") && !std::string(want[0]).empty() &&
        std::string(want[0]) == "-9.6 kJ/mol", "SN2 reaction energy -9.6 kJ/mol by hand");
  check(has(t, "Barrier  E(TS) - E(reactant): +27.3 kJ/mol") && std::string(want[1]) == "+27.3 kJ/mol",
        "SN2 barrier +27.3 kJ/mol by hand");
  check(has(t, "Reaction free energy  G(product) - G(reactant): -12.3 kJ/mol") &&
        std::string(want[2]) == "-12.3 kJ/mol", "SN2 reaction free energy -12.3 kJ/mol by hand");
  check(has(t, "Free-energy barrier  G(TS) - G(reactant): +25.6 kJ/mol") &&
        std::string(want[3]) == "+25.6 kJ/mol", "SN2 free-energy barrier +25.6 kJ/mol by hand");

  printf(failures ? "FAILED\n" : "PASSED\n");
  return failures ? 1 : 0;
}
