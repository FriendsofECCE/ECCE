#include "Recipe.H"

#include <algorithm>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <map>
#include <sstream>

#include "tdat/Fragment.H"
#include "tdat/TAtm.H"
#include "tdat/TFormula.H"
#include "tdat/TTCPair.H"
#include "util/Ecce.H"

using std::string;
using std::vector;

namespace {

string trim(const string& s)
{
  size_t a = s.find_first_not_of(" \t\r");
  if (a == string::npos) return "";
  return s.substr(a, s.find_last_not_of(" \t\r") - a + 1);
}

vector<string> fields(const string& s)
{
  vector<string> out;
  size_t at = 0;
  for (;;) {
    size_t bar = s.find('|', at);
    out.push_back(trim(s.substr(at, bar == string::npos ? bar : bar - at)));
    if (bar == string::npos) break;
    at = bar + 1;
  }
  return out;
}

RecipeVariant *variantOf(Recipe& r, const string& id)
{
  for (size_t i = 0; i < r.variants.size(); i++)
    if (r.variants[i].id == id) return &r.variants[i];
  return 0;
}

RecipeStep *stepOf(RecipeVariant& v, const string& id)
{
  for (size_t i = 0; i < v.steps.size(); i++)
    if (v.steps[i].id == id) return &v.steps[i];
  return 0;
}

}  // namespace


bool parseRecipe(const string& text, Recipe& out, string& error)
{
  out = Recipe();
  std::istringstream in(text);
  string line;
  int n = 0;
  auto bad = [&](const string& why) {
    error = "line " + std::to_string(n) + ": " + why;
    return false;
  };
  while (std::getline(in, line)) {
    n++;
    line = trim(line);
    if (line.empty() || line[0] == '#') continue;
    const size_t colon = line.find(':');
    if (colon == string::npos) return bad("expected \"key: value\"");
    const string key = trim(line.substr(0, colon));
    const string value = trim(line.substr(colon + 1));
    vector<string> f = fields(value);
    if (key == "recipe") out.id = value;
    else if (key == "title") out.title = value;
    else if (key == "field") out.field = value;
    else if (key == "summary") out.summary = value;
    else if (key == "description") out.description = value;
    else if (key == "role") {
      if (f.size() < 5) return bad("role needs id | label | method | basis | why");
      out.roles.push_back({f[0], f[1], f[2], f[3], f[4]});
    } else if (key == "variant") {
      if (f.size() < 3) return bad("variant needs id | label | code");
      RecipeVariant v;
      v.id = f[0]; v.label = f[1]; v.code = f[2];
      out.variants.push_back(v);
    } else if (key == "step") {
      if (f.size() < 5) return bad("step needs variant | id | code | role | run type");
      RecipeVariant *v = variantOf(out, f[0]);
      if (!v) return bad("step of an unknown variant " + f[0]);
      if (stepOf(*v, f[1])) return bad("step " + f[1] + " twice");
      RecipeStep s;
      s.id = f[1]; s.code = f[2]; s.role = f[3]; s.runType = f[4];
      if (f.size() > 5) s.caption = f[5];
      if (f.size() > 6) {
        std::istringstream fl(f[6]);
        string flag;
        while (std::getline(fl, flag, ',')) {
          flag = trim(flag);
          if (flag == "structure") s.needsStructure = true;
          else if (flag == "thermo") s.thermo = true;
          else if (flag == "fragment") s.perFragment = true;
          else if (flag == "field") s.metalField = true;
          else if (flag.compare(0, 4, "key=") == 0) s.keyRole = flag.substr(4);
          else if (flag.compare(0, 7, "charge=") == 0) {
            char *end = 0;
            const long n = strtol(flag.c_str() + 7, &end, 10);
            if (flag.size() == 7 || *end) return bad("charge= needs a whole number, like charge=+1");
            s.chargeShift = (int)n;
          }
          else if (!flag.empty()) return bad("unknown flag " + flag);
        }
      }
      v->steps.push_back(s);
    } else if (key == "set") {
      if (f.size() < 4) return bad("set needs variant | step | key | value");
      RecipeVariant *v = variantOf(out, f[0]);
      RecipeStep *s = v ? stepOf(*v, f[1]) : 0;
      if (!s) return bad("set on an unknown step " + f[0] + "/" + f[1]);
      s->settings.push_back(std::make_pair(f[2], f[3]));
    } else if (key == "link") {
      if (f.size() < 4) return bad("link needs variant | from | to | name");
      RecipeVariant *v = variantOf(out, f[0]);
      if (!v) return bad("link of an unknown variant " + f[0]);
      if (!stepOf(*v, f[1]) || !stepOf(*v, f[2]))
        return bad("link between unknown steps " + f[1] + " and " + f[2]);
      v->links.push_back({f[1], f[2], f[3]});
    } else if (key == "rules") {
      for (size_t i = 0; i < f.size(); i++) {
        std::istringstream ls(f[i]);
        string r;
        while (std::getline(ls, r, ',')) if (!trim(r).empty()) out.rules.insert(trim(r));
      }
    } else {
      return bad("unknown key " + key);
    }
  }
  n = 0;
  if (out.id.empty()) return bad("no recipe: id");
  if (out.title.empty()) out.title = out.id;
  if (out.variants.empty()) return bad("no variant");
  for (size_t i = 0; i < out.variants.size(); i++)
    if (out.variants[i].steps.empty()) return bad("variant " + out.variants[i].id + " has no step");
  // Solvent is a workflow-wide choice, including in recipes without a role.
  auto solvent = std::find_if(out.roles.begin(), out.roles.end(),
      [](const RecipeRole& r) { return r.id == "solvent"; });
  if (solvent == out.roles.end())
    out.roles.push_back({"solvent", "Solvent", "", "none", "gas phase by default"});
  else {
    if (solvent->basis.empty())
      solvent->basis = solvent->method.empty() || solvent->method == "none" ? "none" : "water";
    solvent->method.clear();
  }
  return true;
}


