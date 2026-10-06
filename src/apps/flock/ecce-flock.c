/*
 * ecce-flock [-n] [-w SECONDS] [-E CODE] FD
 * ecce-flock [-n] [-w SECONDS] [-E CODE] FILE COMMAND [ARG...]
 *
 * The subset of util-linux flock(1) the session scripts use, for systems
 * without it (macOS).  An exclusive flock(2) lock: on an inherited
 * descriptor it stays held by the caller after this exits, as with
 * flock(1); on FILE it is held while COMMAND runs.  Exit CODE (default 1)
 * when the lock could not be taken.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static int take(int fd, int nowait, double wait)
{
  struct timespec start, now, nap = { 0, 50 * 1000 * 1000 };
  int poll = nowait || wait >= 0;
  clock_gettime(CLOCK_MONOTONIC, &start);
  for (;;) {
    if (flock(fd, LOCK_EX | (poll ? LOCK_NB : 0)) == 0) return 0;
    if (errno == EINTR) continue;
    if (errno != EWOULDBLOCK || nowait) return -1;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if ((now.tv_sec - start.tv_sec) + (now.tv_nsec - start.tv_nsec) / 1e9
        >= wait)
      return -1;
    nanosleep(&nap, 0);
  }
}

static int usage(void)
{
  fprintf(stderr, "usage: ecce-flock [-n] [-w SECONDS] [-E CODE] "
                  "FD | FILE COMMAND [ARG...]\n");
  return 64;
}

int main(int argc, char **argv)
{
  int nowait = 0, code = 1, i = 1, fd, status;
  double wait = -1;
  pid_t pid;
  for (; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
    if (!strcmp(argv[i], "-n")) nowait = 1;
    else if (!strcmp(argv[i], "-w") && i + 1 < argc) wait = atof(argv[++i]);
    else if (!strcmp(argv[i], "-E") && i + 1 < argc) code = atoi(argv[++i]);
    else return usage();
  }
  if (i >= argc) return usage();

  if (i + 1 == argc) {
    char *end;
    long n = strtol(argv[i], &end, 10);
    if (*end || n < 0) return usage();
    return take((int) n, nowait, wait) == 0 ? 0 : code;
  }

  fd = open(argv[i], O_RDONLY | O_CREAT | O_CLOEXEC, 0666);
  if (fd < 0) fd = open(argv[i], O_RDONLY | O_CLOEXEC);
  if (fd < 0) { perror(argv[i]); return 1; }
  if (take(fd, nowait, wait) != 0) return code;
  pid = fork();
  if (pid < 0) { perror("fork"); return 1; }
  if (pid == 0) {
    execvp(argv[i + 1], argv + i + 1);
    perror(argv[i + 1]);
    _exit(127);
  }
  while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
  if (WIFEXITED(status)) return WEXITSTATUS(status);
  return 128 + WTERMSIG(status);
}
