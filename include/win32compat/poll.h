// <poll.h> over Winsock's WSAPoll; sockets only, not pipes or files.
#ifndef WIN32COMPAT_POLL_H
#define WIN32COMPAT_POLL_H
#include <winsock2.h>
typedef unsigned long nfds_t;
inline int poll(struct pollfd *fds, nfds_t n, int timeout)
{
  return WSAPoll(fds, n, timeout);
}
#endif