string recipeText(const Recipe& r)
{
  std::ostringstream o;
  o << "recipe: " << r.id << "\ntitle: " << r.title << "\n";
  if (!r.field.empty()) o << "field: " << r.field << "\n";
  if (!r.summary.empty()) o << "summary: " << r.summary << "\n";
  if (!r.description.empty()) o << "description: " << r.description << "\n";
  for (const RecipeRole& x : r.roles)
    o << "role: " << x.id << " | " << x.label << " | " << x.method << " | "
      << x.basis << " | " << x.why << "\n";
  for (const RecipeVariant& v : r.variants) {
    o << "variant: " << v.id << " | " << v.label << " | " << v.code << "\n";
    for (const RecipeStep& s : v.steps) {
      string flags;
      auto add = [&](const string& f) { flags += (flags.empty() ? "" : ",") + f; };
      if (s.needsStructure) add("structure");
      if (s.thermo) add("thermo");
      if (s.perFragment) add("fragment");
      if (s.metalField) add("field");
      if (!s.keyRole.empty()) add("key=" + s.keyRole);
      if (s.chargeShift) add(string("charge=") + (s.chargeShift > 0 ? "+" : "") + std::to_string(s.chargeShift));
      o << "step: " << v.id << " | " << s.id << " | " << s.code << " | " << s.role
        << " | " << s.runType << " | " << s.caption << " | " << flags << "\n";
      for (const auto& kv : s.settings)
        o << "set: " << v.id << " | " << s.id << " | " << kv.first << " | "
          << kv.second << "\n";
    }
    for (const RecipeLink& l : v.links)
      o << "link: " << v.id << " | " << l.from << " | " << l.to << " | " << l.name << "\n";
  }
  if (!r.rules.empty()) {
    o << "rules: ";
    bool first = true;
    for (const string& x : r.rules) { o << (first ? "" : ", ") << x; first = false; }
    o << "\n";
  }
  return o.str();
}


