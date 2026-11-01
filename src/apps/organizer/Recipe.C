#include "Recipe.H"

#include <algorithm>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <sstream>

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
          else if (flag.compare(0, 4, "key=") == 0) s.keyRole = flag.substr(4);
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
      if (!s.keyRole.empty()) add("key=" + s.keyRole);
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


bool codeAvailable(const string& code, const std::set<string>& registered)
{
  return registered.empty() || registered.count(code) > 0;
}
