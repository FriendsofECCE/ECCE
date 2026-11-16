#ifndef STREAMREAD_H
#define STREAMREAD_H

#include <errno.h>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <poll.h>
#include <unistd.h>
#endif

// Appends what the stream fd holds now and returns at once; false once it
// has ended (out may still have the last data).  Called on the GUI thread
// (Tail window), so it must never wait.  A Windows anonymous pipe cannot be
// made non-blocking (fcntl is a stub there), so it is peeked first; POSIX
// checks with poll as well, so a descriptor left blocking cannot hang it.
inline bool readAvailable(int fd, std::string& out)
{
  if (fd < 0) return false;
  char buf[16384];
  for (;;) {
#ifdef _WIN32
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    unsigned want = sizeof buf;
    if (h != INVALID_HANDLE_VALUE && GetFileType(h) == FILE_TYPE_PIPE) {
      DWORD avail = 0;
      if (!PeekNamedPipe(h, 0, 0, 0, &avail, 0)) return false;   // writer gone
      if (avail == 0) return true;
      if (avail < want) want = (unsigned)avail;
    }
    int n = _read(fd, buf, want);
#else
    pollfd p;
    p.fd = fd; p.events = POLLIN; p.revents = 0;
    int r = ::poll(&p, 1, 0);
    if (r < 0 && errno == EINTR) continue;
    if (r == 0) return true;
    ssize_t n = ::read(fd, buf, sizeof buf);
#endif
    if (n > 0) { out.append(buf, n); continue; }
    if (n < 0 && errno == EINTR) continue;
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return true;
    return false;
  }
}

#endif
