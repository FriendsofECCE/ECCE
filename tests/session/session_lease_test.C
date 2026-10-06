// Session leases (#233, SessionLease.H): a program of $ECCE_HOME/bin with a
// session id holds a lease while it runs, and the C++ and shell readers
// (SessionLease::live, ecce_session_leases) both see it, and both forget
// it once the program has ended, however it ended.
//
//   session_lease_test <path to ecce-session-lib.sh>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "util/Ecce.H"
#include "util/SessionLease.H"

using std::cout;
using std::endl;
using std::string;

static int g_fail = 0;
static string g_lib, g_tmp;

static void check(bool ok, const string& what)
{
  cout << (ok ? "PASS  " : "FAIL  ") << what << endl;
  if (!ok) g_fail++;
}

static string shell(const string& snippet)
{
  string cmd = "bash -c 'STATEDIR=\"$ECCE_REALUSERHOME/.ECCE\"; . \"$0\" && " +
               snippet + "' '" + g_lib + "'";
  FILE* p = popen(cmd.c_str(), "r");
  if (!p) return "<popen failed>";
  string out;
  char buf[256];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), p)) > 0) out.append(buf, n);
  pclose(p);
  return out;
}

static bool exists(const string& path)
{
  struct stat st;
  return stat(path.c_str(), &st) == 0;
}

// Started as $ECCE_HOME/bin/<this binary>: takes the lease (as every ECCE
// program does when util starts), says so, and waits to be killed.
static int holderMain()
{
  SessionLease::acquire();
  cout << "held" << endl;
  pause();
  return 0;
}

int main(int argc, char** argv)
{
  if (argc > 1 && string(argv[1]) == "hold") return holderMain();
  if (argc < 2) {
    cout << "usage: session_lease_test <ecce-session-lib.sh>" << endl;
    return 2;
  }
  g_lib = argv[1];
  const char* t = getenv("TMPDIR");
  string base = string(t && *t ? t : "/tmp") + "/ecce-lease-XXXXXX";
  std::vector<char> buf(base.begin(), base.end());
  buf.push_back('\0');
  if (!mkdtemp(buf.data())) return 2;
  g_tmp = buf.data();
  string home = g_tmp + "/home", ecceHome = g_tmp + "/ecce";
  mkdir(home.c_str(), 0700);
  mkdir((home + "/.ECCE").c_str(), 0700);
  mkdir(ecceHome.c_str(), 0700);
  mkdir((ecceHome + "/bin").c_str(), 0700);
  char self[4096];
  ssize_t n = readlink("/proc/self/exe", self, sizeof(self) - 1);
  if (n <= 0) return 2;
  self[n] = '\0';
  // Named as the binary it links to, as an overlay bin/ of a build tree is.
  string name = string(self).substr(string(self).rfind('/') + 1);
  string holder = ecceHome + "/bin/" + name;
  if (symlink(self, holder.c_str()) != 0) return 2;

  const char* id = "0123456789abcdef";
  setenv("ECCE_HOME", ecceHome.c_str(), 1);
  setenv("ECCE_REALUSERHOME", home.c_str(), 1);
  setenv("HOST", "leasehost", 1);
  unsetenv("ECCE_HOST");
  setenv("ECCE_SESSION_ID", id, 1);
  unsetenv("ECCE_SESSION_LIVENESS");
  string key = Ecce::sessionKey();
  string statedir = home + "/.ECCE";

  // The reader in this process holds no lease: this test binary is not run
  // from $ECCE_HOME/bin.
  check(SessionLease::live(statedir, key).empty(), "no lease to begin with");

  int pipefd[2];
  if (pipe(pipefd) != 0) return 2;
  pid_t child = fork();
  if (child == 0) {
    dup2(pipefd[1], 1);
    close(pipefd[0]);
    execl(holder.c_str(), name.c_str(), "hold", (char*)0);
    _exit(127);
  }
  close(pipefd[1]);
  char said[16] = {0};
  ssize_t got = read(pipefd[0], said, sizeof(said) - 1);
  check(got > 0 && string(said).find("held") == 0,
        "a program of $ECCE_HOME/bin took its lease");

  std::vector<SessionLease::Holder> held = SessionLease::live(statedir, key);
  check(held.size() == 1 && held[0].name == name &&
            held[0].pid == (long)child,
        "C++ reader: one lease, " + name + "." + std::to_string(child));
  check(SessionLease::live(statedir, "").size() == 1,
        "C++ reader over every session of this host finds it too");
  string sh = shell("ecce_session_leases");
  check(sh == std::to_string(child) + " " + name + " " + id + "\n",
        "shell reader: '" + std::to_string(child) + " " + name + " " + id +
            "' (got '" + sh.substr(0, sh.find('\n')) + "')");
  check(shell("ECCE_SESSION_LIVENESS=lease ecce_session_alive " +
              string(id) + " && echo yes") == "yes\n",
        "ecce_session_alive by lease alone");

  string leaseFile = statedir + "/leases/" + key + "/" + name + "." +
                     std::to_string(child);
  kill(child, SIGKILL);
  waitpid(child, 0, 0);
  check(exists(leaseFile), "the lease file is left behind by a SIGKILL");
  check(SessionLease::live(statedir, key).empty(),
        "C++ reader: the killed program's lease is stale");
  check(!exists(leaseFile), "and the stale lease file was removed");

  // A stale file found by the shell reader.
  string dir = statedir + "/leases/" + key;
  mkdir((statedir + "/leases").c_str(), 0700);
  mkdir(dir.c_str(), 0700);
  { std::ofstream f((dir + "/ghost.1").c_str()); }
  check(shell("ecce_session_leases").empty(),
        "shell reader: a lease nobody holds is not alive");
  check(!exists(dir + "/ghost.1") && !exists(dir),
        "and it and its empty directory were removed");
  check(shell("ECCE_SESSION_LIVENESS=lease ecce_session_alive " +
              string(id) + " || echo no") == "no\n",
        "ecce_session_alive is false once the program has gone");

  string rm = "rm -rf '" + g_tmp + "'";
  if (system(rm.c_str()) != 0) cout << "(could not remove " << g_tmp << ")" << endl;
  cout << (g_fail ? "FAILED" : "ALL PASS") << endl;
  return g_fail ? 1 : 0;
}