vector<Recipe> loadRecipes(const vector<string>& dirs, vector<string>* problems)
{
  vector<Recipe> out;
  for (size_t d = 0; d < dirs.size(); d++) {
    DIR *dir = opendir(dirs[d].c_str());
    if (!dir) continue;
    vector<string> names;
    while (struct dirent *e = readdir(dir)) {
      const string n = e->d_name;
      if (n.size() > 7 && n.compare(n.size() - 7, 7, ".recipe") == 0) names.push_back(n);
    }
    closedir(dir);
    std::sort(names.begin(), names.end());
    for (size_t i = 0; i < names.size(); i++) {
      std::ifstream f((dirs[d] + "/" + names[i]).c_str());
      std::ostringstream text;
      text << f.rdbuf();
      Recipe r;
      string error;
      if (!parseRecipe(text.str(), r, error)) {
        if (problems) problems->push_back(dirs[d] + "/" + names[i] + ": " + error);
        continue;
      }
      r.site = d > 0;
      bool replaced = false;
      for (size_t k = 0; k < out.size(); k++)
        if (out[k].id == r.id) { out[k] = r; replaced = true; }
      if (!replaced) out.push_back(r);
    }
  }
  return out;
}


vector<string> recipeDirs()
{
  vector<string> dirs;
  dirs.push_back(string(Ecce::ecceHome()) + "/data/client/recipes");
  const vector<Ecce::SiteLayer> layers = Ecce::siteConfigLayers();
  for (size_t i = layers.size(); i-- > 0;)     // lowest priority first
    dirs.push_back(layers[i].dir + "Recipes");
  const char *extra = getenv("ECCE_RECIPE_DIRS");
  if (extra) {
    std::istringstream ls(extra);
    string d;
    while (std::getline(ls, d, ':')) if (!d.empty()) dirs.push_back(d);
  }
  return dirs;
}


namespace {

bool hasDiffuse(const string& basis)
{
  return basis.find('+') != string::npos || basis.find("aug") != string::npos ||
         basis.find("SVPD") != string::npos || basis.find("TZVPD") != string::npos ||
         basis.find("ma-") != string::npos;
}

string diffuseOf(const string& basis)
{
  if (basis.compare(0, 4, "def2") == 0)
    return basis + "D";                         // def2-SVP -> def2-SVPD
  if (basis.compare(0, 4, "6-31") == 0 && basis.find('G') != string::npos) {
    string b = basis;
    b.insert(b.find('G'), "+");                 // 6-31G(d) -> 6-31+G(d)
    return b;
  }
  if (basis.find("cc-p") != string::npos) return "aug-" + basis;
  return basis;
}

bool beyondKr(const StructureFacts& f)
{
  for (size_t i = 0; i < f.elements.size(); i++)
    if (f.elements[i] > 36) return true;
  return false;
}

}  // namespace


RoleChoice recommend(const Recipe& recipe, const RecipeRole& role,
                     const StructureFacts& facts)
{
  RoleChoice c;
  c.method = role.method;
  c.basis = role.basis;
  c.why = role.why;
  if (!facts.known || c.basis.empty()) return c;
  const bool all = recipe.rules.empty();
  if ((all || recipe.rules.count("anion-diffuse")) && facts.charge < 0 &&
      !hasDiffuse(c.basis)) {
    c.basis = diffuseOf(c.basis);
    c.why = "anion: diffuse functions describe the extra electron";
  }
  if ((all || recipe.rules.count("heavy-ecp")) && beyondKr(facts) &&
      c.basis.compare(0, 4, "def2") != 0) {
    c.basis = "def2-SVP";
    c.why = "elements beyond Kr: def2 basis sets bring an effective core potential";
  }
  if ((all || recipe.rules.count("size")) && facts.atoms > 60)
    c.warnings.push_back(std::to_string(facts.atoms) +
        " atoms: consider the RI approximation or a smaller basis");
  return c;
}


