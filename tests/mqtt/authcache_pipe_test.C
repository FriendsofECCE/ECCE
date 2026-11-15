// AuthCache's -pipe handoff between two processes (#605): the parent's
// pipeOut() and the child's pipeIn() must hand over the cache whichever
// starts first, and pipeIn() must return when no writer ever comes (it
// blocked in fopen() for good, which hung the Viewer on a Mac).  After
// reading it removes only what ECCE handed over, a FIFO or its own AuthPipe.*
// temp file, never a path the caller owns (#1060).
//
//   authcache_pipe_test

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "tdat/AuthCache.H"
#include "util/TempStorage.H"

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

// A cache file as the test harnesses and pipeOutData() on Windows write it.
static void writeCache(const string& path, mode_t mode)
{
  int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, mode);
  if (fd < 0) return;
  const string data = "1\nhttp://localhost:1/Ecce/|alice|dummy\n";
  if (write(fd, data.data(), data.size()) < 0) {}
  close(fd);
  chmod(path.c_str(), mode);
}

static bool present(const string& path)
{
  struct stat st;
  return lstat(path.c_str(), &st) == 0;
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
  setenv("ECCE_HOME", dir, 1);                 // only for Ecce::realUser()
  setenv("ECCE_TMPDIR", dir, 1);
  const string root = TempStorage::getTempRootPath();
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

  // -pipe /dev/null (the launch tests' local mode): its end of data is
  // final, so there is nothing to wait for (#1047).
  r = reader("/dev/null");
  got = waitFor(r, 15, &took);
  check(got == 0 && took < 3, "/dev/null: pipeIn returns at once with an "
                              "empty cache");

  struct stat devnull;
  check(stat("/dev/null", &devnull) == 0 && S_ISCHR(devnull.st_mode),
        "/dev/null: left in place");

  // Linux production: a FIFO under the name pipeName() makes.
  pipe = AuthCache::pipeName();
  w = writer(pipe);
  r = reader(pipe);
  got = waitFor(r, 15, &took);
  waitFor(w, 15, &took);
  check(got == 1 && !present(pipe), "pipeName() FIFO: read and removed");

  // Windows production: pipeOutData() writes a plain AuthPipe.* file into
  // the temp root (mkstemp: this user's, mode 0600).
  pipe = root + "/AuthPipe.XXXXXX";
  {
    char *tmpl = &pipe[0];
    int fd = mkstemp(tmpl);
    if (fd >= 0) close(fd);
  }
  writeCache(pipe, 0600);
  got = waitFor(reader(pipe), 15, &took);
  check(got == 1 && !present(pipe), "AuthPipe temp file: read and removed");

  // A file the caller passed (the test harnesses write these): kept.
  pipe = string(dir) + "/creds";
  writeCache(pipe, 0600);
  got = waitFor(reader(pipe), 15, &took);
  check(got == 1 && present(pipe), "caller's own file: read and kept");

  // ECCE's name but not ECCE's file: outside the temp root, or readable
  // by others.
  pipe = string(dir) + "/AuthPipe.outside";
  writeCache(pipe, 0600);
  got = waitFor(reader(pipe), 15, &took);
  check(got == 1 && present(pipe), "AuthPipe.* outside the temp root: kept");
  unlink(pipe.c_str());
  pipe = root + "/AuthPipe.shared";
  writeCache(pipe, 0644);
  got = waitFor(reader(pipe), 15, &took);
  check(got == 1 && present(pipe), "AuthPipe.* with mode 0644: kept");
  unlink(pipe.c_str());

  // A symlink, even one named like ECCE's file in the temp root: neither
  // the link nor its target goes.
  string target = string(dir) + "/target";
  writeCache(target, 0600);
  pipe = root + "/AuthPipe.link";
  if (symlink(target.c_str(), pipe.c_str()) != 0) check(false, "symlink");
  got = waitFor(reader(pipe), 15, &took);
  check(got == 1 && present(pipe) && present(target),
        "symlink: link and target kept");
  unlink(pipe.c_str());
  unlink(target.c_str());

  // A directory: nothing to read, nothing removed.
  pipe = string(dir) + "/adir";
  mkdir(pipe.c_str(), 0700);
  got = waitFor(reader(pipe), 15, &took);
  check(got == 0 && present(pipe), "directory: kept");
  rmdir(pipe.c_str());
  unlink((string(dir) + "/creds").c_str());

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

  rmdir(root.c_str());
  rmdir(dir);
  cout << (g_fail ? "FAILED" : "PASSED") << endl;
  return g_fail ? 1 : 0;
}
