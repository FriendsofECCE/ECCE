// Stand-in for <sys/wait.h> on Windows, which has no child-process status
// model: waitpid reports no children. Spawning goes through other code (#133).
#ifndef WIN32COMPAT_SYS_WAIT_H
#define WIN32COMPAT_SYS_WAIT_H
#include "util/PosixCompat.H"
#define WNOHANG 1
#define WUNTRACED 2
#define WIFEXITED(s) 1
#define WEXITSTATUS(s) ((s) & 0xff)
#define WIFSIGNALED(s) 0
#define WTERMSIG(s) 0
#define WIFSTOPPED(s) 0
inline pid_t waitpid(pid_t, int *, int) { errno = ECHILD; return -1; }
inline pid_t wait(int *) { errno = ECHILD; return -1; }
#endif
