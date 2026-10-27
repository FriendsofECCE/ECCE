// AuthCache's -pipe handoff between two processes (#605): the parent's
// pipeOut() and the child's pipeIn() must hand over the cache whichever
// starts first, and pipeIn() must return when no writer ever comes (it
// blocked in fopen() for good, which hung the Viewer on a Mac).
//
//   authcache_pipe_test

#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "tdat/AuthCache.H"

using namespace std;

static int g_fail = 0;
static void check(bool ok, const string& what)
{
  cout << (ok ? "PASS  " : "FAIL  ") << what << endl;
  if (!ok) g_fail++;
}

// Child: read the pipe, exit with the number of entries it got.
static pid_t reader(const string& pipe)
{
  pid_t pid = fork();
  if (pid == 0) {
    AuthCache::getCache().pipeIn(pipe);
    _exit(AuthCache::getCache().size());
  }
  return pid;
}

static pid_t writer(const string& pipe)
{
  pid_t pid = fork();
  if (pid == 0) {
    AuthCache::getCache().addAuthentication("http://localhost:1/Ecce/",
                                            "alice", "dummy", "", false);
    AuthCache::getCache().pipeOut(pipe);
    _exit(0);
  }
  return pid;
}

// The child's exit status, or -1 if it is still running after `limit` s.
static int waitFor(pid_t pid, int limit, double *took)
{
  time_t start = time(0);
  int status = 0;
  while (waitpid(pid, &status, WNOHANG) == 0) {
    if (time(0) - start > limit) {
      kill(pid, SIGKILL);
      waitpid(pid, &status, 0);
      return -1;
    }
    usleep(50000);
  }
  *took = difftime(time(0), start);
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

int main()
{
  char dir[] = "/tmp/authcache-pipe-XXXXXX";
  if (!mkdtemp(dir)) return 1;
  setenv("HOME", dir, 1);
  setenv("ECCE_REALUSERHOME", dir, 1);
  setenv("ECCE_NO_MESSAGING", "1", 1);
  double took = 0;

  string pipe = string(dir) + "/p1";
  pid_t r = reader(pipe);
  usleep(500000);
  pid_t w = writer(pipe);
  int got = waitFor(r, 15, &took);
  waitFor(w, 15, &took);
  check(got == 1, "reader first: one entry handed over");

  pipe = string(dir) + "/p2";
  w = writer(pipe);
  sleep(1);
  r = reader(pipe);
  got = waitFor(r, 15, &took);
  waitFor(w, 15, &took);
  check(got == 1, "writer first: one entry handed over");

  // A FIFO nobody will write to: the writer gave up or never opened it.
  pipe = string(dir) + "/p3";
  mkfifo(pipe.c_str(), 0600);
  r = reader(pipe);
  got = waitFor(r, 15, &took);
  check(got == 0, "no writer: pipeIn returns with an empty cache "
                  "(not a hang)");
  check(access(pipe.c_str(), F_OK) != 0, "no writer: the FIFO is removed");

  // The Gateway's socketpair (#637): "socket:<fd>" inherited by the child.
  for (int order = 0; order < 3; order++) {
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) {
      check(false, "socketpair");
      break;
    }
    const string arg = "socket:" + to_string(fds[0]);
    pid_t rs = fork();
    if (rs == 0) {
      close(fds[1]);
      if (order == 1) usleep(500000);        // the writer finishes first
      AuthCache::getCache().pipeIn(arg);
      _exit(AuthCache::getCache().size());
    }
    close(fds[0]);
    //  The writer is a child too, so this process's cache stays empty and
    //  the next reader inherits nothing by fork.
    pid_t ws = fork();
    if (ws == 0) {
      if (order == 0) usleep(500000);        // the reader waits first
      if (order != 2) {                      // 2: a writer that sends nothing
        AuthCache::getCache().addAuthentication("http://localhost:1/Ecce/",
                                                "alice", "dummy", "", false);
        AuthCache::getCache().pipeOutSocket(fds[1]);
      }
      _exit(0);
    }
    close(fds[1]);
    got = waitFor(rs, 15, &took);
    waitFor(ws, 15, &took);
    static const char *what[] = {
      "socket, reader first: one entry handed over",
      "socket, writer first: one entry handed over",
      "socket, writer closes without data: empty cache, no hang"};
    check(got == (order == 2 ? 0 : 1), what[order]);
  }

  rmdir(dir);
  cout << (g_fail ? "FAILED" : "PASSED") << endl;
  return g_fail ? 1 : 0;
}
