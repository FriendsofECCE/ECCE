/**
 * @file
 *
 * Recipes (#710): a workflow template read from a text file, and the rules
 * that recommend a level of theory.  No wx and no data model, so the parser
 * and the rules are tested on their own.
 *
 * File format, one "key: value" per line, fields separated by "|", "#" starts
 * a comment:
 *
 *   recipe: <id>
 *   title: <name>                 field: <group>
 *   summary: <one line for the list>
 *   description: <text>
 *   role: <id> | <label> | <method> | <basis> | <why>
 *   variant: <id> | <label> | <code label>
 *   step: <variant> | <id> | <code> | <role> | <run type> | <caption> | <flags>
 *       flags (comma separated): structure (the box needs a structure),
 *       key=<reactant|product|ts|min|final> (its place in the key data),
 *       thermo (the key data gives its G)
 *   set: <variant> | <step id> | <ES key> | <value>
 *   link: <variant> | <from> | <to> | <link name>     (name as WorkflowSteps)
 *   rules: anion-diffuse, heavy-ecp, size, registered
 */

#ifndef RECIPE_HH
#define RECIPE_HH

#include <set>
#include <string>
#include <utility>
#include <vector>

struct RecipeRole {
  std::string id, label, method, basis, why;
};

struct RecipeStep {
  std::string id, code, role, runType, caption;
  std::vector<std::pair<std::string, std::string> > settings;
  bool needsStructure = false;
  bool thermo = false;
  std::string keyRole;
};

struct RecipeLink {
  std::string from, to, name;
};

struct RecipeVariant {
  std::string id, label, code;
  std::vector<RecipeStep> steps;
  std::vector<RecipeLink> links;
};

struct Recipe {
  std::string id, title, field, summary, description;
  std::vector<RecipeRole> roles;
  std::vector<RecipeVariant> variants;
  std::set<std::string> rules;
  bool site = false;           ///< from a site directory, not the install
};

/// False with `error` set when the text is not a usable recipe.
bool parseRecipe(const std::string& text, Recipe& out, std::string& error);

/// The text parseRecipe reads back as the same recipe.
std::string recipeText(const Recipe& recipe);

/// Every *.recipe in the directories, earlier directories first; a later
/// recipe with the id of an earlier one replaces it.  Unreadable files are
/// named in `problems`.
std::vector<Recipe> loadRecipes(const std::vector<std::string>& dirs,
                                std::vector<std::string>* problems = 0);

/// The install's recipes, then those of the site (Recipes/ in each site
/// configuration layer, central server included) and ECCE_RECIPE_DIRS.
std::vector<std::string> recipeDirs();

/// What the rules know of the structure the level of theory is for.
struct StructureFacts {
  bool known = false;
  int charge = 0;
  int atoms = 0;
  std::vector<int> elements;     ///< atomic numbers present
};

struct RoleChoice {
  std::string method, basis, why;
  std::vector<std::string> warnings;
};

/// The recommendation for a role under the recipe's rules: the role's
/// default, changed where a rule applies, with the reason.
RoleChoice recommend(const Recipe& recipe, const RecipeRole& role,
                     const StructureFacts& facts);

/// Warnings for a level of theory the user (or a box) has chosen; never an
/// error.
std::vector<std::string> checkLevel(const Recipe& recipe,
                                    const std::string& method,
                                    const std::string& basis,
                                    const StructureFacts& facts);

/// A code label is usable when `registered` holds it (empty: all are).
bool codeAvailable(const std::string& code,
                   const std::set<std::string>& registered);

#endif