vector<string> checkLevel(const Recipe& recipe, const string& method,
                          const string& basis, const StructureFacts& facts)
{
  (void)method;
  vector<string> w;
  if (!facts.known) return w;
  const bool all = recipe.rules.empty();
  if ((all || recipe.rules.count("anion-diffuse")) && facts.charge < 0 &&
      !basis.empty() && !hasDiffuse(basis))
    w.push_back("Charge " + std::to_string(facts.charge) + ": " + basis +
                " has no diffuse functions; " + diffuseOf(basis) + " is the usual choice");
  if ((all || recipe.rules.count("heavy-ecp")) && beyondKr(facts) &&
      basis.compare(0, 4, "def2") != 0)
    w.push_back("Elements beyond Kr: " + basis + " may lack an effective core potential");
  if ((all || recipe.rules.count("size")) && facts.atoms > 60)
    w.push_back(std::to_string(facts.atoms) +
                " atoms: consider the RI approximation or a smaller basis");
  return w;
}


// ---- fragments (#201) ----

bool hasFragmentSteps(const RecipeVariant& variant)
{
  for (const RecipeStep& s : variant.steps) if (s.perFragment) return true;
  return false;
}


namespace { bool isMetal(int z); }

RecipeVariant expandFragments(const RecipeVariant& variant,
                              const vector<RecipeFragment>& fragments,
                              const vector<int>& z)
{
  RecipeVariant out = variant;
  out.steps.clear();
  out.links.clear();
  const string spec = fragmentsText(fragments);
  std::map<string, vector<string> > ids;   // recipe step id -> made step ids
  for (const RecipeStep& s : variant.steps) {
    if (!s.perFragment) {
      out.steps.push_back(s);
      ids[s.id].push_back(s.id);
      continue;
    }
    for (size_t k = 0; k < fragments.size(); k++) {
      const RecipeFragment& one = fragments[k];
      // A single metal atom in its complex state: the d orbitals of the bare
      // ion are degenerate and the SCF fills an arbitrary subset, so a first
      // step in point charges at the donor atoms picks the occupation. A
      // non-metal has no such set, and without the elements nothing is known.
      const bool oneAtom = !one.free && !one.atoms.empty() &&
          one.atoms.find_first_not_of("0123456789") == string::npos;
      const long atom = oneAtom ? atol(one.atoms.c_str()) : 0;
      if (s.metalField && oneAtom && atom >= 1 && atom <= (long)z.size() &&
          isMetal(z[atom - 1])) {
        RecipeStep pre = s;
        pre.id = s.id + "-" + std::to_string(k + 1) + "-field";
        pre.field = true;
        pre.metalField = false;
        pre.fragment = (int)k + 1;
        pre.fragments = spec;
        pre.caption = "atoms " + one.atoms + " in weak point charges at the donor atoms, "
                      "charge " + std::to_string(one.charge) +
                      ", multiplicity " + std::to_string(one.mult);
        out.steps.push_back(pre);
        ids[s.id + "#field"].push_back(pre.id);
      }
      RecipeStep f = s;
      f.id = s.id + "-" + std::to_string(k + 1);
      f.fragment = (int)k + 1;
      f.fragments = spec;
      const RecipeFragment& g = fragments[k];
      f.caption = "atoms " + g.atoms + ", charge " + std::to_string(g.charge) +
                  ", multiplicity " + std::to_string(g.mult);
      out.steps.push_back(f);
      ids[s.id].push_back(f.id);
    }
  }
  for (const RecipeLink& l : variant.links)
    for (const string& from : ids[l.from])
      for (const string& to : ids[l.to])
        out.links.push_back({from, to, l.name});
  // The field step takes the geometry from the same source as its fragment
  // step and hands its orbitals on.
  for (const RecipeLink& l : variant.links)
    for (const string& pre : ids[l.to + "#field"]) {
      for (const string& from : ids[l.from]) out.links.push_back({from, pre, l.name});
      out.links.push_back({pre, pre.substr(0, pre.size() - 6), "orbitals"});
    }
  return out;
}


string fragmentsText(const vector<RecipeFragment>& fragments)
{
  string t;
  for (size_t k = 0; k < fragments.size(); k++)
    t += (k ? "/" : "") + fragments[k].atoms + ":" + std::to_string(fragments[k].charge) +
         ":" + std::to_string(fragments[k].mult) + (fragments[k].free ? ":free" : "");
  return t;
}


