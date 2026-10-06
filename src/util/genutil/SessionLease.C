///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: SessionLease.C   (see include/util/SessionLease.H)
///////////////////////////////////////////////////////////////////////////////
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include "util/Ecce.H"
#include "util/SessionLease.H"

using std::string;
using std::vector;

namespace {

string resolvedSelf()
{
  char buf[PATH_MAX];
#if defined(__linux__)
  ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (n <= 0) return "";
  buf[n] = '\0';
  return buf;
#elif defined(__APPLE__)
  uint32_t size = sizeof(buf);
  if (_NSGetExecutablePath(buf, &size) != 0) return "";
  char real[PATH_MAX];
  return realpath(buf, real) ? string(real) : string();
#else
  return "";
#endif
}

string realOf(const string& path)
{
  char real[PATH_MAX];
  return realpath(path.c_str(), real) ? string(real) : string();
}

// $ECCE_HOME/bin holds it, or $ECCE_HOME/bin/<name> resolves to it.
bool isEcceProgram(const string& exe, string& name)
{
  const char* home = getenv("ECCE_HOME");
  if (!home || !*home || exe.empty()) return false;
  name = exe.substr(exe.rfind('/') + 1);
  string bindir = realOf(string(home) + "/bin");
  if (!bindir.empty() && exe.compare(0, bindir.size() + 1, bindir + "/") == 0)
    return true;
  return realOf(string(home) + "/bin/" + name) == exe;
}

void mkdirs(const string& path)
{
  for (size_t i = 1; i <= path.size(); i++)
    if (i == path.size() || path[i] == '/')
      mkdir(path.substr(0, i).c_str(), 0700);
}

bool sameFile(int fd, const string& path)
{
  struct stat a, b;
  return fstat(fd, &a) == 0 && stat(path.c_str(), &b) == 0 &&
         a.st_dev == b.st_dev && a.st_ino == b.st_ino;
}

}  // namespace

namespace SessionLease {

bool useLease()
{
  const char* m = getenv("ECCE_SESSION_LIVENESS");
  return !(m && strcmp(m, "proc") == 0);
}

bool useProc()
{
#if defined(__linux__)
  const char* m = getenv("ECCE_SESSION_LIVENESS");
  return !(m && strcmp(m, "lease") == 0);
#else
  return false;
#endif
}

void acquire()
{
  static bool done = false;
  if (done) return;
  done = true;
  const char* home = getenv("ECCE_REALUSERHOME");
  string key = Ecce::sessionKey();
  string name;
  if (!home || !*home || key.empty() || !isEcceProgram(resolvedSelf(), name))
    return;
  string dir = string(home) + "/.ECCE/leases/" + key;
  string path = dir + "/" + name + "." + std::to_string((long)getpid());
  // A reader that finds the file before it is locked takes it for stale
  // and removes it; then the lock is on a file nobody can see, so check
  // that the path still names it, and start again if not. The descriptor
  // stays open for the life of the process; exec closes it.
  for (int attempt = 0; attempt < 20; attempt++) {
    mkdirs(dir);
    // A reader also removes an empty directory, between mkdirs and open.
    int fd = open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    if (fd >= 0) {
      if (flock(fd, LOCK_EX | LOCK_NB) == 0 && sameFile(fd, path)) return;
      close(fd);
    }
    usleep(10000);
  }
}

vector<Holder> live(const string& statedir, const string& key)
{
  vector<Holder> found;
  string base = statedir + "/leases";
  string prefix = Ecce::sessionKeyFor("0000000000000000");
  prefix.erase(prefix.size() - 16);       // <host>_
  vector<string> keys;
  if (!key.empty()) {
    keys.push_back(key);
  } else if (DIR* d = opendir(base.c_str())) {
    while (struct dirent* e = readdir(d)) {
      string k = e->d_name;
      if (k.compare(0, prefix.size(), prefix) == 0 &&
          k.size() == prefix.size() + 16)
        keys.push_back(k);
    }
    closedir(d);
  }
  for (size_t i = 0; i < keys.size(); i++) {
    string dir = base + "/" + keys[i];
    DIR* d = opendir(dir.c_str());
    if (!d) continue;
    while (struct dirent* e = readdir(d)) {
      string file = e->d_name;
      size_t dot = file.rfind('.');
      if (file[0] == '.' || dot == string::npos || dot == 0) continue;
      string path = dir + "/" + file;
      int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
      if (fd < 0) continue;
      if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
        if (errno == EWOULDBLOCK) {
          Holder h;
          h.key = keys[i];
          h.name = file.substr(0, dot);
          h.pid = atol(file.c_str() + dot + 1);
          found.push_back(h);
        }
      } else {
        unlink(path.c_str());               // stale: its holder has gone
      }
      close(fd);
    }
    closedir(d);
    rmdir(dir.c_str());                     // only succeeds when empty
  }
  return found;
}

}  // namespace SessionLease
