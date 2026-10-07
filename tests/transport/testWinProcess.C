// DirectTransport process launch on Windows (#133): exit status, working
// directory, environment, stdout/stderr, idle timeout, argument quoting for
// paths with spaces, background start with a pid and cancel of the whole
// tree.  Needs an sh.exe (MSYS2 or Git for Windows).  On other systems it
// runs the same cases against /bin/sh, minus the Windows-only ones.
// "testWinProcess --argv a b ..." prints its arguments one per line (the
// child used for the quoting check).  Exit 0 only if every case passes.

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "comm/DirectTransport.H"

static int failures = 0;

static void check(const char* name, bool ok, const std::string& why = "")
{
  std::cout << (ok ? "ok   " : "FAIL ") << name;
  if (!ok && !why.empty()) std::cout << "  (" << why << ")";
  std::cout << std::endl;
  if (!ok) failures++;
}

static double now()
{
  return std::chrono::duration<double>(
    std::chrono::steady_clock::now().time_since_epoch()).count();
}

static std::string slurp(const std::string& f)
{
  std::ifstream in(f.c_str(), std::ios::binary);
  std::string s, l;
  while (std::getline(in, l)) {
    if (!l.empty() && l.back() == '\r') l.pop_back();
    s += l + "\n";
  }
  return s;
}

static bool alive(long pid)
{
#ifdef _WIN32
  // The job id is an MSYS pid, not a Windows one: ask the bundled ps.
  std::vector<std::string> a;
  a.push_back("ps");
  a.push_back("-p");
  a.push_back(std::to_string(pid));
  TransportResult r = DirectTransport::runProcess(a, "", -1, -1, 30);
  return r.status == 0;
#else
  return kill((pid_t)pid, 0) == 0;
#endif
}

static void sleepMs(int ms)
{
#ifdef _WIN32
  Sleep(ms);
#else
  usleep(ms * 1000);
#endif
}

static std::string trim(std::string s)
{
  while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
  return s;
}

