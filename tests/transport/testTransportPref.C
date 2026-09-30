// RCommand::transportMode(): ECCE_TRANSPORT wins, then the Edit >
// Preferences switch, else the pty path.  The preference is written in an
// isolated ECCE_REALUSERHOME with the same Preferences class the dialog uses.

#include <cstdlib>
#include <iostream>
#include <string>

#include <unistd.h>

#include "comm/RCommand.H"
#include "util/PreferenceLabels.H"
#include "util/Preferences.H"

using namespace std;

static int failures = 0;

static void same(const string& name, const string& got, const string& want)
{
  bool ok = got == want;
  cout << (ok ? "ok   " : "FAIL ") << name << " -> " << got << endl;
  if (!ok) failures++;
}

int main()
{
  char dir[] = "/tmp/ecce-transportpref-XXXXXX";
  if (!mkdtemp(dir)) return 2;
  setenv("ECCE_REALUSERHOME", dir, 1);
  setenv("ECCE_HOME", dir, 1);
  string cmd = string("mkdir -p ") + dir + "/.ECCE " + dir + "/data/client/config && touch " + dir + "/data/client/config/errmsg";
  if (system(cmd.c_str()) != 0) return 2;

  unsetenv("ECCE_TRANSPORT");
  same("no preference file, no variable", RCommand::transportMode(), "pty");

  {
    Preferences pref(PrefLabels::GLOBALPREFFILE);
    pref.setBool(PrefLabels::BUILTINSSH, false);
    pref.saveFile();
  }
  same("preference unticked", RCommand::transportMode(), "pty");

  {
    Preferences pref(PrefLabels::GLOBALPREFFILE);
    pref.setBool(PrefLabels::BUILTINSSH, true);
    pref.saveFile();
  }
  same("preference ticked", RCommand::transportMode(), "ssh");

  setenv("ECCE_TRANSPORT", "pty", 1);
  same("ECCE_TRANSPORT=pty beats the preference", RCommand::transportMode(), "pty");
  setenv("ECCE_TRANSPORT", "direct", 1);
  same("ECCE_TRANSPORT=direct beats the preference", RCommand::transportMode(), "direct");
  setenv("ECCE_TRANSPORT", "", 1);
  same("empty ECCE_TRANSPORT is unset", RCommand::transportMode(), "ssh");

  unsetenv("ECCE_TRANSPORT");
  unsetenv("ECCE_REALUSERHOME");
  unsetenv("ECCE_HOME");
  same("no user home: pty", RCommand::transportMode(), "pty");

  string rm = string("rm -rf ") + dir;
  if (system(rm.c_str()) != 0) return 2;
  return failures ? 1 : 0;
}