bool parseFragments(const string& text, vector<RecipeFragment>& out)
{
  out.clear();
  std::istringstream in(text);
  string one;
  while (std::getline(in, one, '/')) {
    const size_t a = one.find(':'), b = a == string::npos ? a : one.find(':', a + 1);
    if (b == string::npos) return false;
    RecipeFragment f;
    f.atoms = trim(one.substr(0, a));
    char *end = 0;
    f.charge = (int)strtol(one.c_str() + a + 1, &end, 10);
    if (*end != ':') return false;
    f.mult = (int)strtol(one.c_str() + b + 1, &end, 10);
    if (*end == ':') {
      const string st = trim(end + 1);
      if (st == "free") f.free = true;
      else if (st != "complex") return false;
    } else if (*end) return false;
    if (f.mult < 1) return false;
    out.push_back(f);
  }
  return !out.empty();
}


namespace {

// "2-13,15" -> 1-based numbers; false when it does not read.
bool atomList(const string& text, vector<int>& out)
{
  out.clear();
  std::istringstream in(text);
  string part;
  while (std::getline(in, part, ',')) {
    part = trim(part);
    if (part.empty()) return false;
    char *end = 0;
    const long a = strtol(part.c_str(), &end, 10);
    long b = a;
    if (*end == '-') b = strtol(end + 1, &end, 10);
    if (*end || a < 1 || b < a) return false;
    for (long x = a; x <= b; x++) out.push_back((int)x);
  }
  return !out.empty();
}

bool isMetal(int z)
{
  return (z >= 21 && z <= 30) || (z >= 39 && z <= 48) || (z >= 57 && z <= 80) ||
         (z >= 89 && z <= 112) || z == 3 || z == 4 || z == 11 || z == 12 || z == 13 ||
         z == 19 || z == 20 || z == 31 || z == 37 || z == 38 || z == 49 || z == 50 ||
         z == 55 || z == 56 || (z >= 81 && z <= 83);
}

// "1,2,3,7" -> "1-3,7"
string listText(vector<int> a)
{
  std::sort(a.begin(), a.end());
  string t;
  for (size_t i = 0; i < a.size();) {
    size_t j = i;
    while (j + 1 < a.size() && a[j + 1] == a[j] + 1) j++;
    t += (t.empty() ? "" : ",") + std::to_string(a[i] + 1);
    if (j > i) t += "-" + std::to_string(a[j] + 1);
    i = j + 1;
  }
  return t;
}

}  // namespace


bool resolveFragments(const vector<RecipeFragment>& fragments, int natoms,
                      vector<vector<int> >& atoms, string& why)
{
  atoms.assign(fragments.size(), vector<int>());
  vector<int> owner(natoms, -1);
  int rest = -1;
  for (size_t k = 0; k < fragments.size(); k++) {
    const string name = "fragment " + std::to_string(k + 1);
    if (trim(fragments[k].atoms) == "rest") {
      if (rest >= 0) { why = "only one fragment can be \"rest\""; return false; }
      rest = (int)k;
      continue;
    }
    vector<int> list;
    if (!atomList(fragments[k].atoms, list)) {
      why = name + ": \"" + fragments[k].atoms + "\" is not a list of atom numbers such as 2-13";
      return false;
    }
    for (int a : list) {
      if (a > natoms) {
        why = name + ": atom " + std::to_string(a) + ", but the complex has " +
              std::to_string(natoms) + " atoms";
        return false;
      }
      if (owner[a - 1] >= 0) {
        why = "atom " + std::to_string(a) + " is in fragment " + std::to_string(owner[a - 1] + 1) +
              " and in fragment " + std::to_string(k + 1) + "; fragments must not share atoms";
        return false;
      }
      owner[a - 1] = (int)k;
      atoms[k].push_back(a - 1);
    }
  }
  vector<int> missing;
  for (int a = 0; a < natoms; a++)
    if (owner[a] < 0) {
      if (rest >= 0) atoms[rest].push_back(a);
      else missing.push_back(a);
    }
  if (rest >= 0 && atoms[rest].empty()) { why = "the \"rest\" fragment has no atoms"; return false; }
  if (!missing.empty()) {
    why = "atoms " + listText(missing) + " are in no fragment; the fragments must cover the complex";
    return false;
  }
  return true;
}


