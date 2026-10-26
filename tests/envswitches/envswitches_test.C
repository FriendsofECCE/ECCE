// Preferences > Advanced switches reach the environment of started
// programs, and a variable set by hand wins (#307).
#include "util/EnvSwitches.H"
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
using std::string;

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

// The value a freshly started child sees, as the session's programs do.
static string childSees(const char* var)
{
  string cmd = string("printf %s \"${") + var + "-UNSET}\"";
  FILE* p = popen(cmd.c_str(), "r");
  string out;
  char buf[256];
  while (p && fgets(buf, sizeof buf, p)) out += buf;
  if (p) pclose(p);
  return out;
}

static void setPref(const char* key, bool on, const char* file = 0)
{
  Preferences pref(PrefLabels::GLOBALPREFFILE);
  pref.setBool(key, on);
  if (file) pref.setString(string(key) + ".File", file);
  pref.saveFile();
}

int main()
{
  char tmpl[] = "/tmp/envsw_XXXXXX";
  if (!mkdtemp(tmpl)) return 2;
  string home = tmpl;
  mkdir((home + "/.ECCE").c_str(), 0755);
  setenv("ECCE_REALUSERHOME", home.c_str(), 1);
  setenv("ECCE_HOME", home.c_str(), 1);
  const char* vars[] = {"ECCE_NATIVE_DIALOGS", "ECCE_GL_PERPIXEL",
      "ECCE_NO_PROGRESS_BACKFILL", "ECCE_DEBUG_GL", "ECCE_DEBUG_MOSYM_LOG",
      "ECCE_PREF_ENV"};
  for (const char* v : vars) unsetenv(v);

  // Nothing ticked: nothing set.
  EnvSwitches::apply();
  CHECK(childSees("ECCE_NATIVE_DIALOGS") == "UNSET");
  CHECK(childSees("ECCE_GL_PERPIXEL") == "UNSET");

  // Ticked: set here and seen by a child.
  setPref("Advanced.NativeDialogs", true);
  setPref("Advanced.PerPixelOff", true);
  setPref("Advanced.NoProgressBackfill", true);
  setPref("Advanced.DebugGL", true);
  EnvSwitches::apply();
  CHECK(childSees("ECCE_NATIVE_DIALOGS") == "1");
  CHECK(childSees("ECCE_GL_PERPIXEL") == "0");
  CHECK(childSees("ECCE_NO_PROGRESS_BACKFILL") == "1");
  CHECK(childSees("ECCE_DEBUG_GL") == "1");
  CHECK(childSees("ECCE_DEBUG_GL_VISUAL") == "1");

  // File switch: default file in the user's ECCE folder, then a chosen one.
  setPref("Advanced.MoSymLog", true);
  EnvSwitches::apply();
  CHECK(childSees("ECCE_DEBUG_MOSYM_LOG") == home + "/.ECCE/mosym.log");
  setPref("Advanced.MoSymLog", true, "/some/where.log");
  EnvSwitches::apply();
  CHECK(childSees("ECCE_DEBUG_MOSYM_LOG") == "/some/where.log");

  // Unticked later in the same environment (a program started after the
  // change inherits the old values): they go away again.
  setPref("Advanced.NativeDialogs", false);
  setPref("Advanced.MoSymLog", false);
  EnvSwitches::apply();
  CHECK(childSees("ECCE_NATIVE_DIALOGS") == "UNSET");
  CHECK(childSees("ECCE_DEBUG_MOSYM_LOG") == "UNSET");
  CHECK(childSees("ECCE_GL_PERPIXEL") == "0");

  // A variable set by hand wins, ticked or not, and is never removed.
  for (const char* v : vars) unsetenv(v);
  setenv("ECCE_GL_PERPIXEL", "1", 1);
  setenv("ECCE_NATIVE_DIALOGS", "yes", 1);
  setPref("Advanced.NativeDialogs", false);
  EnvSwitches::apply();
  CHECK(childSees("ECCE_GL_PERPIXEL") == "1");
  CHECK(EnvSwitches::setExplicitly("ECCE_GL_PERPIXEL"));
  CHECK(childSees("ECCE_NATIVE_DIALOGS") == "yes");

  // OpenGL drawing choice (#566): a preference value becomes ECCE_GL_PATH,
  // automatic removes it again, a variable set by hand wins.
  unsetenv("ECCE_GL_PATH");
  {
    Preferences pref(PrefLabels::GLOBALPREFFILE);
    pref.setString("Advanced.GLPath", "software");
    pref.saveFile();
    EnvSwitches::apply();
    CHECK(childSees("ECCE_GL_PATH") == "software");
    CHECK(EnvSwitches::viaPreference("ECCE_GL_PATH"));
    pref.setString("Advanced.GLPath", "auto");
    pref.saveFile();
    EnvSwitches::apply();
    CHECK(childSees("ECCE_GL_PATH") == "UNSET");
    setenv("ECCE_GL_PATH", "hardware", 1);
    pref.setString("Advanced.GLPath", "software");
    pref.saveFile();
    EnvSwitches::apply();
    CHECK(childSees("ECCE_GL_PATH") == "hardware");
    CHECK(!EnvSwitches::viaPreference("ECCE_GL_PATH"));
    unsetenv("ECCE_GL_PATH");
    pref.setString("Advanced.GLPath", "auto");
    pref.saveFile();
  }

  // Not an ECCE session: leaves the environment alone.
  for (const char* v : vars) unsetenv(v);
  unsetenv("ECCE_REALUSERHOME");
  EnvSwitches::apply();
  CHECK(childSees("ECCE_NO_PROGRESS_BACKFILL") == "UNSET");

  unlink((home + "/.ECCE/EcceGlobal").c_str());
  rmdir((home + "/.ECCE").c_str());
  rmdir(home.c_str());
  printf(fails ? "FAILED\n" : "ok\n");
  return fails ? 1 : 0;
}
