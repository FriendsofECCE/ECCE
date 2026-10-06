#include <winsock2.h>
#include <ws2tcpip.h>
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
// Winsock has no per-call non-blocking send; the sockets that use this are
// not created on Windows yet (socketpairCloexec fails).
#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0
#endif
#ifndef SHUT_RDWR
#define SHUT_RD SD_RECEIVE
#define SHUT_WR SD_SEND
#define SHUT_RDWR SD_BOTH
#endif
// Winsock takes the option value as const char*.
#define setsockopt(s, lvl, opt, val, len) \
  setsockopt((s), (lvl), (opt), (const char *)(val), (len))
