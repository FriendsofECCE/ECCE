// A machine's sourceFile under ECCE_TRANSPORT=direct against results recorded
// from the pty path (#204): the variables the file sets, PATH it prepends to,
// what it unsets, and a missing file must give the same answers, for a
// bash-syntax and a csh-syntax file.  Shells that are not installed are
// skipped.  golden/rcommand_source_<n>.txt, one per case below;
// ECCE_GOLDEN_RECORD=1 rewrites them from the pty run.

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

#include "comm/RCommand.H"
#include "golden.H"

using namespace std;

static string esc(const string& s)
{
  string r;
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '\r') r += "\\r";
    else if (s[i] == '\n') r += "\\n";
    else r += s[i];
  }
  return r;
}

static void put(const string& f, const string& content)
{
  ofstream(f.c_str()) << content;
}

static bool have(const string& shell)
{
  string cmd = "command -v " + shell + " >/dev/null 2>&1";
  return system(cmd.c_str()) == 0;
}

static bool scenario(bool direct, const string& shell, const string& srcFile,
                     const string& shellPath, const string& tmp,
                     vector<string>& log)
{
  if (direct) setenv("ECCE_TRANSPORT", "direct", 1);
  else setenv("ECCE_TRANSPORT", "pty", 1);

  RCommand rc("system", "", shell, "", "", "", "", shellPath, "", srcFile);
  if (!rc.isOpen()) {
    cout << "no session (" << (direct ? "direct" : "pty") << ", " << shell
         << "): " << rc.commError() << endl;
    return false;
  }
  if (direct && rc.expfid() != -1) {
    cout << "the direct transport was not used" << endl;
    return false;
  }

  string o;
  bool r;
#define ASK(name, cmd) \
  do { o = "untouched"; r = rc.execout(cmd, o); \
       log.push_back(string(name) + ": ret=" + (r ? "1" : "0") + " out=[" + \
                     esc(o) + "]"); } while (0)

  ASK("ECCE_T variables", "env | grep '^ECCE_T[1_]' | sort");
  ASK("PATH head", "echo $PATH | cut -d: -f1,2");
  // A missing command is worded by the shell, so only the verdict compares.
  r = rc.execout("ecce_src_tool", o);
  log.push_back(string("tool runs: ret=") + (r ? "1 out=[" + esc(o) + "]" : "0"));
  ASK("pwd before cd()", "pwd");
  string path;
  r = rc.which("ecce_src_tool", path);
  log.push_back("which: ret=" + string(r ? "1" : "0") + " out=[" + path + "]");
  r = rc.cd(tmp);
  log.push_back(string("cd: ret=") + (r ? "1" : "0"));
  ASK("pwd after cd()", "pwd");
  return true;
}

int main()
{
  if (!getenv("ECCE_REALUSER")) {
    const char* u = getenv("USER");
    setenv("ECCE_REALUSER", u ? u : "nobody", 1);
  }

  char tmpl[] = "/tmp/testRCommandSourceXXXXXX";
  string tmp = mkdtemp(tmpl);
  mkdir((tmp + "/bin").c_str(), 0755);
  mkdir((tmp + "/dest").c_str(), 0755);
  put(tmp + "/bin/ecce_src_tool", "#!/bin/sh\necho tool ran\n");
  chmod((tmp + "/bin/ecce_src_tool").c_str(), 0755);
  setenv("ECCE_T_INHERIT", "inherited", 1);

  put(tmp + "/src.bash",
      "echo noise from the file\n"
      "export ECCE_T1=\"a b 'q' \\\"d\\\" \\$x\"\n"
      "export PATH=" + tmp + "/bin:$PATH\n"
      "unset ECCE_T_INHERIT\n"
      "alias ll='ls -l'\n"
      "cd " + tmp + "/dest\n"
      "nonexistent_command_ecce\n"
      "export ECCE_T_AFTER=yes\n");
  put(tmp + "/src.csh",
      "echo noise from the file\n"
      "setenv ECCE_T1 \"a b 'q' x\"\n"
      "setenv PATH \"" + tmp + "/bin:$PATH\"\n"
      "unsetenv ECCE_T_INHERIT\n"
      "alias ll 'ls -l'\n"
      "cd " + tmp + "/dest\n"
      "nonexistent_command_ecce\n"
      "setenv ECCE_T_AFTER yes\n");

  struct Case { const char* shell; string file; string pathPrefix; } cases[] = {
    { "bash", tmp + "/src.bash", "" },
    { "bash", tmp + "/src.bash", "/opt/ecce_pfx" },
    { "bash", "/nonexistent_ecce_src", "" },
    { "csh", tmp + "/src.csh", "" },
    { "csh", tmp + "/src.csh", "/opt/ecce_pfx" },
    { "csh", "/nonexistent_ecce_src", "" },
    { "tcsh", tmp + "/src.csh", "" },
  };

  int failures = 0, ran = 0;
  for (size_t c = 0; c < sizeof(cases) / sizeof(cases[0]); c++) {
    const Case& k = cases[c];
    if (!have(k.shell)) {
      cout << "SKIP " << k.shell << " is not installed" << endl;
      continue;
    }
    cout << "== " << k.shell << " file=" << k.file << " shellPath=["
         << k.pathPrefix << "]" << endl;
    vector<string> log;
    char name[64];
    snprintf(name, sizeof name, "rcommand_source_%d", (int)c);
    golden::Subs subs;
    subs.push_back(make_pair(tmp, string("@TMP@")));
    // "PATH head" shows the caller's own first directories after ours, or
    // alone when the file was not read.
    {
      string path = getenv("PATH") ? getenv("PATH") : "";
      size_t c1 = path.find(':'), c2 = path.find(':', c1 + 1);
      subs.push_back(make_pair(path.substr(0, c2) + "\\r",
                               string("@PATH0@:@PATH1@\\r")));
      subs.push_back(make_pair(":" + path.substr(0, path.find(':')) + "\\r",
                               string(":@PATH0@\\r")));
    }
    if (golden::recording()) {
      if (!scenario(false, k.shell, k.file, k.pathPrefix, tmp, log)) {
        cout << "SKIP: no pty session, nothing recorded" << endl;
        continue;
      }
      golden::record(name, log, subs);
      ran++;
      continue;
    }
    if (!scenario(true, k.shell, k.file, k.pathPrefix, tmp, log)) {
      failures++;
      cout << "FAIL: no direct session" << endl;
      continue;
    }
    ran++;
    failures += golden::compare(name, log, subs);
  }

  // A shell that cannot be started is a failed connection, with its message.
  {
    setenv("ECCE_TRANSPORT", "direct", 1);
    put(tmp + "/src.bad", "exit 3\n");
    RCommand rc("system", "", "bash", "", "", "", "", "", "", tmp + "/src.bad");
    bool ok = !rc.isOpen() && rc.commError().find("src.bad") != string::npos;
    cout << (ok ? "ok   " : "FAIL ") << "file that exits fails the connection: ["
         << rc.commError() << "]" << endl;
    if (!ok) failures++;
  }

  string cmd = "rm -rf " + tmp;
  if (system(cmd.c_str()) != 0) {}
  if (ran == 0) { cout << "SKIP: nothing ran" << endl; return 77; }
  cout << (failures ? "FAILED " : "PASSED ") << failures << " difference(s)"
       << endl;
  return failures ? 1 : 0;
}