int fragmentMultiplicity(const vector<int>& z, int charge, bool free)
{
  int e = -charge;
  for (int x : z) e += x;
  const int lowest = e % 2 ? 2 : 1;
  if (!free || z.size() != 1 || e < 0) return lowest;
  const int Z = z[0];
  // Ground terms of the neutral atoms and +1 ions of the d block (NIST):
  // the 4s/5s/6s occupation varies, so no rule gives them.
  static const int d3[10][2] = {{2, 3}, {3, 4}, {4, 5}, {7, 6}, {6, 7}, {5, 6}, {4, 3}, {3, 2}, {2, 1}, {1, 2}};
  static const int d4[10][2] = {{2, 1}, {3, 4}, {6, 5}, {7, 6}, {6, 7}, {5, 4}, {4, 3}, {1, 2}, {2, 1}, {1, 2}};
  static const int d5[10][2] = {{2, 3}, {3, 2}, {4, 5}, {5, 6}, {6, 7}, {5, 6}, {4, 5}, {3, 2}, {2, 1}, {1, 2}};
  int row = -1, col = -1;
  if (Z >= 21 && Z <= 30) { row = 3; col = Z - 21; }
  else if (Z >= 39 && Z <= 48) { row = 4; col = Z - 39; }
  else if (Z == 57 || (Z >= 72 && Z <= 80)) { row = 5; col = Z == 57 ? 0 : Z - 71; }
  if (row > 0) {
    if (charge == 0 || charge == 1) {
      const int (*t)[2] = row == 3 ? d3 : row == 4 ? d4 : d5;
      return t[col][charge];
    }
    // a higher ion keeps only d electrons: d^n, n = group - charge
    const int n = col + 3 - charge;
    if (n < 0 || n > 10) return lowest;
    return (n <= 5 ? n : 10 - n) + 1;
  }
  // main group: Hund's rule on the valence p shell, s electrons paired
  static const int nobleZ[] = {0, 2, 10, 18, 36, 54, 86};
  int core = 0;
  for (int k : nobleZ) if (k < Z) core = k;
  int v = e - core;
  if (core >= 18 && Z > core + 2) v -= 10;           // a filled d shell below the p block
  if (v <= 2) return lowest;
  const int p = v - 2;
  if (p > 6) return lowest;
  return (p <= 3 ? p : 6 - p) + 1;
}


