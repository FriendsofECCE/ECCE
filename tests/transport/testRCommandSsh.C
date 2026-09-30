// RCommand over libssh (ECCE_TRANSPORT=ssh) against the pty ssh path as the
// oracle (#204): the same operations through RCommand to the test sshd must
// return the same values and output both ways, for a tcsh and a bash account.
// Needs the sshd from tests/transport/sshd/run.sh and a ~/.ssh with the test
// key, known_hosts and config that rcommand_test.sh sets up; exit 77 if absent.

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <sys/select.h>
#include <sys/stat.h>
#include <ctime>
#include <unistd.h>

#include "comm/RCommand.H"

using namespace std;

static int extra = 0;

static void check(const string& name, bool ok)
{
  cout << (ok ? "ok   " : "FAIL ") << name << endl;
  if (!ok) extra++;
}

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

static string rec(const string& name, bool ret, const string& out,
                  const string& err = "")
{
  return name + ": ret=" + (ret ? "1" : "0") + " out=[" + esc(out) + "]" +
         (err.empty() ? "" : " err=[" + esc(err) + "]");
}

static string readFile(const string& f)
{
  ifstream in(f.c_str());
  return string((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
}

static const char* HOST = "127.0.0.1";

static void setMode(bool ssh)
{
  if (ssh) setenv("ECCE_TRANSPORT", "ssh", 1);
  else unsetenv("ECCE_TRANSPORT");
}

// Runs the whole scenario; one line per operation.
static bool scenario(bool ssh, const string& user, const string& shell,
                     const string& rdir, const string& ldir,
                     vector<string>& log, bool& bgAlive)
{
  setMode(ssh);
  RCommand rc(HOST, "ssh", shell, user);
  if (!rc.isOpen()) {
    cout << "no session (" << (ssh ? "ssh" : "pty") << ", " << user << "): "
         << rc.commError() << endl;
    return false;
  }
  if (ssh && rc.expfid() != -1) {
    cout << "the ssh transport was not used" << endl;
    return false;
  }

  string o;
  bool r;
#define EXECOUT(name, cmd) \
  do { o = "untouched"; r = rc.execout(cmd, o); \
       log.push_back(rec(name, r, o, r ? "" : rc.commError())); } while (0)
  // The pty leaves its own "CMDSTAT=n" and prompt in the buffer and words
  // a missing command as csh does ("Could not find command"; bash gets the
  // generic text), so only the verdict is compared.
#define EXECOUT_STATUS_ONLY(name, cmd) \
  do { r = rc.execout(cmd, o); \
       log.push_back(rec(name, r, "")); } while (0)

  EXECOUT("echo", "echo hello");
  EXECOUT("multi-line", "echo a; echo b; echo c");
  EXECOUT("empty output", "true");
  // The pty path leaves a remote command's stderr on this process's own
  // stderr; the ssh path merges it into the output (checked in sshChecks).
  EXECOUT_STATUS_ONLY("stderr merged", "echo out; echo err 1>&2; echo out2");
  EXECOUT("status 1", "(exit 1)");
  EXECOUT_STATUS_ONLY("status 3", "echo before; (exit 3)");
  EXECOUT_STATUS_ONLY("status 127", "no_such_command_ecce");
  EXECOUT("status 2", "(exit 2)");
  EXECOUT("pipe", "echo abc | tr a-c A-C");

  r = rc.exec("echo quiet");
  log.push_back(rec("exec ok", r, ""));
  r = rc.exec("(exit 1)", "custom failure text");
  log.push_back(rec("exec fail", r, "", rc.commError()));

  r = rc.cd(rdir);
  log.push_back(rec("cd abs", r, "", r ? "" : rc.commError()));
  EXECOUT("pwd", "pwd");
  r = rc.cd("sub");
  log.push_back(rec("cd relative", r, "", r ? "" : rc.commError()));
  EXECOUT("pwd relative", "pwd");
  EXECOUT("ls in sub", "ls");
  r = rc.cd("no_such_dir");
  log.push_back(rec("cd missing", r, "", rc.commError()));
  EXECOUT("pwd after failed cd", "pwd");
  r = rc.cd("..");
  log.push_back(rec("cd ..", r, "", r ? "" : rc.commError()));
  EXECOUT("pwd after ..", "pwd");

  const char* names[] = { "plain", "script", "sub", "missing" };
  for (int i = 0; i < 4; i++) {
    string n = names[i];
    log.push_back(rec("exists " + n, rc.exists(n), ""));
    log.push_back(rec("directory " + n, rc.directory(n), ""));
    log.push_back(rec("writable " + n, rc.writable(n), ""));
    log.push_back(rec("executable " + n, rc.executable(n), ""));
  }

  string path;
  r = rc.which("sh", path);
  log.push_back(rec("which sh", r, path));
  r = rc.which("no_such_command_ecce", path);
  log.push_back(rec("which missing", r, path));
  r = rc.which("./script", path);
  log.push_back(rec("which ./script", r, path));

  // The pid belongs to the remote machine, so liveness is asked there.
  o = "";
  r = rc.execbg("sleep 20", o);
  bgAlive = r && atol(o.c_str()) > 1 && rc.exec("kill -0 " + o);
  log.push_back(rec("execbg", r, bgAlive ? "live pid" : "no live pid [" + o + "]"));
  if (bgAlive) rc.exec("kill -9 " + o);

  string dest = rdir + "/copies";
  rc.exec("mkdir -p " + dest);
  string fromPath = ldir + "/lplain";
  const char* from[] = { fromPath.c_str(), 0 };
  r = rc.shellput(from, dest);
  log.push_back(rec("shellput", r, "", r ? "" : rc.commError()));
  EXECOUT("shellput content", "cat " + dest + "/lplain");

  string getdir = ldir + "/got";
  mkdir(getdir.c_str(), 0755);
  unlink((getdir + "/plain").c_str());
  vector<string> src;
  src.push_back(rdir + "/plain");
  r = rc.shellget(src, getdir);
  log.push_back(rec("shellget", r, "", r ? "" : rc.commError()));
  log.push_back(rec("shellget content", true, readFile(getdir + "/plain")));
  src.clear();
  src.push_back(rdir + "/missing");
  r = rc.shellget(src, getdir);
  log.push_back(rec("shellget missing", r, ""));

  // The static copies and commands, each opening a connection of its own.
  string sdir = rdir + "/static", serr, sout;
  rc.exec("rm -rf " + sdir + " && mkdir -p " + sdir);
  vector<string> one;
  string lp = ldir + "/lplain", lo = ldir + "/lother", lt = ldir + "/tree",
         ls = ldir + "/lscript";
  one.push_back(lp);
  r = RCommand::put(serr, HOST, "ssh", user, "", one, sdir);
  log.push_back(rec("put file into dir", r, ""));
  EXECOUT("put content", "cat " + sdir + "/lplain");
  r = RCommand::put(serr, HOST, "ssh", user, "", one, sdir + "/renamed");
  log.push_back(rec("put file to new name", r, ""));
  EXECOUT("put renamed content", "cat " + sdir + "/renamed");
  one.clear();
  one.push_back(ls);
  r = RCommand::put(serr, HOST, "ssh", user, "", one, sdir);
  log.push_back(rec("put executable", r, ""));
  log.push_back(rec("put keeps the exec bit", rc.executable(sdir + "/lscript"), ""));
  one.clear();
  one.push_back(lp);
  one.push_back(lo);
  rc.exec("mkdir " + sdir + "/two");
  r = RCommand::put(serr, HOST, "ssh", user, "", one, sdir + "/two");
  log.push_back(rec("put two files into dir", r, ""));
  EXECOUT("put two listing", "ls " + sdir + "/two");
  one.clear();
  one.push_back(lt);
  r = RCommand::put(serr, HOST, "ssh", user, "", one, sdir);
  log.push_back(rec("put directory into dir", r, ""));
  EXECOUT("put directory listing", "cd " + sdir + " && find tree | sort");
  one.clear();
  one.push_back(ldir + "/l*");
  rc.exec("mkdir " + sdir + "/globbed");
  r = RCommand::put(serr, HOST, "ssh", user, "", one, sdir + "/globbed");
  log.push_back(rec("put glob into dir", r, ""));
  EXECOUT("put glob listing", "ls " + sdir + "/globbed");
  one.clear();
  one.push_back(lp);
  r = RCommand::put(serr, HOST, "ssh", user, "", one, "~/ecce_rcssh_tilde_" + user);
  log.push_back(rec("put to ~/", r, ""));
  EXECOUT("put ~/ content", "cat ~/ecce_rcssh_tilde_" + user + " && rm -f ~/ecce_rcssh_tilde_" + user);

  string gdir = ldir + "/gotstatic";
  string rm = "rm -rf " + gdir;
  if (system(rm.c_str()) != 0) {}
  mkdir(gdir.c_str(), 0755);
  one.clear();
  one.push_back(rdir + "/plain");
  r = RCommand::get(serr, HOST, "ssh", user, "", one, gdir);
  log.push_back(rec("get file into dir", r, readFile(gdir + "/plain")));
  r = RCommand::get(serr, HOST, "ssh", user, "", one, gdir + "/renamed");
  log.push_back(rec("get file to new name", r, readFile(gdir + "/renamed")));
  one.clear();
  one.push_back(rdir + "/sub");
  r = RCommand::get(serr, HOST, "ssh", user, "", one, gdir);
  log.push_back(rec("get directory", r, readFile(gdir + "/sub/inner")));
  one.clear();
  one.push_back(rdir + "/p*");
  r = RCommand::get(serr, HOST, "ssh", user, "", one, gdir);
  log.push_back(rec("get remote glob", r, readFile(gdir + "/plain")));
  one.clear();
  one.push_back(rdir + "/plain");
  one.push_back(rdir + "/script");
  r = RCommand::get(serr, HOST, "ssh", user, "", one, gdir);
  struct stat gsb;
  log.push_back(rec("get two files", r, readFile(gdir + "/script") +
                    (stat((gdir + "/script").c_str(), &gsb) == 0 &&
                     (gsb.st_mode & 0100) ? "+x" : "")));
  one.clear();
  one.push_back(rdir + "/no_such_*");
  r = RCommand::get(serr, HOST, "ssh", user, "", one, gdir);
  log.push_back(rec("get glob without a match", r, ""));
  one.clear();
  one.push_back(ldir + "/no_such_*");
  r = RCommand::put(serr, HOST, "ssh", user, "", one, sdir);
  log.push_back(rec("put glob without a match", r, ""));

  // The pty path's copy() ignores missing files; so does the ssh path.
  one.clear();
  one.push_back(ldir + "/no_such_file");
  r = RCommand::put(serr, HOST, "ssh", user, "", one, sdir);
  log.push_back(rec("put missing", r, serr));
  one.clear();
  one.push_back(lp);
  one.push_back(lo);
  r = RCommand::put(serr, HOST, "ssh", user, "", one, sdir + "/nodir");
  log.push_back(rec("put two files to a missing target", r, serr));
  EXECOUT("put two to missing target", "ls " + sdir + "; ls -l " + sdir + "/nodir");
  one.clear();
  one.push_back(rdir + "/missing");
  r = RCommand::get(serr, HOST, "ssh", user, "", one, gdir);
  log.push_back(rec("get missing file", r, serr));
  one.clear();
  one.push_back(rdir + "/plain");
  one.push_back(rdir + "/script");
  r = RCommand::get(serr, HOST, "ssh", user, "", one, gdir + "/renamed");
  log.push_back(rec("get two files to a non-directory", r, serr +
                    readFile(gdir + "/renamed")));
  r = RCommand::get(serr, HOST, "ssh", user, "", one, gdir + "/nolocaldir");
  log.push_back(rec("get two files to a missing local target", r, serr));
  { string ls = "ls -l " + gdir + "/nolocaldir 2>&1 | tail -n +2 | awk '{print $1, $NF}'";
    FILE* pp = popen(ls.c_str(), "r"); string lo2; char bb[512];
    while (pp && fgets(bb, sizeof bb, pp)) lo2 += bb; if (pp) pclose(pp);
    log.push_back(rec("missing local target now", true, lo2)); }
  r = RCommand::command("echo via command; echo two", sout, serr, HOST, "ssh",
                        shell, user);
  log.push_back(rec("command", r, sout));
  r = RCommand::command("(exit 1)", sout, serr, HOST, "ssh", shell, user);
  log.push_back(rec("command fails", r, ""));
  r = RCommand::command("echo", "a  b", sout, serr, HOST, "ssh", shell, user);
  log.push_back(rec("command with args", r, sout));
  return true;
}

// A stream is a command whose stdout is read from a descriptor and whose
// stdin is written from here: echo, merged stderr, EOF, interrupt, stop.
static void streamChecks(const string& user, const string& shell)
{
  setMode(true);
  RCommand rc(HOST, "ssh", shell, user, "", "", "", "/opt/ecce_test_path");
  string o;
  check("stream: open", rc.isOpen() && rc.canStream());
  rc.exec("rm -rf /tmp/ecce_stream_" + user + " && mkdir -p /tmp/ecce_stream_" + user);
  rc.cd("/tmp/ecce_stream_" + user);

  check("stream: starts", rc.startStream("pwd; echo err 1>&2; cat; echo after-eof"));
  check("stream: not started twice", !rc.startStream("cat"));
  int fd = rc.expfid();
  check("stream: has a descriptor", fd > 2);

  auto readFor = [&](const string& want, int ms) {
    string got;
    for (int waited = 0; waited < ms && got.find(want) == string::npos;) {
      fd_set f; FD_ZERO(&f); FD_SET(fd, &f);
      struct timeval tv = { 0, 100000 };
      if (select(fd + 1, &f, 0, 0, &tv) > 0) {
        char b[4096];
        ssize_t n = read(fd, b, sizeof b);
        if (n <= 0) break;
        got.append(b, n);
      } else waited += 100;
    }
    return got;
  };
  string first = readFor("err\n", 5000);
  check("stream: runs in the directory, stderr merged: " + esc(first),
        first.find("/tmp/ecce_stream_" + user + "\n") != string::npos &&
        first.find("err\n") != string::npos);
  check("stream: write reaches the script's stdin", rc.expwrite("ping 1"));
  check("stream: reply comes back",
        readFor("ping 1\n", 5000).find("ping 1\n") != string::npos);
  check("stream: a large reply survives",
        rc.expwrite(string(20000, 'x')) &&
        readFor(string(20000, 'x') + "\n", 5000).size() >= 20000);

  time_t t0 = time(0);
  rc.stopStream(3000);
  string tail;
  char b[256];
  ssize_t n;
  (void)n; (void)b;
  check("stream: stop returns within the grace period", time(0) - t0 <= 4);
  check("stream: descriptor is gone afterwards", rc.expfid() == -1);

  // EOF from the far end closes the descriptor.
  check("stream: starts (short)", rc.startStream("echo hi; sleep 1"));
  fd = rc.expfid();
  string hi = readFor("hi\n", 5000);
  check("stream: short script output", hi == "hi\n");
  bool eof = false;
  for (int i = 0; i < 100 && !eof; i++) {
    fd_set f; FD_ZERO(&f); FD_SET(fd, &f);
    struct timeval tv = { 0, 100000 };
    if (select(fd + 1, &f, 0, 0, &tv) > 0) {
      char c[64];
      eof = read(fd, c, sizeof c) == 0;
    }
  }
  check("stream: EOF when the script ends", eof);
  rc.stopStream();

  // ^C ends a long script's shell; the stream then ends.
  check("stream: starts (long)", rc.startStream("sleep 30; echo late"));
  fd = rc.expfid();
  usleep(500000);
  t0 = time(0);
  check("stream: interrupt accepted", rc.execout("\003", o));
  eof = false;
  for (int i = 0; i < 100 && !eof; i++) {
    fd_set f; FD_ZERO(&f); FD_SET(fd, &f);
    struct timeval tv = { 0, 100000 };
    if (select(fd + 1, &f, 0, 0, &tv) > 0) {
      char c[64];
      eof = read(fd, c, sizeof c) <= 0;
    }
  }
  check("stream: interrupt ends the script", eof && time(0) - t0 < 8);
  rc.stopStream();
  rc.exec("rm -rf /tmp/ecce_stream_" + user);
}

static void sshChecks(const string& user, const string& shell)
{
  setMode(true);
  {
    RCommand rc(HOST, "ssh", shell, user, "", "", "", "/opt/ecce_test_path",
                "/opt/ecce_test_lib");
    string p, l;
    check("ssh: open", rc.isOpen());
    rc.execout("echo $PATH", p);
    rc.execout("echo $LD_LIBRARY_PATH", l);
    check("ssh: shellPath prefixed", p.compare(0, 20, "/opt/ecce_test_path:") == 0);
    check("ssh: libPath prefixed", l.compare(0, 19, "/opt/ecce_test_lib:") == 0);
    check("ssh: remoteShellIsBash", rc.remoteShellIsBash());
    check("ssh: stderr merged into output",
          rc.execout("echo out; echo err 1>&2; echo out2", p) &&
          p == "out\r\nerr\r\nout2\r\n");
    check("ssh: hop refused", !rc.hop("elsewhere") &&
          rc.commError() == "hop is not available with ECCE_TRANSPORT=ssh");
    check("ssh: raw api fails without crashing",
          !rc.expwrite("date") && rc.expect1("x") == -1 && rc.expfid() == -1 &&
          !rc.isDirect() && rc.canStream());
  }
  {
    RCommand rc(HOST, "ssh", shell, user, "", "", "", "", "", "", false);
    check("allowDirect=false keeps the pty", rc.isOpen() && rc.expfid() > 0);
  }
  {
    RCommand rc(HOST, "ssh", shell, user, "", "", "", "", "", "", true, false);
    check("allowSsh=false keeps the pty", rc.isOpen() && rc.expfid() > 0);
  }
  {
    RCommand rc(HOST, "ssh", shell, user, "", "", "", "", "", "/nonexistent");
    check("a missing sourceFile stays on libssh", rc.isOpen() && rc.expfid() == -1);
  }
  {
    RCommand rc(HOST, "rsh", shell, user);
    check("rsh keeps the pty", rc.expfid() != -1 || !rc.isOpen());
  }
  {
    RCommand rc("system", "", "bash");
    string o;
    check("local machine is direct", rc.isOpen() && rc.execout("echo x", o) &&
          o == "x\r\n" && rc.expfid() == -1);
  }
}

// A sourceFile in the account's own login-shell syntax: the pty sources it in
// its shell, libssh in a shell run once at connect, and both must see the
// same variables, PATH, unset variables and working directory.
static bool sourceScenario(bool ssh, const string& user, const string& shell,
                           const string& srcFile, const string& shellPath,
                           const string& rdir, vector<string>& log)
{
  setMode(ssh);
  RCommand rc(HOST, "ssh", shell, user, "", "", "", shellPath, "", srcFile);
  if (!rc.isOpen()) {
    cout << "no session (" << (ssh ? "ssh" : "pty") << ", " << user << "): "
         << rc.commError() << endl;
    return false;
  }
  if (ssh && rc.expfid() != -1) {
    cout << "the ssh transport was not used" << endl;
    return false;
  }
  string o;
  bool r;
#define ASK(name, cmd) \
  do { o = "untouched"; r = rc.execout(cmd, o); \
       log.push_back(rec(name, r, o)); } while (0)
  ASK("ECCE_T variables", "env | grep '^ECCE_T[1_]' | sort; echo end");
  ASK("LOGNAME", "env | grep -c '^LOGNAME='");
  ASK("PATH head", "echo $PATH | cut -d: -f1,2");
  r = rc.execout("ecce_src_tool", o);
  log.push_back(string("tool runs: ret=") + (r ? "1 out=[" + esc(o) + "]" : "0"));
  ASK("pwd before cd()", "pwd");
  string path;
  r = rc.which("ecce_src_tool", path);
  log.push_back(rec("which", r, path));
  r = rc.cd(rdir);
  log.push_back(rec("cd", r, ""));
  ASK("pwd after cd()", "pwd");
  return true;
}

static void sourceChecks(const string& user, const string& shell,
                         const string& rdir, const string& ldir)
{
  bool csh = shell == "csh";
  string body = csh ?
    "echo noise from the file\n"
    "setenv ECCE_T1 \"a b 'q' x\"\n"
    "setenv PATH \"" + rdir + "/bin:$PATH\"\n"
    "unsetenv LOGNAME\n"
    "alias ll 'ls -l'\n"
    "cd " + rdir + "/sub\n"
    "nonexistent_command_ecce\n"
    "setenv ECCE_T_AFTER yes\n" :
    "echo noise from the file\n"
    "export ECCE_T1=\"a b 'q' \\\"d\\\" \\$x\"\n"
    "export PATH=" + rdir + "/bin:$PATH\n"
    "unset LOGNAME\n"
    "alias ll='ls -l'\n"
    "cd " + rdir + "/sub\n"
    "nonexistent_command_ecce\n"
    "export ECCE_T_AFTER=yes\n";
  { ofstream f((ldir + "/srcfile").c_str()); f << body; }
  { ofstream f((ldir + "/srctool").c_str()); f << "#!/bin/sh\necho tool ran\n"; }
  chmod((ldir + "/srctool").c_str(), 0755);
  {
    setMode(true);
    RCommand rc(HOST, "ssh", shell, user);
    string o;
    rc.exec("mkdir -p " + rdir + "/bin");
    string srcPath = ldir + "/srcfile", toolPath = ldir + "/srctool";
    const char* from[] = { srcPath.c_str(), 0 };
    rc.shellput(from, rdir);
    const char* tool[] = { toolPath.c_str(), 0 };
    rc.shellput(tool, rdir + "/bin");
    rc.exec("mv " + rdir + "/bin/srctool " + rdir + "/bin/ecce_src_tool");
  }

  struct { string file, prefix; } cases[] = {
    { rdir + "/srcfile", "" },
    { rdir + "/srcfile", "/opt/ecce_pfx" },
    { rdir + "/no_such_source_file", "" } };
  for (int c = 0; c < 3; c++) {
    vector<string> pty, ssh;
    cout << "source file " << cases[c].file << " shellPath=[" << cases[c].prefix
         << "]" << endl;
    if (!sourceScenario(false, user, shell, cases[c].file, cases[c].prefix, rdir, pty)) {
      check("sourceFile: pty session (oracle)", false);
      continue;
    }
    if (!sourceScenario(true, user, shell, cases[c].file, cases[c].prefix, rdir, ssh)) {
      check("sourceFile: ssh session", false);
      continue;
    }
    for (size_t i = 0; i < pty.size() && i < ssh.size(); i++) {
      if (pty[i] == ssh[i]) cout << "ok   " << pty[i] << endl;
      else {
        extra++;
        cout << "FAIL\n  pty " << pty[i] << "\n  ssh " << ssh[i] << endl;
      }
    }
  }
  {
    // The stream runs on a session of its own and must see the same
    // imported environment and directory as a command on the main one.
    setMode(true);
    RCommand rc(HOST, "ssh", shell, user, "", "", "", "", "", rdir + "/srcfile");
    const string cmd = "echo \"[$ECCE_T1][$ECCE_T_AFTER][$LOGNAME]\"; pwd; "
                       "ecce_src_tool";
    string viaExec;
    bool ok = rc.isOpen() && rc.execout(cmd, viaExec) && rc.startStream(cmd);
    check("sourceFile: stream starts", ok);
    if (ok) {
      int fd = rc.expfid();
      string got;
      for (int i = 0; i < 100 && got.find("tool ran") == string::npos; i++) {
        fd_set f; FD_ZERO(&f); FD_SET(fd, &f);
        struct timeval tv = { 0, 100000 };
        if (select(fd + 1, &f, 0, 0, &tv) > 0) {
          char b[4096];
          ssize_t n = read(fd, b, sizeof b);
          if (n <= 0) break;
          got.append(b, n);
        }
      }
      string want;
      for (size_t i = 0; i < viaExec.size(); i++)
        if (viaExec[i] != '\r') want += viaExec[i];
      check("sourceFile: stream sees the imported environment and directory: " +
            esc(got), got == want && got.find("[a b 'q'") != string::npos);
    }
    rc.stopStream();
  }
  {
    setMode(true);
    { ofstream f((ldir + "/srcbad").c_str()); f << "exit 3\n"; }
    RCommand rc0(HOST, "ssh", shell, user);
    string badPath = ldir + "/srcbad";
    const char* from[] = { badPath.c_str(), 0 };
    rc0.shellput(from, rdir);
    RCommand rc(HOST, "ssh", shell, user, "", "", "", "", "", rdir + "/srcbad");
    // csh's exit only ends the sourced file, as it does in the pty session.
    if (csh)
      check("csh: exit in the sourceFile only ends it", rc.isOpen());
    else
      check("sourceFile that exits fails the connection (" + rc.commError() + ")",
            !rc.isOpen() && rc.commError().find("srcbad") != string::npos);
  }
}

static bool hookCalled = false;
static bool acceptHook(const string&, const string&) { hookCalled = true; return true; }

// Password login with no key, then the host-key paths.  Uses "pwhost",
// which the setup gave a known_hosts line but no IdentityFile.
static void authChecks(const string& user, const string& shell)
{
  setMode(true);
  {
    RCommand rc("pwhost", "ssh", shell, user, "ecce-test");
    string o;
    check("password auth: open (" + rc.commError() + ")", rc.isOpen());
    check("password auth: runs commands",
          rc.execout("id -un", o) && o == user + "\r\n");
  }
  {
    RCommand rc("pwhost", "ssh", shell, user, "wrong-password");
    check("wrong password refused with a message",
          !rc.isOpen() && rc.commError() != "");
  }

  const char* home = getenv("HOME");
  string kh = string(home ? home : "") + "/.ssh/known_hosts";
  string keep = kh + ".keep";
  if (rename(kh.c_str(), keep.c_str()) != 0) {
    check("host key: known_hosts present to move aside", false);
    return;
  }
  {
    RCommand::hostKeyHook = 0;
    RCommand rc(HOST, "ssh", shell, user);
    check("unknown host key refused, tells the user to run ssh",
          !rc.isOpen() && rc.commError().find("ssh " + string(HOST)) != string::npos &&
          rc.commError().find("SHA256:") != string::npos);
  }
  // hostkeydialog stub in $ECCE_HOME/bin; DISPLAY only has to be set.
  {
    char tmpl[] = "/tmp/hkdXXXXXX";
    string hd = mkdtemp(tmpl);
    mkdir((hd + "/bin").c_str(), 0755);
    string stub = hd + "/bin/hostkeydialog", log = hd + "/args";
    const char* oldHome = getenv("ECCE_HOME");
    string saveHome = oldHome ? oldHome : "";
    const char* oldDisp = getenv("DISPLAY");
    string saveDisp = oldDisp ? oldDisp : "";
    setenv("ECCE_HOME", hd.c_str(), 1);
    setenv("DISPLAY", ":99", 1);
    RCommand::hostKeyHook = 0;

    { ofstream f(stub.c_str());
      f << "#!/bin/sh\necho \"$@\" > " << log << "\necho accept\n"; }
    chmod(stub.c_str(), 0755);
    {
      RCommand rc(HOST, "ssh", shell, user);
      check("dialog accepts: connected", rc.isOpen());
      string args = readFile(log);
      check("dialog got host, SHA256 fingerprint and key type",
            args.find(HOST) == 0 && args.find("SHA256:") != string::npos &&
            args.find("ED25519") != string::npos);
      check("dialog accept recorded in known_hosts",
            readFile(kh).find("127.0.0.1") != string::npos);
    }
    unlink(log.c_str());
    {
      RCommand rc(HOST, "ssh", shell, user);
      check("recorded key: no second prompt", rc.isOpen() && access(log.c_str(), F_OK) != 0);
    }
    unlink(kh.c_str());

    { ofstream f(stub.c_str()); f << "#!/bin/sh\nexit 1\n"; }
    {
      RCommand rc(HOST, "ssh", shell, user);
      check("dialog refuses: not connected, says not accepted",
            !rc.isOpen() && rc.commError().find("was not accepted") != string::npos &&
            rc.commError().find("SHA256:") != string::npos);
      check("dialog refuses: known_hosts unchanged", access(kh.c_str(), F_OK) != 0);
    }

    unlink(stub.c_str());
    {
      RCommand rc(HOST, "ssh", shell, user);
      check("no dialog program: tells the user to run ssh",
            !rc.isOpen() && rc.commError().find("Run \"ssh " + string(HOST)) != string::npos);
    }
    unsetenv("DISPLAY");
    { ofstream f(stub.c_str()); f << "#!/bin/sh\necho accept\n"; }
    chmod(stub.c_str(), 0755);
    {
      RCommand rc(HOST, "ssh", shell, user);
      check("no display: tells the user to run ssh",
            !rc.isOpen() && rc.commError().find("Run \"ssh " + string(HOST)) != string::npos);
    }
    unlink(stub.c_str()); rmdir((hd + "/bin").c_str());
    unlink(log.c_str()); rmdir(hd.c_str());
    if (oldHome) setenv("ECCE_HOME", saveHome.c_str(), 1); else unsetenv("ECCE_HOME");
    if (oldDisp) setenv("DISPLAY", saveDisp.c_str(), 1);
  }
  {
    RCommand::hostKeyHook = acceptHook;
    RCommand rc(HOST, "ssh", shell, user);
    check("unknown host key accepted by the hook", rc.isOpen() && hookCalled);
    RCommand::hostKeyHook = 0;
    check("accepted key recorded in known_hosts", readFile(kh).find("[127.0.0.1]:") == 0 ||
          readFile(kh).find("127.0.0.1") != string::npos);
  }
  {
    RCommand rc(HOST, "ssh", shell, user);
    check("recorded key is used without asking", rc.isOpen());
  }
  rename(keep.c_str(), kh.c_str());
}

int main()
{
  if (!getenv("ECCE_REALUSER")) {
    const char* u = getenv("USER");
    setenv("ECCE_REALUSER", u ? u : "root", 1);
  }
  setenv("ECCE_AUTHCACHE_NO_BROADCAST", "1", 1);
  const char* home = getenv("HOME");
  if (home && !getenv("ECCE_REALUSERHOME")) setenv("ECCE_REALUSERHOME", home, 1);
  if (!home || access((string(home) + "/.ssh/ecce_test_key").c_str(), R_OK) != 0) {
    cout << "SKIP: run through tests/transport/sshd/rcommand_test.sh" << endl;
    return 77;
  }

  char tmpl[] = "/tmp/testRCommandSshXXXXXX";
  string ldir = mkdtemp(tmpl);
  { ofstream f((ldir + "/lplain").c_str()); f << "local text\nline two\n"; }
  { ofstream f((ldir + "/lother").c_str()); f << "other\n"; }
  { ofstream f((ldir + "/lscript").c_str()); f << "#!/bin/sh\necho s\n"; }
  chmod((ldir + "/lscript").c_str(), 0755);
  mkdir((ldir + "/tree").c_str(), 0755);
  mkdir((ldir + "/tree/a").c_str(), 0755);
  { ofstream f((ldir + "/tree/a/file").c_str()); f << "deep\n"; }
  { ofstream f((ldir + "/tree/top").c_str()); f << "top\n"; }

  struct Acct { const char* user; const char* shell; } accts[] = {
    { "cshuser", "csh" }, { "bashuser", "bash" } };
  int failures = 0;

  for (int a = 0; a < 2; a++) {
    string user = accts[a].user, shell = accts[a].shell;
    cout << "== " << user << " (" << shell << ")" << endl;

    string rdir = "/tmp/ecce_rcssh_" + user;
    setMode(true);
    {
      RCommand rc(HOST, "ssh", shell, user);
      string o;
      if (!rc.isOpen()) {
        cout << "FAIL: cannot open ssh session: " << rc.commError() << endl;
        return 1;
      }
      rc.execout("rm -rf " + rdir + " && mkdir -p " + rdir + "/sub && "
                 "printf 'plain text\\nsecond line\\n' > " + rdir + "/plain && "
                 "printf '#!/bin/sh\\necho hi\\n' > " + rdir + "/script && "
                 "chmod 755 " + rdir + "/script && echo x > " + rdir + "/sub/inner", o);
    }

    vector<string> pty, ssh;
    bool ptyBg = false, sshBg = false;
    if (!scenario(false, user, shell, rdir, ldir, pty, ptyBg)) {
      cout << "FAIL: no pty session (oracle)" << endl;
      return 1;
    }
    if (!scenario(true, user, shell, rdir, ldir, ssh, sshBg)) {
      cout << "FAIL: no ssh session" << endl;
      return 1;
    }

    size_t n = pty.size() > ssh.size() ? pty.size() : ssh.size();
    for (size_t i = 0; i < n; i++) {
      string x = i < pty.size() ? pty[i] : "<missing>";
      string y = i < ssh.size() ? ssh[i] : "<missing>";
      if (x == y) cout << "ok   " << x << endl;
      else { failures++; cout << "FAIL\n  pty " << x << "\n  ssh " << y << endl; }
    }
    if (!ptyBg || !sshBg) {
      failures++;
      cout << "FAIL execbg live pid: pty=" << ptyBg << " ssh=" << sshBg << endl;
    }

    sshChecks(user, shell);
    streamChecks(user, shell);
    sourceChecks(user, shell, rdir, ldir);
    authChecks(user, shell);

    setMode(true);
    RCommand rc(HOST, "ssh", shell, user);
    string o;
    rc.execout("rm -rf " + rdir, o);
  }
  failures += extra;

  string cmd = "rm -rf " + ldir;
  if (system(cmd.c_str()) != 0) {}
  cout << (failures ? "FAILED " : "PASSED ") << failures << " difference(s)" << endl;
  return failures ? 1 : 0;
}