int main(int argc, char** argv)
{
  if (argc > 1 && std::string(argv[1]) == "--argv") {
    for (int i = 2; i < argc; i++) std::cout << argv[i] << "\n";
    return 0;
  }

  DirectTransport t;
#ifdef _WIN32
  char tb[MAX_PATH];
  GetTempPathA(MAX_PATH, tb);
  std::string tmp = std::string(tb) + "testWinProcess dir " +
                    std::to_string(GetCurrentProcessId());
  _mkdir(tmp.c_str());
  for (size_t i = 0; i < tmp.size(); i++) if (tmp[i] == '\\') tmp[i] = '/';
#else
  char tmpl[] = "/tmp/testWinProcessXXXXXX";
  std::string tmp = mkdtemp(tmpl);
#endif

  {
    TransportResult r0 = t.run("exit 0"), r3 = t.run("exit 3");
    check("exit status", r0.status == 0 && r3.status == 3 && r0.error.empty(),
          std::to_string(r0.status) + "," + std::to_string(r3.status) + " " + r0.error);
  }
  {
    TransportResult r = t.run("echo out; echo err >&2");
    check("stdout/stderr separate",
          trim(r.out) == "out" && trim(r.err) == "err", r.out + "|" + r.err);
  }
  {
    TransportResult r = t.run("head -c 4194304 /dev/zero & head -c 4194304 /dev/zero >&2; wait", 60);
    check("4 MB on both streams", r.status == 0 && r.out.size() == 4194304 &&
          r.err.size() == 4194304 && !r.timedOut,
          std::to_string(r.out.size()) + "," + std::to_string(r.err.size()) + " " + r.error);
  }
  {
    DirectTransport d;
    d.setDir(tmp);
    TransportResult r = d.run("echo hi > made.txt");
    check("cwd from setDir", r.status == 0 && slurp(tmp + "/made.txt") == "hi\n", r.err);
    d.setDir(tmp + "/missing");
    r = d.run("echo unreachable");
    check("missing dir is status 97", r.status == 97 && r.out.empty(),
          std::to_string(r.status));
  }
  {
    DirectTransport d;
    d.setEnv("ECCE_TEST_VAR", "a b 'c' \"d\"");
    d.setEnv("ECCE_TEST_UNI", "caf\xc3\xa9");
    TransportResult r = d.run("printf '%s|%s' \"$ECCE_TEST_VAR\" \"$ECCE_TEST_UNI\"");
    check("env set", r.status == 0 && r.out == "a b 'c' \"d\"|caf\xc3\xa9", r.out + r.err);
    d.unsetEnv("PATH");
    d.setEnv("ECCE_PLUS", "x");
    r = d.run("echo ${ECCE_PLUS}");
    check("env unset leaves set", trim(r.out) == "x", r.out + r.err);
  }
  {
    double t0 = now();
    TransportResult r = t.run("sleep 30", 1);
    double dt = now() - t0;
    check("idle timeout kills", r.timedOut && dt < 8, std::to_string(dt) + " " + r.error);
    t0 = now();
    r = t.run("echo a; sleep 2; echo b; sleep 2; echo c", 3);
    check("output restarts the timeout", !r.timedOut && r.status == 0 && now() - t0 > 3.5,
          r.error);
  }
  {
    TransportResult r = t.run("cat", 10);
    check("script stdin is empty", r.status == 0 && r.out.empty() && !r.timedOut);
    std::vector<std::string> a;
    a.push_back(argv[0]);
    a.push_back("--argv");
    a.push_back("plain");
    a.push_back("with space");
    a.push_back("say \"hi\"");
    a.push_back("trailing\\");
    a.push_back("trailing space\\");
    a.push_back("back\\\\\"slash");
    a.push_back("");
    r = DirectTransport::runProcess(a, "", -1, -1, 30);
    std::string want = "plain\nwith space\nsay \"hi\"\ntrailing\\\ntrailing space\\\n"
                       "back\\\\\"slash\n\n";
    std::string got;
    for (size_t i = 0; i < r.out.size(); i++) if (r.out[i] != '\r') got += r.out[i];
    check("runProcess argument quoting", r.status == 0 && got == want,
          r.error + "|" + r.out);
#ifdef _WIN32
    // The program itself in a directory whose name has spaces.
    std::string copy = tmp + "/prog with space.exe";
    CopyFileA(argv[0], copy.c_str(), FALSE);
    std::vector<std::string> b(1, copy);
    b.push_back("--argv");
    b.push_back("x y");
    r = DirectTransport::runProcess(b, "", -1, -1, 30);
    check("runProcess path with spaces", r.status == 0 && trim(r.out) == "x y",
          r.error + "|" + r.out);
#endif
  }
  {
    std::string marker = tmp + "/bg.txt", err;
    double t0 = now();
    long pid = t.spawnDetached("sleep 1; echo done > '" + marker + "'", err);
    check("spawnDetached returns fast with a pid", pid > 0 && now() - t0 < 1.0, err);
    check("spawnDetached running", pid > 0 && alive(pid));
    for (int i = 0; i < 100 && slurp(marker).empty(); i++) sleepMs(100);
    check("spawnDetached finishes", slurp(marker) == "done\n");
    for (int i = 0; i < 50 && pid > 0 && alive(pid); i++) sleepMs(100);
    check("spawnDetached exits", pid > 0 && !alive(pid));

    std::string log = tmp + "/bg.log";
    pid = t.spawnDetached("echo to-log; echo to-err >&2", err, log);
    for (int i = 0; i < 50 && slurp(log).size() < 14; i++) sleepMs(100);
    std::string l = slurp(log);
    check("spawnDetached log gets both streams",
          l.find("to-log") != std::string::npos && l.find("to-err") != std::string::npos, l);
  }
#ifdef _WIN32
  {
    std::string err, pidf = tmp + "/child.pid";
    // sh starts a sleeping grandchild and records its pid, then waits.
    long pid = t.spawnDetached("sleep 300 & echo $! > '" + pidf + "'; wait", err);
    for (int i = 0; i < 50 && slurp(pidf).empty(); i++) sleepMs(100);
    check("tree: root and child running", pid > 0 && alive(pid) && !slurp(pidf).empty(), err);
    // cygwin's $! is not a Windows pid; count live sleep.exe descendants instead
    bool killed = DirectTransport::killTree(pid);
    for (int i = 0; i < 50 && alive(pid); i++) sleepMs(100);
    check("cancel kills the root", killed && !alive(pid));
    TransportResult r = t.run("ps -W 2>/dev/null | grep -c 'sleep.exe' ; true");
    check("cancel leaves no sleep", trim(r.out) == "0" || trim(r.out).empty(), r.out + r.err);
  }
#endif
  {
    DirectTransport::Stream s;
    std::string err;
    bool ok = t.openStream("while read l; do echo got:$l; done", s, err);
    check("stream opens", ok, err);
    if (ok) {
      t.writeStream(s, "one\n");
      char buf[64];
      int n = 0;
#ifdef _WIN32
      n = _read(s.rfd, buf, sizeof buf);
#else
      n = read(s.rfd, buf, sizeof buf);
#endif
      check("stream round trip", n > 0 && std::string(buf, n).find("got:one") == 0,
            std::string(buf, n > 0 ? n : 0));
      int st = t.closeStream(s, 5000);
      check("stream closes with status 0", st == 0, std::to_string(st));
    }
  }

  std::cout << (failures ? "FAILED" : "all passed") << std::endl;
  return failures ? 1 : 0;
}
