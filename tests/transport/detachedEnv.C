// RCommand::execbg() runs its command as `nohup <command>` with no shell
// between, so a VAR=value prefix is taken for the program name. The job
// store started that way on Windows never ran (#1284). Through env the
// variable reaches the program, backslashes and all.

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#define getpid() ((int)GetCurrentProcessId())
#else
#include <unistd.h>
#endif

#include "comm/DirectTransport.H"
#include "comm/RCommand.H"

using std::string;

static int g_fail = 0;

static void check(bool ok, const string& what)
{
  std::cout << (ok ? "PASS  " : "FAIL  ") << what << std::endl;
  if (!ok) g_fail++;
}

// What the detached program saw in ECCE_TEST_VAR; "" if it never ran.
static string runDetached(const string& assignments, const string& out)
{
  remove(out.c_str());
  const string prog = "sh -c 'printf %s \"$ECCE_TEST_VAR\" > " + out + "'";
  DirectTransport t;
  string error;
  long pid = t.spawnDetached("nohup " + RCommand::withEnv(assignments, prog), error);
  if (pid < 0) return "";
  for (int i = 0; i < 100; i++) {
    std::ifstream f(out.c_str());
    if (f.good()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      std::stringstream ss;
      ss << std::ifstream(out.c_str()).rdbuf();
      return ss.str();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return "";
}

int main()
{
  const string value = "\\\\.\\pipe\\ecce-sshm-test";
  const string assign = "ECCE_TEST_VAR=" + RCommand::quotePath(value) + " ";
  const string out = "detached_env_" + std::to_string(getpid()) + ".out";

  check(RCommand::withEnv("", "prog") == "prog", "no assignments: no env");
  {
    // The command as it was: nohup takes the assignment for the program.
    remove(out.c_str());
    DirectTransport t;
    string error;
    t.spawnDetached("nohup " + assign + "sh -c 'echo x > " + out + "'", error);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    check(!std::ifstream(out.c_str()).good(),
          "without env the detached program does not run");
  }
  check(runDetached(assign, out) == value,
        "the variable reaches the detached program unchanged");
  remove(out.c_str());
  return g_fail ? 1 : 0;
}
