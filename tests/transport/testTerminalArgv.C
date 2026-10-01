// EcceShell's local-terminal command for an ssh machine (#204): the remote
// command line, the terminal argv and the detached start, with stub terminals
// standing in for xterm.  No connection is made.

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

static string stub(const string& dir, const string& name, const string& out)
{
  string f = dir + "/" + name;
  ofstream(f.c_str()) << "#!/bin/sh\nfor a in \"$@\"; do echo \"$a\"; done > "
                      << out << ".tmp\nmv " << out << ".tmp " << out << "\n";
  chmod(f.c_str(), 0755);
  return f;
}

int main()
{
  char tmpl[] = "/tmp/ecce-termXXXXXX";
  string dir = mkdtemp(tmpl);
  string err;

  // The remote command line.
  same("bash, source, dir",
       EcceShell::remoteCommand("bash", "/s.sh", "/d", "", err),
       "exec bash -c '[ -e /s.sh ] && . /s.sh; cd /d && exec $SHELL'");
  same("csh, source, dir",
       EcceShell::remoteCommand("csh", "/s.csh", "/d", "", err),
       "exec csh -c 'if (-e /s.csh) source /s.csh; cd /d && exec $SHELL'");
  same("tcsh, no source, home",
       EcceShell::remoteCommand("tcsh", "", "~", "", err),
       "exec tcsh -c 'cd ~ && exec $SHELL'");
  same("command",
       EcceShell::remoteCommand("bash", "", "", "tail -f /x/y", err),
       "exec bash -c 'tail -f /x/y'");
  same("command with quotes",
       EcceShell::remoteCommand("bash", "", "", "tail -f 'a b'", err),
       "exec bash -c 'tail -f '\\''a b'\\'''");
  same("directory with a space is quoted",
       EcceShell::remoteCommand("bash", "", "/a b", "", err),
       "exec bash -c 'cd '\\''/a b'\\'' && exec $SHELL'");
  err = "";
  check("! is refused",
        EcceShell::remoteCommand("csh", "", "/a!b", "", err) == "" && err != "");

  // The argv, with a stub named xterm (gets xterm's options) ...
  string out = dir + "/argv";
  setenv("ECCE_TERMINAL", stub(dir, "xterm", out).c_str(), 1);
  EcceShell::SshTerminal t;
  t.title = "My Title";
  t.host = "node1";
  t.remoteCommand = "exec bash -c 'cd /d && exec $SHELL'";
  vector<string> argv;
  check("argv builds", EcceShell::terminalArgv(t, argv, err));
  check("xterm: options then -e ssh",
        argv.size() > 8 && argv[0] == dir + "/xterm" && argv[1] == "-title" &&
        argv[2] == "My Title" && argv[3] == "-bg" && argv[5] == "-fg" &&
        argv[7] == "-sb");
  string tail = join(argv, argv.size() - 5);
  same("xterm: tail is -e ssh -t host cmd", tail,
       "-e|ssh|-t|node1|" + t.remoteCommand);

  t.geometry = "80x40";
  t.user = "bob";
  EcceShell::terminalArgv(t, argv, err);
  check("geometry", join(argv).find("|-sb|-geom|80x40|-e|") != string::npos);
  same("user", join(argv, argv.size() - 7),
       "-e|ssh|-t|-l|bob|node1|" + t.remoteCommand);

  t.frontend = "login";
  t.frontendMode = "forward";
  EcceShell::terminalArgv(t, argv, err);
  same("forward: -J user@front", join(argv, argv.size() - 9),
       "-e|ssh|-t|-J|bob@login|-l|bob|node1|" + t.remoteCommand);

  t.frontendMode = "nested";
  EcceShell::terminalArgv(t, argv, err);
  same("nested: ssh on the front end",
       join(argv, argv.size() - 7),
       "-e|ssh|-t|-l|bob|login|ssh -t -l 'bob' 'node1' "
       "'exec bash -c '\\''cd /d && exec $SHELL'\\'''");

  t.user = "";
  t.frontendMode = "forward";
  EcceShell::terminalArgv(t, argv, err);
  same("no user: no -l", join(argv, argv.size() - 7),
       "-e|ssh|-t|-J|login|node1|" + t.remoteCommand);

  // ... and with other terminals: no xterm options, their own exec flag.
  setenv("ECCE_TERMINAL", stub(dir, "kitty", out).c_str(), 1);
  EcceShell::terminalArgv(t, argv, err);
  same("kitty: no flag, no options", join(argv),
       dir + "/kitty|ssh|-t|-J|login|node1|" + t.remoteCommand);
  setenv("ECCE_TERMINAL", (stub(dir, "gnome-terminal", out) + " --wait").c_str(), 1);
  EcceShell::terminalArgv(t, argv, err);
  same("gnome-terminal: its arguments, then --", join(argv),
       dir + "/gnome-terminal|--wait|--|ssh|-t|-J|login|node1|" + t.remoteCommand);

  setenv("ECCE_TERMINAL", (dir + "/nonesuch").c_str(), 1);
  check("missing terminal is an error",
        !EcceShell::terminalArgv(t, argv, err) &&
        err == "Could not find terminal " + dir + "/nonesuch in path.");

  // Started detached, arguments intact.
  setenv("ECCE_TERMINAL", stub(dir, "xterm", out).c_str(), 1);
  t.remoteCommand = "exec bash -c 'echo \"$HOME\"; tail -f x'";
  EcceShell::terminalArgv(t, argv, err);
  check("spawn", EcceShell::spawnDetached(argv, err));
  for (int i = 0; i < 50 && access(out.c_str(), F_OK) != 0; i++) usleep(100000);
  ifstream in(out.c_str());
  string line, last;
  while (getline(in, line)) last = line;
  same("stub saw the remote command as one argument", last, t.remoteCommand);
  vector<string> bad(1, dir + "/nonesuch");
  check("spawn of a missing program fails", !EcceShell::spawnDetached(bad, err));

  string rm = "rm -rf " + dir;
  if (system(rm.c_str())) {}
  cout << (failures ? "FAILED" : "PASSED") << endl;
  return failures ? 1 : 0;
}
