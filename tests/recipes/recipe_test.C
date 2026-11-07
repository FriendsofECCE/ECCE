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
#include "tdat/Fragment.H"
#include <sstream>

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
  check(problems.empty() && all.size() == 7, "all seven built-in recipes load");
  const Recipe *ts = 0, *conf = 0;
  for (const Recipe& r : all) { if (r.id == "ts-irc") ts = &r; if (r.id == "conformers") conf = &r; }
  check(ts && ts->variants.size() == 2 && ts->variants[0].steps.size() == 5 &&
        ts->variants[1].steps.size() == 4, "TS search: G16 five steps, ORCA four");
  check(ts && ts->variants[0].links.size() == 6, "TS search: six links in the G16 variant");
  check(conf && conf->variants[0].steps[0].code == "xtb" &&
        conf->variants[0].steps[0].needsStructure, "Conformers start with an xTB optimisation that needs a structure");

  // the CREST recipes (#808)
  auto find = [&](const char *id) -> const Recipe * {
    for (const Recipe& r : all) if (r.id == id) return &r;
    return (const Recipe *)0;
  };
  auto stepIn = [](const RecipeVariant& v, const char *id) -> const RecipeStep * {
    for (const RecipeStep& s : v.steps) if (s.id == id) return &s;
    return (const RecipeStep *)0;
  };
  auto linkIn = [](const RecipeVariant& v, const char *from, const char *to) -> std::string {
    for (const RecipeLink& l : v.links) if (l.from == from && l.to == to) return l.name;
    return "";
  };
  struct Want { const char *id, *runType, *link; int shift; };
  const Want crest[] = {{"conformers", "Conformer Search", "geometry", 0},
                        {"protonation-sites", "Protonation Sites", "geometry.c1", 1},
                        {"tautomers", "Tautomers", "geometry.c1", 0},
                        {"solvation-shell", "Solvation Shell", "geometry", 0}};
  for (const Want& w : crest) {
    const Recipe *r = find(w.id);
    bool ok = r && r->variants.size() == 2 && r->field == "Conformations";
    for (size_t i = 0; ok && i < 2; i++) {
      const RecipeVariant& v = r->variants[i];
      const char *dft = v.code == "ORCA" ? "ORCA" : "G16";
      const RecipeStep *p = stepIn(v, "preopt"), *c = stepIn(v, "crest"),
                       *x = stepIn(v, "xtb"), *d = stepIn(v, "dft");
      ok = v.steps.size() == 4 && v.links.size() == 3 && p && c && x && d &&
           p->code == "xtb" && p->runType == "Geometry" && p->needsStructure &&
           c->code == "CREST" && c->runType == w.runType && !c->needsStructure &&
           c->chargeShift == 0 && x->code == "xtb" && d->code == dft &&
           x->chargeShift == w.shift && d->chargeShift == 0 && d->thermo && d->keyRole == "final" &&
           linkIn(v, "preopt", "crest") == "geometry" &&
           linkIn(v, "crest", "xtb") == w.link && linkIn(v, "xtb", "dft") == "geometry";
    }
    check(ok, (std::string(w.id) + ": xTB Opt -> CREST -> xTB -> DFT, both codes, links, charge shift").c_str());
  }
  const Recipe *prot = find("protonation-sites"), *sol = find("solvation-shell"),
               *xc = find("xtb-crest");
  check(prot && !prot->rules.count("anion-diffuse") && prot->rules.count("size"),
        "protonation: no anion-diffuse rule for the cation");
  const RecipeStep *shell = sol ? stepIn(sol->variants[0], "crest") : 0;
  bool water = false, ten = false;
  for (size_t i = 0; shell && i < shell->settings.size(); i++) {
    if (shell->settings[i].first == "ES.Runtype.QCG.Solvent" && shell->settings[i].second == "water") water = true;
    if (shell->settings[i].first == "ES.Runtype.QCG.Number" && shell->settings[i].second == "10") ten = true;
  }
  check(water && ten && sol->roles.back().id == "solvent" && sol->roles.back().basis == "water",
        "solvation shell: water, ten molecules, Solvent row starts at water");
  bool ok = xc && xc->variants.size() == 2;
  for (size_t i = 0; ok && i < 2; i++) {
    const RecipeVariant& v = xc->variants[i];
    const RecipeStep *x = stepIn(v, "xtb"), *c = stepIn(v, "crest"), *d = stepIn(v, "dft");
    ok = v.steps.size() == 3 && x && c && d && x->needsStructure && x->runType == "Geometry" &&
         c->runType == "Conformer Search" && !c->needsStructure &&
         linkIn(v, "xtb", "crest") == "geometry" && linkIn(v, "crest", "dft") == "geometry.c1";
  }
  check(ok, "xtb-crest: xTB Opt -> CREST on its geometry -> DFT, both codes");
  Recipe shifted;
  std::string serr;
  check(parseRecipe("recipe: s\nvariant: a | A | G16\nstep: a | p | G16 | | Geometry | p | charge=-2, thermo\n", shifted, serr) &&
        shifted.variants[0].steps[0].chargeShift == -2 && shifted.variants[0].steps[0].thermo &&
        has(recipeText(shifted), "charge=-2") && !parseRecipe("recipe: s\nvariant: a | A | G16\nstep: a | p | G16 | | Geometry | p | charge=x\n", shifted, serr),
        "charge= flag parses, is written back, and a bad number is refused");

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

  // Fragments (#201): the recipe's fragment step, the split, the checks.
  {
    Recipe fr;
    std::string err;
    check(parseRecipe("recipe: f\nvariant: v | V | ORCA\n"
                      "step: v | cx | ORCA | | Energy | complex | structure\n"
                      "step: v | frag | ORCA | | Energy | fragment | fragment\n"
                      "link: v | cx | frag | geometry\n", fr, err) &&
          fr.variants[0].steps[1].perFragment, "the fragment flag is read");
    check(has(recipeText(fr), "| fragment\n"), "and written back");
    std::vector<RecipeFragment> parts;
    check(parseFragments("1:0:1/2-13:-1:2", parts) && parts.size() == 2 &&
          parts[1].atoms == "2-13" && parts[1].charge == -1 && parts[1].mult == 2 &&
          fragmentsText(parts) == "1:0:1/2-13:-1:2", "fragment list read and written");
    RecipeVariant ex = expandFragments(fr.variants[0], parts);
    check(ex.steps.size() == 3 && ex.steps[1].id == "frag-1" && ex.steps[2].id == "frag-2" &&
          ex.steps[2].fragment == 2 && ex.steps[2].fragments == "1:0:1/2-13:-1:2" &&
          ex.links.size() == 2 && ex.links[1].from == "cx" && ex.links[1].to == "frag-2",
          "one step and one link per fragment");
    std::vector<std::vector<int> > atoms;
    check(resolveFragments({{"1", 0, 1}, {"rest", 0, 1}}, 13, atoms, err) &&
          atoms[1].size() == 12 && atoms[1][0] == 1, "rest takes the other atoms");
    check(!resolveFragments({{"1-3", 0, 1}, {"3-13", 0, 1}}, 13, atoms, err) &&
          has(err, "atom 3 is in fragment 1 and in fragment 2"), "shared atom refused");
    check(!resolveFragments({{"1", 0, 1}, {"2-12", 0, 1}}, 13, atoms, err) &&
          has(err, "atoms 13 are in no fragment"), "missing atom refused");
    check(!resolveFragments({{"1", 0, 1}, {"2-14", 0, 1}}, 13, atoms, err) &&
          has(err, "atom 14"), "atom beyond the structure refused");
    // CoCl(NH3) +1: Co | ligands, Cl (odd) an anion, Co takes +2; Cr(CO)6 neutral.
    std::vector<int> z = {27, 17, 7, 1, 1, 1};
    std::vector<double> xyz = {0, 0, 0,  2.2, 0, 0,  -2.0, 0, 0,  -2.4, 0.9, 0,
                               -2.4, -0.45, 0.8,  -2.4, -0.45, -0.8};
    std::vector<double> radii = {1.26, 1.02, 0.71, 0.31, 0.31, 0.31};
    std::vector<RecipeFragment> p = proposeFragments(z, xyz, radii, 1);
    check(fragmentsText(p) == "1:2:2/2-6:-1:1", ("metal | ligands, odd ligand an anion: " + fragmentsText(p)).c_str());
    check(checkFragments(p, z, 1).empty(), "the proposal passes the check");
    check(has(checkFragments({{"1", 2, 1}, {"2-6", -1, 1}}, z, 1), "cannot have multiplicity 1"),
          "a multiplicity the electrons do not allow is refused");
    std::string warn;
    check(checkFragments({{"1", 0, 2}, {"2-6", 0, 2}}, z, 1, &warn).empty() &&
          has(warn, "add up to 0, the complex has 1"), "charges that do not add up only warn");
    // Two water molecules without a metal: one fragment per molecule.
    std::vector<int> zw = {8, 1, 1, 8, 1, 1};
    std::vector<double> xw = {0, 0, 0,  0.96, 0, 0,  -0.24, 0.93, 0,
                              2.9, 0, 0,  3.2, 0.9, 0,  3.2, -0.9, 0};
    check(fragmentsText(proposeFragments(zw, xw, {0.66, 0.31, 0.31, 0.66, 0.31, 0.31}, 0)) ==
          "1-3:0:1/4-6:0:1", "no metal: one fragment per molecule");
  }

  // Fragment state (#201): in the complex (lowest multiplicity) or free (ground state).
  {
    std::vector<RecipeFragment> parts;
    check(parseFragments("1:0:7:free/2-13:0:1", parts) && parts.size() == 2 && parts[0].free &&
          !parts[1].free && fragmentsText(parts) == "1:0:7:free/2-13:0:1", "the free state reads and writes");
    check(!parseFragments("1:0:7:loose/2-13:0:1", parts), "an unknown state is refused");
    struct { int z, q; bool free; int mult; const char *what; } cases[] = {
      {24, 0, false, 1, "Cr(0) in the complex: singlet"}, {24, 0, true, 7, "free Cr: septet (7S)"},
      {26, 2, true, 5, "free Fe2+: quintet (d6)"}, {27, 3, true, 5, "free Co3+: quintet (d6)"},
      {27, 3, false, 1, "Co3+ in the complex: singlet"}, {29, 1, true, 1, "free Cu+: singlet"},
      {26, 3, false, 2, "Fe3+ in the complex: doublet"}, {26, 3, true, 6, "free Fe3+: sextet (d5)"},
      {42, 0, true, 7, "free Mo: septet"}, {77, 1, true, 5, "free Ir+: quintet"},
      {8, 0, true, 3, "free O: triplet"}, {17, -1, true, 1, "free Cl-: singlet"}};
    for (const auto& c : cases)
      check(fragmentMultiplicity(std::vector<int>(1, c.z), c.q, c.free) == c.mult, c.what);
    check(fragmentMultiplicity({6, 8}, 0, true) == 1, "a free molecule keeps the lowest multiplicity");
  }

  // Fragment names (#201): n copies of one molecule as (formula)n.
  {
    auto named = [](const std::string& xyz) {
      std::istringstream in(xyz);
      Fragment f;
      f.restoreXYZ(in);
      return groupedFormula(f);
    };
    std::string co6 = "12\nCO6\n";
    const char *ax[6] = {"1 0 0", "-1 0 0", "0 1 0", "0 -1 0", "0 0 1", "0 0 -1"};
    for (const char *a : ax) {
      double x, y, zz;
      sscanf(a, "%lf %lf %lf", &x, &y, &zz);
      char l[160];
      snprintf(l, sizeof l, "C %.3f %.3f %.3f\nO %.3f %.3f %.3f\n", 1.92 * x, 1.92 * y, 1.92 * zz,
               3.07 * x, 3.07 * y, 3.07 * zz);
      co6 += l;
    }
    std::string got = named(co6);
    check(got == "(CO)6", ("six CO molecules are (CO)6: " + got).c_str());
    got = named("5\nmix\nC 0 0 0\nO 1.13 0 0\nN 5 0 0\nH 5.4 0.9 0\nH 5.4 -0.45 0.8\n");
    check(got.find('(') == std::string::npos, ("different molecules keep the plain formula: " + got).c_str());
    got = named("3\nCl3\nCl 0 0 0\nCl 5 0 0\nCl 0 5 0\n");
    check(got == "Cl3", ("separate atoms keep the plain formula: " + got).c_str());
  }

  printf(failures ? "FAILED\n" : "PASSED\n");
  return failures ? 1 : 0;
}