vector<RecipeFragment> proposeFragments(const vector<int>& z, const vector<double>& xyz,
                                        const vector<double>& radii, int charge)
{
  const int n = (int)z.size();
  vector<RecipeFragment> out;
  if (n == 0) return out;
  int metal = -1;
  for (int i = 0; i < n && metal < 0; i++) if (isMetal(z[i])) metal = i;
  // Molecules: atoms joined by bonds (distance within 1.2 times the radii),
  // not through the metal.
  vector<int> mol(n, -1);
  int nmol = 0;
  for (int s = 0; s < n; s++) {
    if (mol[s] >= 0 || s == metal) continue;
    vector<int> todo(1, s);
    mol[s] = nmol;
    while (!todo.empty()) {
      const int a = todo.back();
      todo.pop_back();
      for (int b = 0; b < n; b++) {
        if (mol[b] >= 0 || b == metal) continue;
        double d2 = 0;
        for (int c = 0; c < 3; c++) d2 += (xyz[3 * a + c] - xyz[3 * b + c]) * (xyz[3 * a + c] - xyz[3 * b + c]);
        const double r = 1.2 * (radii[a] + radii[b]);
        if (d2 < r * r) { mol[b] = nmol; todo.push_back(b); }
      }
    }
    nmol++;
  }
  auto electrons = [&](const vector<int>& atoms) {
    int e = 0;
    for (int a : atoms) e += z[a];
    return e;
  };
  auto make = [&](const vector<int>& atoms, int q) {
    RecipeFragment f;
    f.atoms = listText(atoms);
    f.charge = q;
    f.mult = (electrons(atoms) - q) % 2 ? 2 : 1;
    return f;
  };
  if (metal >= 0) {
    int ligandCharge = 0;
    vector<int> rest;
    for (int m = 0; m < nmol; m++) {
      vector<int> atoms;
      for (int a = 0; a < n; a++) if (mol[a] == m) atoms.push_back(a);
      if (electrons(atoms) % 2) ligandCharge -= 1;
      rest.insert(rest.end(), atoms.begin(), atoms.end());
    }
    out.push_back(make(vector<int>(1, metal), charge - ligandCharge));
    if (!rest.empty()) out.push_back(make(rest, ligandCharge));
    return out;
  }
  if (nmol < 2) {
    vector<int> rest;
    for (int a = 1; a < n; a++) rest.push_back(a);
    out.push_back(make(vector<int>(1, 0), 0));
    if (!rest.empty()) out.push_back(make(rest, charge));
    return out;
  }
  for (int m = 0; m < nmol; m++) {
    vector<int> atoms;
    for (int a = 0; a < n; a++) if (mol[a] == m) atoms.push_back(a);
    out.push_back(make(atoms, m == 0 ? charge : 0));
  }
  return out;
}


string checkFragments(const vector<RecipeFragment>& fragments, const vector<int>& z,
                      int charge, string *warning)
{
  if (fragments.size() < 2) return "a fragment analysis needs at least two fragments";
  vector<vector<int> > atoms;
  string why;
  if (!resolveFragments(fragments, (int)z.size(), atoms, why)) return why;
  int sum = 0;
  for (size_t k = 0; k < fragments.size(); k++) {
    int e = -fragments[k].charge;
    for (int a : atoms[k]) e += z[a];
    if (e < 0) return "fragment " + std::to_string(k + 1) + ": more charge than electrons";
    if ((e + fragments[k].mult) % 2 == 0)
      return "fragment " + std::to_string(k + 1) + ": " + std::to_string(e) +
             " electrons cannot have multiplicity " + std::to_string(fragments[k].mult);
    sum += fragments[k].charge;
  }
  if (warning)
    *warning = sum == charge ? string()
        : "The fragment charges add up to " + std::to_string(sum) + ", the complex has " +
          std::to_string(charge) + ".";
  return "";
}


bool codeAvailable(const string& code, const std::set<string>& registered)
{
  return registered.empty() || registered.count(code) > 0;
}


namespace {
// Names from the code editors' solvent lists. Empty entries are unavailable;
// aliases belong here so one choice has the same meaning in every step.
struct SolventNames { const char *name, *xtb, *orca, *g16, *qm; };
const SolventNames solvents[] = {
  {"water", "water", "water", "Water", "Water"},
  {"methanol", "methanol", "methanol", "Methanol", "Methanol"},
  {"ethanol", "", "ethanol", "Ethanol", "Ethanol"},
  {"acetonitrile", "acetonitrile", "acetonitrile", "Acetonitrile", "Acetonitrile"},
  {"DMSO", "dmso", "DMSO", "Dimethylsulfoxide", "DMSO"},
  {"chloroform", "chcl3", "chloroform", "Chloroform", "Chloroform"},
  {"toluene", "toluene", "toluene", "Toluene", "Toluene"},
  {"hexane", "hexane", "hexane", "", "Hexane"},
  {"acetone", "acetone", "acetone", "Acetone", ""},
  {"benzene", "benzene", "benzene", "Benzene", ""},
  {"dichloromethane", "ch2cl2", "CH2Cl2", "Dichloromethane", ""},
  {"THF", "thf", "THF", "Tetrahydrofuran", ""},
  {"DMF", "dmf", "DMF", "n,n-dimethylformamide", ""},
  {"nitromethane", "nitromethane", "nitromethane", "Nitromethane", ""},
  {"octanol", "octanol", "octanol", "n-Octanol", ""}
};
string solventName(const SolventNames& s, const string& code)
{
  if (code == "CREST" || code == "xtb") return s.xtb;
  if (code == "ORCA") return s.orca;
  if (code == "G16") return s.g16;
  if (code == "ECCE-QM") return s.qm;
  return "";
}
}

