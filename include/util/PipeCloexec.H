#ifndef PIPECLOEXEC_H
#define PIPECLOEXEC_H

#include <fcntl.h>
#include <unistd.h>

// pipe2() is Linux-only; macOS (#133) needs pipe() plus FD_CLOEXEC. The
// two-step form leaves a short window in which a concurrent fork could
// inherit the fds, which is harmless here: every caller forks right after.
inline int pipeCloexec(int fds[2])
{
#ifdef __APPLE__
  if (pipe(fds) < 0) return -1;
  fcntl(fds[0], F_SETFD, FD_CLOEXEC);
  fcntl(fds[1], F_SETFD, FD_CLOEXEC);
  return 0;
#else
  return pipe2(fds, O_CLOEXEC);
#endif
}

#endif
