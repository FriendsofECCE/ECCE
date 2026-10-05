// EcceShell on a local machine: the terminal comes from ECCE_TERMINAL like
// the remote one, and a directory with a space and a quote reaches the shell
// intact.  Stub terminals record their argv and run the command; a stub
// $SHELL records where it started.  No display is needed (xset is a stub).

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

#include "comm/EcceShell.H"

using namespace std;

static int failures = 0;

static void check(const string& name, bool ok)
{
  cout << (ok ? "ok   " : "FAIL ") << name << endl;
  if (!ok) failures++;
}

static void same(const string& name, const string& got, const string& want)
{
  check(name, got == want);
  if (got != want) cout << "     got  [" << got << "]\n     want [" << want << "]" << endl;
}

static string join(const vector<string>& v, size_t from = 0)
{
  string s;
  for (size_t i = from; i < v.size(); i++) s += (i > from ? "|" : "") + v[i];
  return s;
}

static void put(const string& f, const string& text, bool exe = false)
{
  ofstream(f.c_str()) << text;
  if (exe) chmod(f.c_str(), 0755);
}

static vector<string> lines(const string& f)
{
  vector<string> v;
  ifstream in(f.c_str());
  string l;
  while (getline(in, l)) v.push_back(l);
  return v;
}

// Records argv, honours the working-directory options it was given, then
// runs what follows the exec flag as the real terminal would.
static const char* STUB =
  "#!/bin/bash\n"
  "printf '%s\\n' \"$0\" \"$@\" > \"$STUB_OUT.argv\"\n"
  "wd=\n"
  "while [ $# -gt 0 ]; do\n"
  "  case $1 in\n"
  "    --working-directory=*) wd=${1#*=} ;;\n"
  "    --workdir|--directory) shift; wd=$1 ;;\n"
  "    -e|--|-x) shift; break ;;\n"
  "  esac\n"
  "  shift\n"
  "done\n"
  "cd \"${wd:-/}\"\n"
  "\"$@\" > \"$STUB_OUT.run\" 2>&1\n"
  "touch \"$STUB_OUT.done\"\n";

struct Run { string msg; int status; vector<string> argv, run; };

static Run go(const string& out, const string& kind, const string& machine,
              const string& base, const string& full, const string& cmd = "")
{
  string rm = "rm -f '" + out + "'.*";
  if (system(rm.c_str())) {}
  setenv("STUB_OUT", out.c_str(), 1);
  EcceShell sh;
  Run r;
  if (kind == "dir") r.msg = sh.dirshell("Calc", machine, "", "", "", base, full);
  else if (kind == "cmd") r.msg = sh.cmdshell("Calc", machine, "", "", "", cmd, full);
  else r.msg = sh.topshell(machine, "", "", "");
  r.status = sh.lastStatus();
  for (int i = 0; r.status != -1 && i < 100 &&
                  access((out + ".done").c_str(), F_OK) != 0; i++)
    usleep(100000);
  r.argv = lines(out + ".argv");
  r.run = lines(out + ".run");
  return r;
}

