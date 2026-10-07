// UserEditor::getEditCommand with stub programs on PATH: the default editor
// and terminal per platform, and macOS open(1), which must wait (-W) for a
// copy of its own (-n) or the edit session ends before the file is opened.

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

#include "util/SFile.H"
#include "util/UserEditor.H"

using namespace std;

static int failures = 0;

static void same(const string& name, const string& got, const string& want)
{
  cout << (got == want ? "ok   " : "FAIL ") << name << endl;
  if (got != want) {
    cout << "     got  [" << got << "]\n     want [" << want << "]" << endl;
    failures++;
  }
}

static string command(const SFile& f, bool readOnly)
{
  UserEditor ed;
  string exe;
  char* args[64] = {0};
  ed.getEditCommand(f, exe, args, 64, "title", readOnly);
  string s;
  for (int i = 0; args[i]; i++) {
    string a = args[i];
    size_t slash = a.rfind('/');
    if (i == 0 && slash != string::npos) a = a.substr(slash + 1);
    s += (i ? "|" : "") + a;
  }
  ed.freeArguments(args);
  return s;
}

int main()
{
  char tmpl[] = "/tmp/ecce-edcmdXXXXXX";
  const string tmp = mkdtemp(tmpl);
  const string bin = tmp + "/bin", home = tmp + "/home";
  mkdir(bin.c_str(), 0755);
  mkdir(home.c_str(), 0755);
  mkdir((home + "/.ECCE").c_str(), 0755);
  const char* progs[] = { "open", "vi", "xterm", "ecce-macos-terminal" };
  for (int i = 0; i < 4; i++) {
    string p = bin + "/" + progs[i];
    ofstream(p.c_str()) << "#!/bin/sh\nexit 0\n";
    chmod(p.c_str(), 0755);
  }
  setenv("PATH", (bin + ":/usr/bin:/bin").c_str(), 1);
  setenv("HOME", home.c_str(), 1);
  setenv("ECCE_REALUSERHOME", home.c_str(), 1);
  unsetenv("ECCE_HOME");
  unsetenv("ECCE_TERMINAL");
  unsetenv("VISUAL");
  unsetenv("EDITOR");
  const string file = tmp + "/a b.txt";
  ofstream(file.c_str()) << "x\n";
  SFile f(file);

  unsetenv("ECCE_EDITOR");
#ifdef __APPLE__
  same("default editor", UserEditor::getPreferredEditor(), "open -t");
  same("default terminal", UserEditor::getTerminal(), "ecce-macos-terminal");
  same("default command", command(f, false), "open|-t|-W|-n|" + file);
#else
  same("default editor", UserEditor::getPreferredEditor(), "vi");
  same("default terminal", UserEditor::getTerminal(), "xterm");
#endif

  setenv("ECCE_EDITOR", "open -e", 1);
  same("open -e", command(f, false), "open|-e|-W|-n|" + file);
  same("open -e read-only", command(f, true), "open|-e|-W|-n|" + file);
  setenv("ECCE_EDITOR", "open -W -e", 1);
  same("open -W kept once", command(f, false), "open|-W|-e|-n|" + file);

  setenv("ECCE_EDITOR", "vi", 1);
  setenv("ECCE_TERMINAL", "ecce-macos-terminal", 1);
  same("vi in the macOS terminal", command(f, true),
       "ecce-macos-terminal|-e|vi|-R|" + file);
  setenv("ECCE_TERMINAL", "xterm", 1);
  string x = command(f, false);
  same("vi in xterm: options", x.substr(0, 17), "xterm|-geom|80x40");
  same("vi in xterm: command", x.substr(x.find("|-e|")), "|-e|vi|" + file);

  string rm = "rm -rf '" + tmp + "'";
  if (system(rm.c_str())) {}
  cout << (failures ? "FAILED" : "PASSED") << endl;
  return failures ? 1 : 0;
}
