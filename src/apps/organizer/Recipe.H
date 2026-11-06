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
 *       thermo (the key data gives its G),
 *       charge=<+N|-N> (the source's charge plus N, for a step after CREST Protonation Sites),
 *       fragment (made once per fragment set in the set-up panel: that
 *       fragment's atoms of the source's structure, its charge and multiplicity)
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
  int chargeShift = 0;   // added to the charge carried from the source (a protonated cation: +1)
  std::string keyRole;
  bool perFragment = false;   ///< the "fragment" flag
  int fragment = 0;           ///< after expandFragments: 1.. (0 = not a fragment step)
  std::string fragments;      ///< after expandFragments: every fragment, fragmentsText()
};

/// A fragment of a structure: atom numbers (1-based, "2-13,15", or "rest"
/// for every atom no other fragment has), charge and spin multiplicity.
struct RecipeFragment {
  std::string atoms;
  int charge = 0;
  int mult = 1;
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

/// Shared solvent names accepted by every mapped code in the variant.
std::vector<std::string> recipeSolvents(const RecipeVariant& variant);
std::string recipeSolventModel(const std::string& code);
std::vector<std::pair<std::string, std::string>> recipeSolventSettings(
    const std::string& code, const std::string& solvent);

// ---- fragments (#201) ----

/// True when a step of the variant is made once per fragment.
bool hasFragmentSteps(const RecipeVariant& variant);

/// The variant with each "fragment" step made once per fragment (ids
/// "<id>-1", "<id>-2", ...) and every link to or from it repeated.
RecipeVariant expandFragments(const RecipeVariant& variant,
                              const std::vector<RecipeFragment>& fragments);

/// "1:0:1/2-13:0:1" (atoms:charge:multiplicity, "/" between fragments), and back.
std::string fragmentsText(const std::vector<RecipeFragment>& fragments);
bool parseFragments(const std::string& text, std::vector<RecipeFragment>& out);

/// The 0-based atoms of every fragment of a structure of `natoms` atoms;
/// false with `why` set when a list does not read, an atom is out of range,
/// two fragments share an atom or an atom is in none.
bool resolveFragments(const std::vector<RecipeFragment>& fragments, int natoms,
                      std::vector<std::vector<int> >& atoms, std::string& why);

/// The obvious split of a structure (atomic numbers, coordinates in
/// Angstrom, covalent radii, total charge): one metal atom | the rest, or one
/// fragment per molecule.  A ligand (molecule bonded to the metal) with an
/// odd electron count is an anion, the metal takes the remaining charge;
/// each fragment's multiplicity is the lowest its electron count allows.
std::vector<RecipeFragment> proposeFragments(const std::vector<int>& z,
                                             const std::vector<double>& xyz,
                                             const std::vector<double>& radii,
                                             int charge);

/// Problems that stop the steps from running (unreadable lists, a
/// multiplicity the electron count does not allow), "" when none; with
/// `warning` the charges that do not add up to the complex's.
std::string checkFragments(const std::vector<RecipeFragment>& fragments,
                           const std::vector<int>& z, int charge,
                           std::string *warning = 0);

/// A code label is usable when `registered` holds it (empty: all are).
bool codeAvailable(const std::string& code,
                   const std::set<std::string>& registered);

#endif
