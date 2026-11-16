// readAvailable() (comm/StreamRead.H), which the Tail window calls on the
// GUI thread: on a blocking pipe it must return what is there and come back
// at once when the pipe is empty, and report the end once the writer
// closes.  A plain read() blocks on the empty pipe; on Windows, where a pipe
// cannot be made non-blocking, that froze the Organizer as soon as the
// followed file stopped growing.  Exit 0 only if every case passes.

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <string>
#include <thread>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include "comm/StreamRead.H"

static int failures = 0;

static void check(const char* name, bool ok, const std::string& why = "")
{
  printf("%s %s\n", ok ? "ok  " : "FAIL", name);
  if (!ok && !why.empty()) printf("     [%s]\n", why.c_str());
  fflush(stdout);
  if (!ok) failures++;
}

// readAvailable on another thread; a call stuck in read() cannot be
// cancelled, so a hang ends the program with a failure.
static bool readWithin(int fd, std::string& out, const char* name)
{
  std::packaged_task<bool()> task([fd, &out] { return readAvailable(fd, out); });
  std::future<bool> f = task.get_future();
  std::thread(std::move(task)).detach();
  if (f.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
    check(name, false, "still blocked after 5 s (the GUI thread would hang)");
    printf("%d failure(s)\n", failures);
    fflush(stdout);
    _exit(1);
  }
  return f.get();
}

static int writeAll(int fd, const std::string& s)
{
#ifdef _WIN32
  return _write(fd, s.data(), (unsigned)s.size());
#else
  return (int)write(fd, s.data(), s.size());
#endif
}

int main()
{
  int p[2];
#ifdef _WIN32
  // A CRT pipe on an anonymous pipe, as DirectTransportWin's streams are.
  if (_pipe(p, 4096, _O_BINARY) != 0) { perror("pipe"); return 1; }
#else
  if (pipe(p) != 0) { perror("pipe"); return 1; }
#endif

  writeAll(p[1], "line 1\nline 2\n");
  std::string out;
  bool open = readWithin(p[0], out, "data waiting");
  check("data waiting: returned, stream open", open);
  check("data waiting: all of it read", out == "line 1\nline 2\n", out);

  out.clear();
  open = readWithin(p[0], out, "empty pipe, writer open");
  check("empty pipe: returned at once, stream open", open && out.empty(), out);

  writeAll(p[1], "last\n");
#ifdef _WIN32
  _close(p[1]);
#else
  close(p[1]);
#endif
  out.clear();
  bool more = readWithin(p[0], out, "writer closed");
  if (more) more = readWithin(p[0], out, "writer closed, again");
  check("writer closed: end reported", !more);
  check("writer closed: last data kept", out == "last\n", out);

  printf("%d failure(s)\n", failures);
  fflush(stdout);
  _exit(failures ? 1 : 0);   // no join: a detached reader may still exist
}
