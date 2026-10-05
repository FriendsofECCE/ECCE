#ifndef CONSUMESIGNAL_H
#define CONSUMESIGNAL_H

#include <csignal>
#include <ctime>

// Takes one already-pending, blocked signal off the calling thread, so
// unblocking afterwards does not deliver it. macOS has no sigtimedwait
// (#133); sigwait cannot block there because the caller has just seen the
// signal pending via sigpending() and it is blocked in this thread.
inline void consumePendingSignal(const sigset_t *set)
{
#ifdef __APPLE__
  int sig;
  sigwait(set, &sig);
#else
  struct timespec zero = { 0, 0 };
  sigtimedwait(set, 0, &zero);
#endif
}

#endif
