#include "util/EnvSwitches.H"

#include <cstdlib>
#include <set>
#include <sstream>

#include "util/Ecce.H"
#include "util/PosixCompat.H"
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"

// Names this module has set, comma separated.  A child process inherits the
// list with the values, so it can tell a value from Preferences (which the
// current preference replaces) from one the user set by hand (which wins).
static const char* kOurs = "ECCE_PREF_ENV";

const vector<EnvSwitches::Entry>& EnvSwitches::all()
{
  static vector<Entry> list;
  if (list.empty()) {
    list.push_back({"Advanced.NativeDialogs",
        "Native (C++) Details dialogs",
        "Opens the theory Details dialogs with the C++ engine instead of the Python one.",
        false, "", {{"ECCE_NATIVE_DIALOGS", "1"}}});
    list.push_back({"Advanced.PerPixelOff",
        "Per-pixel lighting off",
        "Draws the 3-D view with fixed-function lighting; try it when the picture is wrong or blank.",
        false, "", {{"ECCE_GL_PERPIXEL", "0"}}});
    list.push_back({"Advanced.NoProgressBackfill",
        "No progress records for older calculations",
        "The Organizer does not work out a progress record for a calculation that has none.",
        false, "", {{"ECCE_NO_PROGRESS_BACKFILL", "1"}}});
    list.push_back({"Advanced.DebugGL",
        "3-D view diagnostics",
        "Writes the lighting path and the OpenGL visual to the program's error output.",
        false, "", {{"ECCE_DEBUG_GL", "1"}, {"ECCE_DEBUG_GL_VISUAL", "1"}}});
    list.push_back({"Advanced.DebugWindows",
        "Window and panel diagnostics",
        "Writes how windows are raised and panels placed to the program's error output.",
        false, "", {{"ECCE_DEBUG_RAISE", "1"}, {"ECCE_DEBUG_PANELS", "1"}}});
    list.push_back({"Advanced.MoSymLog",
        "MO-diagram symmetry log",
        "Appends the symmetry analysis behind the MO diagram to this file.",
        true, "mosym.log", {{"ECCE_DEBUG_MOSYM_LOG", ""}}});
    list.push_back({"Advanced.DavAuthLog",
        "Data-server login log",
        "Appends what happens during data-server logins to this file.",
        true, "davauth.log", {{"ECCE_DEBUG_DAVAUTH", ""}}});
  }
  return list;
}

static std::set<string> ours()
{
  std::set<string> s;
  const char* v = getenv(kOurs);
  if (!v) return s;
  std::stringstream in(v);
  string name;
  while (std::getline(in, name, ',')) if (!name.empty()) s.insert(name);
  return s;
}

static void saveOurs(const std::set<string>& s)
{
  string v;
  for (std::set<string>::const_iterator i = s.begin(); i != s.end(); ++i)
    v += (v.empty() ? "" : ",") + *i;
  if (v.empty()) unsetenv(kOurs); else setenv(kOurs, v.c_str(), 1);
}

bool EnvSwitches::setExplicitly(const string& var)
{
  const char* v = getenv(var.c_str());
  return v && *v && ours().count(var) == 0;
}

string EnvSwitches::fileFor(const Entry& e, const string& saved)
{
  if (!saved.empty()) return saved;
  const char* home = getenv("ECCE_REALUSERHOME");
  if (!home || !*home) return "";
  return string(home) + "/.ECCE/" + e.defaultFile;
}

void EnvSwitches::apply()
{
  const char* home = getenv("ECCE_REALUSERHOME");
  if (!home || !*home) return;       // not an ECCE session

  Preferences pref(PrefLabels::GLOBALPREFFILE);
  std::set<string> mine = ours();
  const vector<Entry>& list = all();
  for (size_t i = 0; i < list.size(); i++) {
    const Entry& e = list[i];
    bool on = false;
    pref.getBool(e.key, on);
    string path;
    if (e.takesFile && on) {
      pref.getString(string(e.key) + ".File", path);
      path = fileFor(e, path);
      if (path.empty()) on = false;
    }
    for (size_t j = 0; j < e.vars.size(); j++) {
      string name = e.vars[j].name;
      if (setExplicitly(name)) continue;
      if (on) {
        setenv(name.c_str(), e.takesFile ? path.c_str() : e.vars[j].value, 1);
        mine.insert(name);
      } else if (mine.count(name)) {
        unsetenv(name.c_str());
        mine.erase(name);
      }
    }
  }
  saveOurs(mine);
}