string recipeSolventModel(const string& code)
{
  if (code == "CREST" || code == "xtb") return "ALPB";
  if (code == "ORCA" || code == "ECCE-QM") return "CPCM";
  if (code == "G16") return "PCM";
  return "";
}

vector<string> recipeSolvents(const RecipeVariant& variant)
{
  vector<string> out = {"none"};
  for (const auto& s : solvents) {
    bool ok = true;
    for (const auto& step : variant.steps)
      if (!recipeSolventModel(step.code).empty() && solventName(s, step.code).empty()) ok = false;
    if (ok) out.push_back(s.name);
  }
  return out;
}

vector<std::pair<string, string>> recipeSolventSettings(const string& code, const string& solvent)
{
  string name;
  for (const auto& s : solvents) if (solvent == s.name) name = solventName(s, code);
  const bool gas = solvent == "none";
  if (!gas && name.empty()) return {};
  if (code == "CREST" || code == "xtb")
    return {{"ES.Theory.Solvent.Model", gas ? "None" : "ALPB"},
            {"ES.Theory.Solvent.Name", gas ? "water" : name}};
  if (code == "ORCA")
    return {{"ES.Theory.SCF.UseSolvation", gas ? "0" : "1"},
            {"ES.Theory.SCF.SolvationModel", "CPCM"},
            {"ES.Theory.SCF.Solvent", gas ? "water" : name}};
  if (code == "G16")
    return {{"ES.Theory.SCF.UseSCRF", gas ? "0" : "1"},
            {"ES.Theory.SCF.SCRF", "PCM"},
            {"ES.Theory.SCF.Solvent", gas ? "Water" : name}};
  if (code == "ECCE-QM")
    return {{"ES.Theory.ECCEQM.Solvent", gas ? "None" : name}};
  return {};
}

// "(CO)6" when the fragment is several copies of one bonded molecule, else
// its plain formula: a ligand set reads as what it is made of.
string groupedFormula(Fragment& frag)
{
  vector<TTagCountPair> *all = frag.tagCounts();
  const string plain = TFormula::empiricalFormula(*all, 0);
  delete all;
  const int n = frag.numAtoms();
  if (n < 2) return plain;
  if (frag.numBonds() == 0) frag.addCovalentBonds();
  vector<int> seen(n, 0);
  string first;
  int copies = 0, firstAtoms = 0;
  for (int a = 0; a < n; a++) {
    if (seen[a]) continue;
    vector<TTagCountPair> tc;
    int atoms = 0;
    vector<int> todo(1, a);
    seen[a] = 1;
    while (!todo.empty()) {
      TAtm *atom = frag.atomRef(todo.back());
      todo.pop_back();
      atoms++;
      if (atom->atomicSymbol() != "X") {
        TTagCountPair look;
        look.tag = atom->atomicSymbol();
        auto it = std::find(tc.begin(), tc.end(), look);
        if (it != tc.end()) it->count++;
        else { look.count = 1; tc.push_back(look); }
      }
      vector<TAtm*> *nb = atom->connectedAtoms();
      for (TAtm *b : *nb) {
        const int k = frag.atomIndex(*b);
        if (k >= 0 && k < n && !seen[k]) { seen[k] = 1; todo.push_back(k); }
      }
      delete nb;
    }
    if (tc.empty()) continue;
    const string f = TFormula::empiricalFormula(tc, 0);
    if (copies == 0) { first = f; firstAtoms = atoms; }
    else if (f != first) return plain;
    copies++;
  }
  // separate atoms (6 Cl) keep the plain formula
  if (copies < 2 || firstAtoms < 2) return plain;
  return "(" + first + ")" + std::to_string(copies);
}
