/**
 * @file
 *
 * Testing and troubleshooting switches that exist as environment variables
 * and can also be ticked in Edit > Preferences > Advanced (#307), for
 * platforms where ECCE is started from a menu and no variable can be set.
 * No wx here.
 */
#ifndef ENVSWITCHES_HH
#define ENVSWITCHES_HH

#include <string>
#include <vector>
using std::string;
using std::vector;

namespace EnvSwitches
{
  struct Var { const char* name; const char* value; };

  struct Entry
  {
    const char* key;          ///< preference key (a boolean)
    const char* label;        ///< check box text
    const char* description;  ///< one line under it
    bool        takesFile;    ///< also has a file path, key + ".File"
    const char* defaultFile;  ///< file name in the user's ECCE folder
    vector<Var> vars;         ///< what the switch sets (a file switch: name only)
  };

  const vector<Entry>& all();

  /// Sets, in this process, the variables of every switch ticked in the
  /// global preferences, and clears those an earlier call set for a switch
  /// since unticked.  A variable that was set some other way is left alone.
  /// Run at the start of every program, so a child sees the same
  /// environment as the program that started it.
  void apply();

  /// True when `var` holds a value the user set outside Preferences.
  bool setExplicitly(const string& var);

  /// True when `var` holds a value this module set from Preferences.
  bool viaPreference(const string& var);

  /// The file a file switch writes to: the saved path, else the default.
  string fileFor(const Entry& e, const string& saved);
}

#endif
