// RCommand on a local machine against results recorded from the old pty
// path (#204): the same operations through RCommand("system") must keep
// returning the same values and the same output, so callers need no change.
// execbg's PID is only checked for being a live process.
// golden/rcommand_direct.txt.

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

#include "comm/RCommand.H"
#include "golden.H"

using namespace std;

static string esc(const string& s)
{
  string r;
  for (size_t i = 0; i < s.size(); i++) {
    char c = s[i];
    if (c == '\r') r += "\\r";
    else if (c == '\n') r += "\\n";
    else r += c;
  }
  return r;
}

static string rec(const string& name, bool ret, const string& out,
                  const string& err = "")
{
  return name + ": ret=" + (ret ? "1" : "0") + " out=[" + esc(out) + "]" +
         (err.empty() ? "" : " err=[" + esc(err) + "]");
}

// Runs the whole scenario; returns one line per operation.  bgAlive is
// set when execbg produced a live process.
static bool scenario(const string& tmp, vector<string>& log, bool& bgAlive)
{
  RCommand rc("system", "", "bash");
  if (!rc.isOpen()) {
    cout << "no session: " << rc.commError() << endl;
    return false;
  }

  string o;
  bool r;
#define EXECOUT(name, cmd) \
  do { o = "untouched"; r = rc.execout(cmd, o); \
       log.push_back(rec(name, r, o, r ? "" : rc.commError())); } while (0)

  EXECOUT("echo", "echo hello");
  EXECOUT("multi-line", "echo a; echo b; echo c");
  EXECOUT("empty output", "true");
  EXECOUT("stderr merged", "echo out; echo err 1>&2; echo out2");
  // The output of these is not part of the contract, only the verdict.
#define EXECOUT_STATUS_ONLY(name, cmd) \
  do { r = rc.execout(cmd, o); \
       log.push_back(rec(name, r, "", rc.commError())); } while (0)

  EXECOUT("status 1", "(exit 1)");
  EXECOUT_STATUS_ONLY("status 3", "echo before; (exit 3)");
  EXECOUT_STATUS_ONLY("status 127", "no_such_command_ecce");
  EXECOUT("status 2", "(exit 2)");

  r = rc.exec("echo quiet");
  log.push_back(rec("exec ok", r, ""));
  r = rc.exec("(exit 1)", "custom failure text");
  log.push_back(rec("exec fail", r, "", rc.commError()));

  r = rc.cd(tmp);
  log.push_back(rec("cd tmp", r, "", r ? "" : rc.commError()));
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

  o = "";
  r = rc.execbg("sleep 20", o);
  long pid = atol(o.c_str());
  bgAlive = r && pid > 1 && kill((pid_t)pid, 0) == 0;
  log.push_back(rec("execbg", r, bgAlive ? "live pid" : "no live pid [" + o + "]"));
  if (bgAlive) kill((pid_t)pid, SIGKILL);

  // Hand-off files in and out.
  string dest = tmp + "/copies";
  mkdir(dest.c_str(), 0755);
  const char* from[] = { (tmp + "/plain").c_str(), 0 };
  string fromPath = tmp + "/plain";
  from[0] = fromPath.c_str();
  r = rc.shellput(from, dest);
  log.push_back(rec("shellput", r, "", r ? "" : rc.commError()));
  {
    ifstream in((dest + "/plain").c_str());
    string c((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    log.push_back(rec("shellput content", true, c));
  }
  unlink((dest + "/plain").c_str());
  vector<string> src;
  src.push_back(tmp + "/plain");
  r = rc.shellget(src, dest);
  log.push_back(rec("shellget", r, "", r ? "" : rc.commError()));
  {
    ifstream in((dest + "/plain").c_str());
    string c((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    log.push_back(rec("shellget content", true, c));
  }
  src.clear();
  src.push_back(tmp + "/missing");
  r = rc.shellget(src, dest);
  log.push_back(rec("shellget missing", r, ""));

  return true;
}

static int extra = 0;

static void check(const char* name, bool ok)
{
  cout << (ok ? "ok   " : "FAIL ") << name << endl;
  if (!ok) extra++;
}

static string slurp(const string& f);

// A local machine's own behaviour.
static void directChecks()
{
  {
    // An 8.x eccejobmaster exports ECCE_TRANSPORT=pty to its children: the
    // value is noted on stderr, never an error, and the default is used.
    setenv("ECCE_TRANSPORT", "pty", 1);
    fflush(stderr);
    string logName = "/tmp/testRCommandDirectErrXXXXXX";
    int fd = mkstemp(&logName[0]);
    int saved = dup(2);
    dup2(fd, 2);
    string o;
    bool ok;
    {
      RCommand rc("system", "", "bash");
      ok = rc.isOpen() && rc.execout("echo ok", o) && o == "ok\r\n" &&
           rc.streamFd() == -1 && rc.sshBackend() == "";
    }
    fflush(stderr);
    dup2(saved, 2);
    close(saved);
    close(fd);
    string said = slurp(logName);
    unlink(logName.c_str());
    unsetenv("ECCE_TRANSPORT");
    check("ECCE_TRANSPORT=pty: the default transport runs the command", ok);
    check("ECCE_TRANSPORT=pty: noted in one line",
          said == "ECCE_TRANSPORT=pty is no longer supported; using the "
                  "default transport\n");
    setenv("ECCE_TRANSPORT", "ssh", 1);
    {
      RCommand rc("system", "", "bash");
      check("ECCE_TRANSPORT=ssh is ignored", rc.isOpen() && rc.exec("true"));
    }
    unsetenv("ECCE_TRANSPORT");
  }
  {
    RCommand rc("system", "", "bash", "", "", "", "", "/opt/ecce_test_path",
                "/opt/ecce_test_lib");
    string p, l;
    check("local: open", rc.isOpen());
    rc.execout("echo $PATH", p);
    rc.execout("echo $LD_LIBRARY_PATH", l);
    check("local: shellPath prefixed",
          p.compare(0, 20, "/opt/ecce_test_path:") == 0);
    check("local: libPath prefixed",
          l.compare(0, 19, "/opt/ecce_test_lib:") == 0);
    check("local: hop refused",
          !rc.hop("elsewhere") &&
          rc.commError() == "hop is only available on an ssh connection");
    check("local: no stream open", rc.streamFd() == -1 &&
          !rc.streamWrite("date") && rc.commError() != "");
    check("local: still usable", rc.exec("true"));
  }
  {
    // This machine is never reached through a front end, so the front end
    // is ignored.
    RCommand rc("system", "", "bash", "", "", "frontend.invalid", "");
    string o;
    check("local machine with a front end runs locally",
          rc.isOpen() && rc.sshBackend() == "" && rc.execout("echo here", o) &&
          o == "here\r\n");
  }
  {
    RCommand rc("system", "", "bash", "", "", "", "", "", "", "/nonexistent");
    check("a missing sourceFile stays local", rc.isOpen());
  }
  {
    string out, err;
    bool ok = RCommand::command("echo via_command", out, err, "system", "",
                                "bash");
    check("static command() works",
          ok && out == "via_command\r\n");
  }
}

static void put(const string& f, const string& content)
{
  ofstream(f.c_str()) << content;
}


// Local get/put: one `cp -r` per call, "No such file" only a warning.
static string slurp(const string& f)
{
  ifstream in(f.c_str());
  return string((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
}

static bool exists(const string& f)
{
  struct stat sb;
  return stat(f.c_str(), &sb) == 0;
}

static void copyChecks(const string& tmp)
{
  const string src = tmp + "/csrc", dst = tmp + "/cdst";
  mkdir(src.c_str(), 0755);
  mkdir((src + "/tree").c_str(), 0755);
  put(src + "/a1.txt", "one\n");
  put(src + "/a2.txt", "two\n");
  put(src + "/b.dat", "bee\n");
  put(src + "/tree/leaf", "leaf\n");
  string err;
  bool r;

  mkdir(dst.c_str(), 0755);
  err = "";
  r = RCommand::put(err, "system", "", "", "", 2, (src + "/*.txt").c_str(),
                    dst.c_str());
  check("copy: glob put ok", r && err == "");
  check("copy: glob put matched .txt only",
        slurp(dst + "/a1.txt") == "one\n" && slurp(dst + "/a2.txt") == "two\n" &&
        !exists(dst + "/b.dat"));

  err = "";
  r = RCommand::get(err, "system", "", "", "", 2, (src + "/b.dat").c_str(),
                    dst.c_str());
  check("copy: get one file", r && slurp(dst + "/b.dat") == "bee\n");

  err = "";
  r = RCommand::put(err, "system", "", "", "", 2, (src + "/tree").c_str(),
                    dst.c_str());
  check("copy: directory source is recursive",
        r && slurp(dst + "/tree/leaf") == "leaf\n");

  vector<string> two;
  two.push_back(src + "/a1.txt");
  two.push_back(src + "/missing");
  two.push_back(src + "/a2.txt");
  mkdir((tmp + "/cdst2").c_str(), 0755);
  err = "";
  r = RCommand::get(err, "system", "", "", "", two, tmp + "/cdst2");
  check("copy: a missing name among others is skipped",
        r && slurp(tmp + "/cdst2/a1.txt") == "one\n" &&
        slurp(tmp + "/cdst2/a2.txt") == "two\n");

  // A name that matches nothing is dropped by the glob, so cp gets no
  // source and says so; that is a failure, not the "No such file" warning.
  err = "";
  r = RCommand::get(err, "system", "", "", "", 2, (src + "/missing").c_str(),
                    dst.c_str());
  check("copy: only a missing name fails",
        !r && err.find("Copy command cp failed") == 0 &&
        err.find("missing destination file operand") != string::npos);
  err = "";
  r = RCommand::get(err, "system", "", "", "", 2, (src + "/nomatch*").c_str(),
                    dst.c_str());
  check("copy: an unmatched glob fails the same way",
        !r && err.find("missing destination file operand") != string::npos);
  err = "";
  r = RCommand::put(err, "system", "", "", "", 3, (src + "/a1.txt").c_str(),
                    (src + "/a2.txt").c_str(), (src + "/b.dat").c_str());
  check("copy: several files to a file fails",
        !r && err.find("Not a directory") != string::npos);
  err = "";
  r = RCommand::put(err, "system", "", "", "", 2, (src + "/a1.txt").c_str(),
                    (tmp + "/nodir/x").c_str());
  check("copy: a missing target directory is only a warning",
        r && err == "" && !exists(tmp + "/nodir"));
  err = "";
  r = RCommand::put(err, "system", "", "", "", 2, (src + "/a1.txt").c_str(),
                    (tmp + "/newname").c_str());
  check("copy: single file to a new name",
        r && slurp(tmp + "/newname") == "one\n");
  put(src + "/it's a name", "q\n");
  err = "";
  r = RCommand::put(err, "system", "", "", "", 2, (src + "/it's a name").c_str(),
                    dst.c_str());
  check("copy: a quote in a name", r && exists(dst + "/it's a name"));
  err = "";
  r = RCommand::put(err, "system", "", "", "", 2, (src + "/a*").c_str(),
                    "relative_dst_ecce_nosuch");
  check("copy: several files to a missing target is only a warning",
        r && err == "" && !exists("relative_dst_ecce_nosuch"));
  if (geteuid() != 0) {
    chmod(dst.c_str(), 0555);
    err = "";
    r = RCommand::put(err, "system", "", "", "", 2, (src + "/a1.txt").c_str(),
                      (dst + "/ro").c_str());
    check("copy: a refused write fails",
          !r && err.find("Permission denied") != string::npos);
    chmod(dst.c_str(), 0755);
  }
}


// Paths with a space or a quote, as Windows user folders have (#247): the
// file operations and cd take a path, not shell syntax.
static void spaceChecks(const string& tmp)
{
  const string d = tmp + "/a dir/it's here";
  mkdir((tmp + "/a dir").c_str(), 0755);
  mkdir(d.c_str(), 0755);
  put(d + "/x 1.out", "x\n");
  put(d + "/y.out", "y\n");
  RCommand rc("system", "", "bash");
  check("space: directory()", rc.directory(d));
  check("space: writable()", rc.writable(d));
  check("space: exists() of a file", rc.exists(d + "/x 1.out"));
  check("space: no word splitting", !rc.exists(tmp + "/a"));
  string o;
  check("space: cd() and pwd", rc.cd(d) && rc.execout("pwd", o) &&
        o == d + "\r\n");
  check("space: relative exists after cd", rc.exists("x 1.out"));
  check("space: home", rc.directory("~") && rc.directory("~/"));
  check("quotePath keeps ~", RCommand::quotePath("~") == "~" &&
        RCommand::quotePath("~/a b") == "~/'a b'");
  check("quoteGlob", RCommand::quoteGlob("/p q/*.out") == "'/p q/'*'.out'" &&
        RCommand::quoteGlob("a b") == "'a b'");
  const string dst = tmp + "/space dst";
  mkdir(dst.c_str(), 0755);
  vector<string> src;
  src.push_back(d + "/*.out");
  check("space: shellget of a glob in a spaced folder",
        rc.shellget(src, dst) && exists(dst + "/x 1.out") &&
        exists(dst + "/y.out"));
}

int main()
{
  // RCommand asks for the login it runs as.
  if (!getenv("ECCE_REALUSER")) {
    const char* u = getenv("USER");
    setenv("ECCE_REALUSER", u ? u : "nobody", 1);
  }

  char tmpl[] = "/tmp/testRCommandDirectXXXXXX";
  string tmp = mkdtemp(tmpl);
  put(tmp + "/plain", "plain text\nsecond line\n");
  put(tmp + "/script", "#!/bin/sh\necho hi\n");
  chmod((tmp + "/script").c_str(), 0755);
  mkdir((tmp + "/sub").c_str(), 0755);
  put(tmp + "/sub/inner", "x\n");

  vector<string> dir;
  bool bg = false;
  golden::Subs subs;
  subs.push_back(make_pair(tmp, string("@TMP@")));
  {
    // `which ./script` answers with a system directory, whichever the
    // distribution keeps sh in.
    subs.push_back(make_pair(string("/usr/bin/./script"), string("@BIN@/./script")));
    subs.push_back(make_pair(string("/bin/./script"), string("@BIN@/./script")));
    subs.push_back(make_pair(string("/usr/bin/sh"), string("@SH@")));
    subs.push_back(make_pair(string("/bin/sh"), string("@SH@")));
  }

  if (!scenario(tmp, dir, bg)) {
    cout << "FAIL: no local session" << endl;
    return 1;
  }

  int failures = golden::compare("rcommand_direct", dir, subs);
  if (!bg) {
    failures++;
    cout << "FAIL execbg live pid" << endl;
  }

  directChecks();
  copyChecks(tmp);
  spaceChecks(tmp);
  failures += extra;

  string cmd = "rm -rf " + tmp;
  if (system(cmd.c_str()) != 0) {}
  cout << (failures ? "FAILED " : "PASSED ") << failures << " difference(s)"
       << endl;
  return failures ? 1 : 0;
}
