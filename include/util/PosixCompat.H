#ifndef POSIXCOMPAT_H
#define POSIXCOMPAT_H

// POSIX calls that MinGW-w64 lacks, as thin wrappers over their Win32
// equivalents (#133). Elsewhere this header only includes the POSIX headers
// that declare them, so Linux and macOS call sites compile unchanged.

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctime>

#ifndef _WIN32
#include <pwd.h>
#include <sys/file.h>
#include <sys/utsname.h>
#else
#include <windows.h>
#include <direct.h>
#include <io.h>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <locale>
#include <sstream>

// The handle is not inherited by child processes.
#ifndef O_CLOEXEC
#define O_CLOEXEC O_NOINHERIT
#endif

// Windows has no POSIX permission bits; access is governed by the ACL of
// the parent directory (the user profile), so the mode is ignored.
inline int mkdir(const char *path, mode_t) { return _mkdir(path); }
inline int fchmod(int, mode_t) { return 0; }
inline int fsync(int fd) { return _commit(fd); }

// No symbolic links are reported, so readlink/symlink are never reached.
#ifndef S_ISLNK
#define S_ISLNK(m) 0
#endif
inline int lstat(const char *path, struct stat *st) { return stat(path, st); }
inline ssize_t readlink(const char *, char *, size_t) { errno = ENOSYS; return -1; }
inline int symlink(const char *, const char *) { errno = ENOSYS; return -1; }

// realpath fails for a missing path; _fullpath does not check.
inline char *realpath(const char *path, char *resolved)
{
  if (_access(path, 0) != 0) { errno = ENOENT; return nullptr; }
  return _fullpath(resolved, path, PATH_MAX);
}

// Advisory whole-file locks on lock files. LockFileEx locks are mandatory
// and cannot be upgraded in place, which no caller relies on.
#define LOCK_SH 1
#define LOCK_EX 2
#define LOCK_NB 4
#define LOCK_UN 8
inline int flock(int fd, int op)
{
  HANDLE h = (HANDLE)_get_osfhandle(fd);
  if (h == INVALID_HANDLE_VALUE) { errno = EBADF; return -1; }
  OVERLAPPED ov = {};
  if (op & LOCK_UN) {
    if (UnlockFileEx(h, 0, MAXDWORD, MAXDWORD, &ov)) return 0;
    errno = EINVAL;
    return -1;
  }
  DWORD flags = ((op & LOCK_EX) ? LOCKFILE_EXCLUSIVE_LOCK : 0) |
                ((op & LOCK_NB) ? LOCKFILE_FAIL_IMMEDIATELY : 0);
  if (LockFileEx(h, flags, 0, MAXDWORD, MAXDWORD, &ov)) return 0;
  errno = GetLastError() == ERROR_LOCK_VIOLATION ? EWOULDBLOCK : EINVAL;
  return -1;
}

inline int setenv(const char *name, const char *value, int overwrite)
{
  if (!overwrite && getenv(name)) return 0;
  return _putenv_s(name, value) == 0 ? 0 : -1;
}

inline char *strptime(const char *s, const char *format, struct tm *tm)
{
  std::istringstream in(s);
  in.imbue(std::locale::classic());
  in >> std::get_time(tm, format);
  if (in.fail()) return nullptr;
  std::streamoff used = in.eof() ? (std::streamoff)strlen(s)
                                 : (std::streamoff)in.tellg();
  return const_cast<char *>(s) + used;
}

inline int gethostname(char *name, size_t len)
{
  DWORD n = (DWORD)len;
  return GetComputerNameExA(ComputerNameDnsHostname, name, &n) ? 0 : -1;
}

struct utsname {
  char sysname[65];
  char nodename[257];
  char release[65];
  char version[65];
  char machine[65];
};
inline int uname(struct utsname *u)
{
  memset(u, 0, sizeof(*u));
  strcpy(u->sysname, "Windows");
  DWORD n = sizeof(u->nodename);
  GetComputerNameExA(ComputerNameDnsHostname, u->nodename, &n);
  SYSTEM_INFO si;
  GetNativeSystemInfo(&si);
  strcpy(u->machine, si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64
                     ? "arm64" : "x86_64");
  return 0;
}

// One account per process; the name and home come from the environment.
typedef int uid_t;
inline uid_t getuid() { return 0; }
inline uid_t geteuid() { return 0; }
struct passwd {
  char *pw_name;
  char *pw_dir;
};
inline struct passwd *getpwuid(uid_t)
{
  static struct passwd pw;
  pw.pw_name = getenv("USERNAME");
  pw.pw_dir = getenv("USERPROFILE");
  return pw.pw_name ? &pw : nullptr;
}
#endif

#endif
