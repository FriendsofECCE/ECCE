/* ECCE.app's CFBundleExecutable.  TCC names the responsible process after
   the bundle's executable; as a bash script that was /bin/bash, so the
   folder-access prompt (and NS*FolderUsageDescription) never applied to
   ECCE.  This starts Resources/ecce.sh as a child (not exec) and stays as the app. */
#include <mach-o/dyld.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;
static volatile pid_t child;

static void forward(int sig) { if (child > 0) kill(child, sig); }

int main(int argc, char **argv) {
  char exe[PATH_MAX], real[PATH_MAX], script[PATH_MAX + 32];
  uint32_t n = sizeof exe;
  if (_NSGetExecutablePath(exe, &n) != 0 || !realpath(exe, real)) {
    fprintf(stderr, "ECCE: cannot find its own path\n");
    return 127;
  }
  /* Contents/MacOS/ecce -> Contents/Resources/ecce.sh (a script in MacOS
     would be a code subcomponent that codesign wants signed). */
  char *slash = strrchr(real, '/');
  if (slash) *slash = 0;
  snprintf(script, sizeof script, "%s/../Resources/ecce.sh", real);
  char **av = calloc(argc + 3, sizeof *av);
  av[0] = "/bin/bash";
  av[1] = script;
  for (int i = 1; i < argc; i++) av[i + 1] = argv[i];
  pid_t pid;
  int err = posix_spawn(&pid, "/bin/bash", NULL, NULL, av, environ);
  if (err) { fprintf(stderr, "ECCE: cannot start %s: %s\n", script, strerror(err)); return 127; }
  child = pid;
  struct sigaction sa;
  memset(&sa, 0, sizeof sa);
  sa.sa_handler = forward;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGHUP, &sa, NULL);
  int st;
  while (waitpid(pid, &st, 0) < 0)
    if (errno != EINTR) return 127;
  return WIFEXITED(st) ? WEXITSTATUS(st) : 128 + WTERMSIG(st);
}
