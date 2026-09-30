#include "comm/DirectTransport.H"

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <vector>

#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

namespace {

// Everything the child needs, built before fork() so the child makes only
// async-signal-safe calls.
struct ChildSpec {
  std::vector<std::string> envStore;
  std::vector<char*> envp;
  std::vector<char*> argv;
  int maxFd;
};

void buildEnv(ChildSpec& cs, const std::map<std::string, std::string>& set,
              const std::map<std::string, bool>& unset)
{
  for (char** e = environ; e && *e; e++) {
    const char* eq = strchr(*e, '=');
    std::string name = eq ? std::string(*e, eq - *e) : std::string(*e);
    if (set.count(name) || unset.count(name)) continue;
    cs.envStore.push_back(*e);
  }
  for (std::map<std::string, std::string>::const_iterator i = set.begin();
       i != set.end(); ++i)
    cs.envStore.push_back(i->first + "=" + i->second);
  for (size_t i = 0; i < cs.envStore.size(); i++)
    cs.envp.push_back(const_cast<char*>(cs.envStore[i].c_str()));
  cs.envp.push_back(0);
  long m = sysconf(_SC_OPEN_MAX);
  cs.maxFd = (m < 0 || m > 65536) ? 65536 : (int)m;
}

void closeFrom(int first, int maxFd)
{
#ifdef SYS_close_range
  if (syscall(SYS_close_range, (unsigned)first, ~0u, 0u) == 0) return;
#endif
  for (int fd = first; fd < maxFd; fd++) close(fd);
}

// Runs in the forked child.  Never returns.  With scriptFd >= 0 the script
// is read from fd 3 (`sh /dev/fd/3`) and stdin is /dev/null, so nothing the
// script runs can consume the script itself.
void childExec(const ChildSpec& cs, int in, int out, int err, bool newGroup,
               int scriptFd = -1)
{
  if (newGroup) setpgid(0, 0);
  for (int s = 1; s < NSIG; s++) {
    if (s == SIGKILL || s == SIGSTOP) continue;
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = SIG_DFL;
    sigaction(s, &sa, 0);
  }
  sigset_t none;
  sigemptyset(&none);
  sigprocmask(SIG_SETMASK, &none, 0);

  if (in != 0) dup2(in, 0);
  if (out != 1) dup2(out, 1);
  if (err != 2) dup2(err, 2);
  if (scriptFd >= 0) {
    dup2(scriptFd, 3);
    closeFrom(4, cs.maxFd);
  } else {
    closeFrom(3, cs.maxFd);
  }

  execve("/bin/sh", const_cast<char* const*>(&cs.argv[0]), const_cast<char* const*>(&cs.envp[0]));
  _exit(127);
}

void setNonBlock(int fd)
{
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

void closeFd(int& fd)
{
  if (fd >= 0) { close(fd); fd = -1; }
}

typedef std::chrono::steady_clock Clock;

int waitChild(pid_t pid, int& status)
{
  for (;;) {
    pid_t r = waitpid(pid, &status, 0);
    if (r >= 0) return 0;
    if (errno != EINTR) return -1;
  }
}

int decode(int st)
{
  if (WIFEXITED(st)) return WEXITSTATUS(st);
  if (WIFSIGNALED(st)) return 128 + WTERMSIG(st);
  return -1;
}

void sleepMs(int ms)
{
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000L;
  nanosleep(&ts, 0);
}

}  // namespace

TransportResult DirectTransport::run(const std::string& script, int timeoutSec)
{
  TransportResult res;
  std::string full = withDir(script);

  ChildSpec cs;
  buildEnv(cs, p_env, p_unset);
  static char a0[] = "sh", a1[] = "/dev/fd/3";
  cs.argv.push_back(a0);
  cs.argv.push_back(a1);
  cs.argv.push_back(0);

  int pin[2], pout[2], perr[2];
  if (pipe2(pin, O_CLOEXEC) < 0) { res.error = strerror(errno); return res; }
  if (pipe2(pout, O_CLOEXEC) < 0) {
    res.error = strerror(errno);
    close(pin[0]); close(pin[1]);
    return res;
  }
  if (pipe2(perr, O_CLOEXEC) < 0) {
    res.error = strerror(errno);
    close(pin[0]); close(pin[1]); close(pout[0]); close(pout[1]);
    return res;
  }

  // Keep the script pipe and /dev/null clear of 0-3, which the child
  // redirects onto.
  int devnull = open("/dev/null", O_RDONLY | O_CLOEXEC);
  if (devnull >= 0) {
    int hi = fcntl(devnull, F_DUPFD_CLOEXEC, 10);
    close(devnull);
    devnull = hi;
  }
  if (devnull >= 0) {
    int hi = fcntl(pin[0], F_DUPFD_CLOEXEC, 10);
    if (hi >= 0) { close(pin[0]); pin[0] = hi; } else { close(devnull); devnull = -1; }
  }
  if (devnull < 0) {
    res.error = strerror(errno);
    close(pin[0]); close(pin[1]); close(pout[0]); close(pout[1]);
    close(perr[0]); close(perr[1]);
    return res;
  }

  // A child that exits without reading the script must not kill us with SIGPIPE.
  sigset_t pipeSet, oldMask, pendBefore;
  sigemptyset(&pipeSet);
  sigaddset(&pipeSet, SIGPIPE);
  sigpending(&pendBefore);
  bool pipePendedBefore = sigismember(&pendBefore, SIGPIPE) == 1;
  pthread_sigmask(SIG_BLOCK, &pipeSet, &oldMask);

  pid_t pid = fork();
  if (pid < 0) {
    res.error = strerror(errno);
    close(pin[0]); close(pin[1]); close(pout[0]); close(pout[1]);
    close(perr[0]); close(perr[1]); close(devnull);
    pthread_sigmask(SIG_SETMASK, &oldMask, 0);
    return res;
  }
  if (pid == 0) childExec(cs, devnull, pout[1], perr[1], true, pin[0]);

  setpgid(pid, pid);   // also done by the child; whoever runs first wins
  close(pin[0]); close(pout[1]); close(perr[1]); close(devnull);
  int fin = pin[1], fout = pout[0], ferr = perr[0];
  setNonBlock(fin); setNonBlock(fout); setNonBlock(ferr);

  Clock::time_point deadline;
  if (timeoutSec > 0) deadline = Clock::now() + std::chrono::seconds(timeoutSec);

  size_t written = 0;
  char buf[65536];
  bool expired = false;
  while (fin >= 0 || fout >= 0 || ferr >= 0) {
    struct pollfd p[3];
    int n = 0, iIn = -1, iOut = -1, iErr = -1;
    if (fin >= 0)  { p[n].fd = fin;  p[n].events = POLLOUT; iIn = n++; }
    if (fout >= 0) { p[n].fd = fout; p[n].events = POLLIN;  iOut = n++; }
    if (ferr >= 0) { p[n].fd = ferr; p[n].events = POLLIN;  iErr = n++; }
    int ms = -1;
    if (timeoutSec > 0) {
      long left = std::chrono::duration_cast<std::chrono::milliseconds>(
                    deadline - Clock::now()).count();
      if (left <= 0) { expired = true; break; }
      ms = (int)left;
    }
    int r = poll(p, n, ms);
    if (r < 0) {
      if (errno == EINTR) continue;
      res.error = strerror(errno);
      break;
    }
    if (r == 0) continue;
    if (iIn >= 0 && (p[iIn].revents & (POLLOUT | POLLERR | POLLHUP))) {
      ssize_t w = write(fin, full.data() + written, full.size() - written);
      if (w > 0) written += w;
      if ((w < 0 && errno != EAGAIN && errno != EINTR) || written >= full.size())
        closeFd(fin);
    }
    int idx[2] = { iOut, iErr };
    int* fds[2] = { &fout, &ferr };
    std::string* dst[2] = { &res.out, &res.err };
    for (int k = 0; k < 2; k++) {
      if (idx[k] < 0 || !(p[idx[k]].revents & (POLLIN | POLLERR | POLLHUP))) continue;
      ssize_t got = read(*fds[k], buf, sizeof buf);
      if (got > 0) {
        dst[k]->append(buf, got);
        if (timeoutSec > 0) deadline = Clock::now() + std::chrono::seconds(timeoutSec);
      } else if (got == 0 || (errno != EAGAIN && errno != EINTR)) {
        closeFd(*fds[k]);
      }
    }
  }
  closeFd(fin);

  int st = 0;
  bool reaped = false;
  if (!expired && timeoutSec > 0) {
    // Output closed; the script may still be running.
    while (!reaped) {
      pid_t r = waitpid(pid, &st, WNOHANG);
      if (r == pid) reaped = true;
      else if (r < 0 && errno != EINTR) break;
      else if (Clock::now() >= deadline) { expired = true; break; }
      else sleepMs(5);
    }
  }
  if (expired) {
    res.timedOut = true;
    res.error = "timed out after " + std::to_string(timeoutSec) + " s";
    kill(-pid, SIGTERM);
    for (int i = 0; i < 100 && !reaped; i++) {
      pid_t r = waitpid(pid, &st, WNOHANG);
      if (r == pid) reaped = true;
      else if (r < 0 && errno != EINTR) break;
      else sleepMs(10);
    }
    kill(-pid, SIGKILL);   // stragglers that ignored TERM
    if (!reaped) waitChild(pid, st);
    reaped = true;
    // Drain whatever was already buffered.
    int* fds[2] = { &fout, &ferr };
    std::string* dst[2] = { &res.out, &res.err };
    for (int k = 0; k < 2; k++) {
      while (*fds[k] >= 0) {
        ssize_t got = read(*fds[k], buf, sizeof buf);
        if (got > 0) dst[k]->append(buf, got);
        else break;
      }
    }
  }
  closeFd(fout); closeFd(ferr);
  if (!reaped && waitChild(pid, st) < 0) {
    res.error = strerror(errno);
    res.status = -1;
  } else {
    res.status = decode(st);
  }

  if (!pipePendedBefore) {
    sigset_t pend;
    sigpending(&pend);
    if (sigismember(&pend, SIGPIPE) == 1) {
      struct timespec zero = { 0, 0 };
      sigtimedwait(&pipeSet, 0, &zero);
    }
  }
  pthread_sigmask(SIG_SETMASK, &oldMask, 0);
  return res;
}

long DirectTransport::spawnDetached(const std::string& script, std::string& error,
                                    const std::string& logFile)
{
  // The script is one argv element; execve caps that at 128 KiB.
  std::string full = withDir(script);
  if (full.size() > 100000) {
    error = "script too long to spawn detached";
    return -1;
  }

  ChildSpec cs;
  buildEnv(cs, p_env, p_unset);
  static char a0[] = "sh", a1[] = "-c";
  std::string cmd = full;
  cs.argv.push_back(a0);
  cs.argv.push_back(a1);
  cs.argv.push_back(const_cast<char*>(cmd.c_str()));
  cs.argv.push_back(0);

  int devnull = open("/dev/null", O_RDWR | O_CLOEXEC);
  if (devnull < 0) { error = strerror(errno); return -1; }
  int outfd = devnull;
  if (!logFile.empty()) {
    outfd = open(logFile.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
    if (outfd < 0) {
      error = "cannot open " + logFile + ": " + strerror(errno);
      close(devnull);
      return -1;
    }
  }
  int rp[2];
  if (pipe2(rp, O_CLOEXEC) < 0) {
    error = strerror(errno);
    close(devnull);
    if (outfd != devnull) close(outfd);
    return -1;
  }

  pid_t mid = fork();
  if (mid < 0) {
    error = strerror(errno);
    close(devnull); if (outfd != devnull) close(outfd);
    close(rp[0]); close(rp[1]);
    return -1;
  }
  if (mid == 0) {
    setsid();
    pid_t w = fork();
    if (w == 0) childExec(cs, devnull, outfd, outfd, false);
    long v = (long)w;   // -1 if the fork failed
    ssize_t ignored = write(rp[1], &v, sizeof v);
    (void)ignored;
    _exit(0);
  }

  close(rp[1]);
  close(devnull);
  if (outfd != devnull) close(outfd);
  long pid = -1;
  size_t got = 0;
  while (got < sizeof pid) {
    ssize_t r = read(rp[0], reinterpret_cast<char*>(&pid) + got, sizeof pid - got);
    if (r > 0) got += r;
    else if (r < 0 && errno == EINTR) continue;
    else break;
  }
  close(rp[0]);
  int st;
  waitChild(mid, st);
  if (got < sizeof pid || pid < 0) {
    error = "could not start the detached process";
    return -1;
  }
  return pid;
}
