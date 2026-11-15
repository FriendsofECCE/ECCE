#include <algorithm>
#include <fstream>
#include <sstream>
#include <set>

#include "dsm/CodeChoice.H"
#include "dsm/CodeFactory.H"
#include "dsm/MachinePreferences.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"
#include "util/Ecce.H"
#include "util/PreferenceLabels.H"
#include "util/Preferences.H"

using std::string;
using std::vector;

static const char *const kKey = "Codes.Shown";
static std::function<void()> g_listener;

void CodeChoice::setListener(std::function<void()> listener)
{
  g_listener = listener;
}

const vector<CodeChoice::Info>& CodeChoice::known()
{
  static const vector<Info> list = {
    {"ECCE-QM", "ECCE-QM",
     "Built-in quantum chemistry engine; nothing to install.", true},
    {"ORCA", "ORCA",
     "DFT, wavefunction methods and spectroscopy.", true},
    {"Gaussian-16", "Gaussian 16",
     "Molecular quantum chemistry: optimisations, scans, transition states, IRC.", true},
    {"xTB", "xTB",
     "Fast semi-empirical tight-binding energies, geometries and frequencies.", true},
    {"CREST", "CREST",
     "Conformer and rotamer search with xTB.", true},
    {"GROMACS", "GROMACS",
     "Classical molecular dynamics of liquids, solids and biomolecules.", true},
    {"CP2K", "CP2K",
     "Periodic DFT and ab initio MD for solids and surfaces.", true},
    {"NWChem", "NWChem",
     "Molecular and periodic quantum chemistry.", false},
    {"MOPAC", "MOPAC",
     "Semi-empirical molecular orbital methods.", false},
    {"QuantumESPRESSO", "Quantum ESPRESSO",
     "Plane-wave periodic DFT.", false},
    {"MLP", "MLP (MACE)",
     "Machine-learned potential energies and forces.", false},
    {"Vina", "AutoDock Vina",
     "Protein-ligand docking.", false},
  };
  return list;
}

string CodeChoice::displayName(const string& code)
{
  for (const Info& i : known())
    if (i.name == code) return i.display;
  return CodeFactory::displayName(code);
}

bool CodeChoice::registeredCode(const string& code)
{
  if (code == "ECCE-QM") return true;
  return !MachinePreferences::itemsForCode(code).empty();
}

// siteconfig/Codes: one line "Shown: ECCE-QM ORCA ...", or "Shown: all".
static bool siteList(vector<string>& out)
{
  std::ifstream in((string(Ecce::ecceHome()) + "/siteconfig/Codes").c_str());
  string line;
  while (in && std::getline(in, line)) {
    if (line.compare(0, 6, "Shown:") != 0) continue;
    std::istringstream words(line.substr(6));
    string w;
    while (words >> w) out.push_back(w);
    if (out.size() == 1 && out[0] == "all") {
      out.clear();
      for (const CodeChoice::Info& i : CodeChoice::known()) out.push_back(i.name);
    }
    return true;
  }
  return false;
}

vector<string> CodeChoice::defaultShown()
{
  vector<string> out;
  if (siteList(out)) return out;
  for (const Info& i : known())
    if (registeredCode(i.name)) out.push_back(i.name);
  return out;
}

// The stored list starts with a marker so that "nothing ticked" differs
// from "never chosen" even though an empty list reads back as absent.
bool CodeChoice::hasStoredChoice()
{
  vector<string> v;
  return Preferences(PrefLabels::GLOBALPREFFILE).getStringList(kKey, v) &&
         !v.empty() && v[0] == "chosen";
}

vector<string> CodeChoice::shownCodes()
{
  vector<string> v;
  if (Preferences(PrefLabels::GLOBALPREFFILE).getStringList(kKey, v) &&
      !v.empty() && v[0] == "chosen") {
    v.erase(v.begin());
    return v;
  }
  return defaultShown();
}

bool CodeChoice::shown(const string& code)
{
  bool listed = false;
  for (const Info& i : known()) listed = listed || i.name == code;
  if (!listed) return true;   // not a code this choice covers
  vector<string> v = shownCodes();
  return std::find(v.begin(), v.end(), code) != v.end();
}

vector<string> CodeChoice::hiddenCodes()
{
  vector<string> v = shownCodes(), out;
  for (const Info& i : known())
    if (std::find(v.begin(), v.end(), i.name) == v.end())
      out.push_back(i.name);
  return out;
}

int CodeChoice::hiddenCount()
{
  return (int)hiddenCodes().size();
}

void CodeChoice::setShown(const vector<string>& codes)
{
  Preferences pref(PrefLabels::GLOBALPREFFILE);
  vector<string> v(1, "chosen");
  v.insert(v.end(), codes.begin(), codes.end());
  pref.setStringList(kKey, v);
  pref.saveFile();
  if (g_listener) g_listener();
}

void CodeChoice::show(const string& code, bool on)
{
  vector<string> v = shownCodes();
  auto at = std::find(v.begin(), v.end(), code);
  if (on && at == v.end()) v.push_back(code);
  if (!on && at != v.end()) v.erase(at);
  setShown(v);
}

string CodeChoice::codeOfType(ResourceType* type)
{
  if (!type) return string();
  const string at = type->getApplicationType();
  for (const Info& i : known())
    if (i.name == at) return at;
  return string();
}

bool CodeChoice::typeShown(ResourceType* type)
{
  const string code = codeOfType(type);
  return code.empty() || shown(code);
}

bool CodeChoice::typeNameShown(const string& typeName)
{
  vector<ResourceType*> types =
      ResourceDescriptor::getResourceDescriptor().getResourceTypes();
  for (size_t i = 0; i < types.size(); i++)
    if (types[i]->getName() == typeName) return typeShown(types[i]);
  return true;
}

string CodeChoice::moreLabel(int n)
{
  return std::to_string(n) + (n == 1 ? " more code (hidden)..."
                                     : " more codes (hidden)...");
}

string CodeChoice::barLabel(int n)
{
  return "+" + std::to_string(n) + " hidden";
}

string CodeChoice::moreTip(int n)
{
  return std::to_string(n) + (n == 1 ? " code is" : " codes are") +
         " hidden; tick " + (n == 1 ? "it" : "them") +
         " in Preferences > Codes";
}