int main()
{
  char tmpl[] = "/tmp/ecce-ltermXXXXXX";
  const string tmp = mkdtemp(tmpl);
  const string bin = tmp + "/bin", home = tmp + "/home", eh = tmp + "/ecce";
  mkdir(bin.c_str(), 0755);
  mkdir(home.c_str(), 0755);
  mkdir((home + "/.ECCE").c_str(), 0755);
  mkdir(eh.c_str(), 0755);
  if (symlink(ECCE_SOURCE_DIR "/siteconfig", (eh + "/siteconfig").c_str())) {}
  if (symlink(ECCE_SOURCE_DIR "/data", (eh + "/data").c_str())) {}
  put(home + "/.ECCE/MyMachines",
      "tlocal\tlocalhost\tt\tt\tt\t1:1\tssh\tna\tna\n");
  setenv("ECCE_HOME", eh.c_str(), 1);
  setenv("ECCE_REALUSERHOME", home.c_str(), 1);
  if (!getenv("ECCE_REALUSER")) {
    const char* u = getenv("USER");
    setenv("ECCE_REALUSER", u ? u : "nobody", 1);
  }

  put(bin + "/xset", "#!/bin/sh\nexit 0\n", true);
  put(bin + "/pwdshell", "#!/bin/sh\npwd\n", true);
  const char* terms[] = { "xterm", "gnome-terminal", "konsole", "myterm" };
  for (int i = 0; i < 4; i++) put(bin + "/" + terms[i], STUB, true);
  const char* path = getenv("PATH");
  setenv("PATH", (bin + ":" + (path ? path : "/usr/bin:/bin")).c_str(), 1);
  setenv("SHELL", (bin + "/pwdshell").c_str(), 1);

  const string calc = tmp + "/runs/calc dir 'q'";
  mkdir((tmp + "/runs").c_str(), 0755);
  mkdir(calc.c_str(), 0755);
  put(calc + "/out file.txt", "A\nB\nLASTLINE\n");
  const string out = tmp + "/o";
  string err;

  // The argv alone, per terminal.
  EcceShell::LocalTerminal t;
  t.title = "My Calc";
  t.mshell = "bash";
  t.dir = calc;
  vector<string> argv;
  // Whether the quoted cd lands in calc is checked by running it, below.
  const string cdLine = "exec bash -c 'cd ";

  setenv("ECCE_TERMINAL", "xterm", 1);
  check("xterm argv", EcceShell::localTerminalArgv(t, argv, err));
  check("xterm: its options", argv.size() > 8 && argv[0] == bin + "/xterm" &&
        argv[1] == "-title" && argv[2] == "My Calc" && argv[3] == "-bg" &&
        argv[7] == "-sb");
  same("xterm: -e, then a cd in the command", join(argv, argv.size() - 4),
       "-e|/bin/sh|-c|" + argv.back());
  check("xterm: cd in the command", argv.back().compare(0, cdLine.size(), cdLine) == 0);

  setenv("ECCE_TERMINAL", "gnome-terminal", 1);
  EcceShell::localTerminalArgv(t, argv, err);
  same("gnome-terminal: --working-directory, --, no xterm options", join(argv),
       bin + "/gnome-terminal|--working-directory=" + calc +
       "|--|/bin/sh|-c|exec bash -c 'exec $SHELL'");

  setenv("ECCE_TERMINAL", "konsole", 1);
  EcceShell::localTerminalArgv(t, argv, err);
  same("konsole: --workdir DIR -e", join(argv),
       bin + "/konsole|--workdir|" + calc +
       "|-e|/bin/sh|-c|exec bash -c 'exec $SHELL'");

  setenv("ECCE_TERMINAL", "myterm --class ecce", 1);
  EcceShell::localTerminalArgv(t, argv, err);
  same("Other: its arguments, -e", join(argv, 0),
       bin + "/myterm|--class|ecce|-e|/bin/sh|-c|" + argv.back());
  check("Other: cd in the command", argv.back().compare(0, cdLine.size(), cdLine) == 0);

  t.dir = "~";
  setenv("ECCE_TERMINAL", "gnome-terminal", 1);
  EcceShell::localTerminalArgv(t, argv, err);
  same("~ is left to the shell", join(argv, 1),
       "--|/bin/sh|-c|exec bash -c 'cd ~ && exec $SHELL'");

  setenv("ECCE_TERMINAL", (tmp + "/nonesuch").c_str(), 1);
  check("missing terminal is an error",
        !EcceShell::localTerminalArgv(t, argv, err) &&
        err.find("Could not find terminal") == 0);

  // Through the EcceShell entry points the Organizer, Launcher and MD
  // Prepare use, on a machine registered as localhost.
  for (int i = 0; i < 4; i++) {
    string term = terms[i];
    setenv("ECCE_TERMINAL", (term == "myterm" ? "myterm --class ecce" :
                             term.c_str()), 1);
    Run r = go(out, "dir", "tlocal", tmp + "/runs", calc);
    check(term + ": shell started [" + r.msg + "]", r.status == 0 && r.msg == "");
    check(term + ": terminal is " + term,
          !r.argv.empty() && r.argv[0] == bin + "/" + term);
    same(term + ": shell is in the calculation directory",
         r.run.empty() ? "" : r.run.back(), calc);
    bool xopts = r.argv.size() > 2 && r.argv[1] == "-title" &&
                 r.argv[2] == "Calc";
    check(term + ": xterm options only for xterm", xopts == (term == "xterm"));
  }

  setenv("ECCE_TERMINAL", "gnome-terminal", 1);
  Run r = go(out, "dir", "tlocal", tmp + "/runs", tmp + "/runs/nosuch");
  check("missing calc dir: base directory [" + r.msg + "]",
        r.status == 0 && r.msg.find("starting shell in base") != string::npos &&
        !r.run.empty() && r.run.back() == tmp + "/runs");

  r = go(out, "top", "tlocal", "", "");
  check("top shell in home", r.status == 0 && !r.run.empty() &&
        r.run.back() == string(getenv("HOME")));

  // Tail, as CalcMgr builds it.
  const string file = calc + "/out file.txt";
  setenv("ECCE_TERMINAL", "xterm", 1);
  r = go(out, "cmd", "tlocal", "", file,
         "tail -n 1 " + EcceShell::shellQuote(file));
  check("tail started [" + r.msg + "]", r.status == 0 && r.msg == "");
  same("tail shows the file", r.run.empty() ? "" : r.run.back(), "LASTLINE");
  check("tail: xterm geometry", join(r.argv).find("|-geom|80x40|") != string::npos);

  r = go(out, "cmd", "tlocal", "", calc + "/nosuch", "tail -f x");
  check("tail of a missing file is refused",
        r.status == -1 && r.msg.find("does not exist") != string::npos);

  string rm = "rm -rf '" + tmp + "'";
  if (system(rm.c_str())) {}
  cout << (failures ? "FAILED" : "PASSED") << endl;
  return failures ? 1 : 0;
}
