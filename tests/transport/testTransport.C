// Exercises DirectTransport against a real /bin/sh: exit codes, large
// output on both streams, large scripts, env/dir handling, timeouts,
// detached spawn, fd hygiene and SIGPIPE.  Exit 0 only if every case passes.

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

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

static bool alive(long pid) { return kill((pid_t)pid, 0) == 0; }

static std::string slurp(const std::string& f)
{
  std::ifstream in(f.c_str());
  std::string s, l;
  while (std::getline(in, l)) s += l;
  return s;
}

int main()
{
  DirectTransport t;
  char tmpl[] = "/tmp/testTransportXXXXXX";
  std::string tmp = mkdtemp(tmpl);

  {
    TransportResult r0 = t.run("exit 0"), r3 = t.run("exit 3"),
                    rk = t.run("kill -TERM $$");
    check("exit status", r0.status == 0 && r3.status == 3 && rk.status == 143,
          std::to_string(r0.status) + "," + std::to_string(r3.status) + "," +
          std::to_string(rk.status));
  }
  {
    TransportResult r = t.run("echo out; echo err >&2");
    check("stdout/stderr separate", r.out == "out\n" && r.err == "err\n");
  }
  {
    TransportResult r = t.run(
      "head -c 4194304 /dev/zero & head -c 4194304 /dev/zero >&2; wait", 60);
    check("4 MB on both streams", r.status == 0 && r.out.size() == 4194304 &&
          r.err.size() == 4194304 && !r.timedOut);
  }
  {
    std::string s = "head -c 1048576 /dev/zero\n";
    while (s.size() < 1048576) s += "# padding padding padding padding padding\n";
    s += "echo done\n";
    TransportResult r = t.run(s, 60);
    check("1 MB script, output first", r.status == 0 && !r.timedOut &&
          r.out.size() == 1048576 + 5);
  }
  {
    DirectTransport d;
    d.setDir("/tmp");
    TransportResult r = d.run("pwd");
    d.setDir("/nonexistent/it's here");
    TransportResult m = d.run("echo unreachable");
    check("setDir", r.out == "/tmp\n", r.out);
    check("missing dir", m.status == 97 && !m.err.empty() && m.out.empty(),
          std::to_string(m.status) + " " + m.err);
    d.setDir(tmp + "/it's a dir");
    mkdir((tmp + "/it's a dir").c_str(), 0700);
    TransportResult q = d.run("pwd");
    check("dir with quote", q.out == tmp + "/it's a dir\n", q.out);
    rmdir((tmp + "/it's a dir").c_str());
  }
  {
    DirectTransport d;
    std::string v = "a b 'c' \"d\" $HOME ! \\n `x`\nsecond line";
    d.setEnv("TT_VAL", v);
    setenv("TT_INHERITED", "yes", 1);
    d.unsetEnv("TT_INHERITED");
    TransportResult r = d.run("printf %s \"$TT_VAL\"");
    check("env exact", r.out == v, r.out);
    TransportResult u = d.run("echo \"${TT_INHERITED+set}\"");
    check("unsetEnv", u.out == "\n", u.out);
  }
  {
    std::string s =
      "echo 'a!b'; echo \"`echo hi`\"; echo x >&2 2>&1\n"
      "cat <<'EOT'\nline ! `not run` $nothing\nEOT\n";
    TransportResult r = t.run(s);
    check("csh-hostile script", r.status == 0 &&
          r.out == "a!b\nhi\nline ! `not run` $nothing\n", r.out);
  }
  {
    std::string pf = tmp + "/pid";
    double t0 = now();
    TransportResult r = t.run("sleep 30 & echo $! > '" + pf + "'; sleep 30; wait", 1);
    double dt = now() - t0;
    long bg = atol(slurp(pf).c_str());
    usleep(200000);
    check("timeout", r.timedOut && dt < 3 && r.status > 128 && !r.error.empty(),
          std::to_string(dt) + " status " + std::to_string(r.status));
    // Zombie-free: the sleep was reparented and reaped by init, or is gone.
    check("timeout kills group", bg > 0 && !alive(bg), std::to_string(bg));
  }
  {
    std::string marker = tmp + "/marker", err;
    double t0 = now();
    long pid = t.spawnDetached("sleep 1; echo done > '" + marker + "'", err);
    double dt = now() - t0;
    check("spawnDetached returns fast", pid > 0 && dt < 0.5, err);
    check("spawnDetached alive, own session",
          pid > 0 && alive(pid) && getsid((pid_t)pid) != getsid(0));
    sleep(2);
    check("spawnDetached ran script", slurp(marker) == "done");
    std::string log = tmp + "/log";
    long p2 = t.spawnDetached("echo hello; echo oops >&2", err, log);
    sleep(1);
    check("spawnDetached logFile", p2 > 0 && slurp(log) == "hellooops", slurp(log));
  }
  {
    int fd = open("/dev/null", O_RDONLY);
    int leak = fcntl(fd, F_DUPFD, 100);   // no CLOEXEC
    close(fd);
    TransportResult r = t.run("ls /proc/self/fd");
    bool seen = false;
    std::string cur;
    for (size_t i = 0; i <= r.out.size(); i++) {
      if (i == r.out.size() || r.out[i] == '\n') {
        if (cur == std::to_string(leak)) seen = true;
        cur.clear();
      } else cur += r.out[i];
    }
    check("fd hygiene", !seen && r.status == 0, r.out);
    close(leak);
  }
  {
    std::string s = "exit 0\n";
    while (s.size() < 1048576) s += "# padding padding padding padding padding\n";
    for (int i = 0; i < 3; i++) {
      TransportResult r = t.run(s);
      if (r.status != 0) { check("SIGPIPE", false); break; }
      if (i == 2) check("SIGPIPE", true);
    }
    sigset_t pend;
    sigpending(&pend);
    check("no SIGPIPE left pending", sigismember(&pend, SIGPIPE) == 0);
  }

  system(("rm -rf '" + tmp + "'").c_str());
  std::cout << (failures ? "FAILED" : "all passed") << std::endl;
  return failures ? 1 : 0;
}
