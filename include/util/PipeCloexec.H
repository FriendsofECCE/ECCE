#ifndef PIPECLOEXEC_H
#define PIPECLOEXEC_H

#include <cerrno>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

// pipe2() is Linux-only; macOS (#133) needs pipe() plus FD_CLOEXEC. The
// two-step form leaves a short window in which a concurrent fork could
// inherit the fds, which is harmless here: every caller forks right after.
inline int pipeCloexec(int fds[2])
{
#ifdef _WIN32
  return pipe(fds);   // PosixCompat.H: created not inheritable
#elif defined(__APPLE__)
  if (pipe(fds) < 0) return -1;
  fcntl(fds[0], F_SETFD, FD_CLOEXEC);
  fcntl(fds[1], F_SETFD, FD_CLOEXEC);
  return 0;
#else
  return pipe2(fds, O_CLOEXEC);
#endif
}

// SOCK_CLOEXEC is not in macOS's socketpair(); same two-step form and
// the same fork window as pipeCloexec above.
inline int socketpairCloexec(int domain, int type, int protocol, int sv[2])
{
#ifdef _WIN32
  (void)domain; (void)type; (void)protocol; (void)sv;
  errno = ENOSYS;   // callers fall back or report; no AF_UNIX socketpair
  return -1;
#elif defined(__APPLE__)
  if (socketpair(domain, type, protocol, sv) < 0) return -1;
  fcntl(sv[0], F_SETFD, FD_CLOEXEC);
  fcntl(sv[1], F_SETFD, FD_CLOEXEC);
  return 0;
#else
  return socketpair(domain, type | SOCK_CLOEXEC, protocol, sv);
#endif
}

#endif
