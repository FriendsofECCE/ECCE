#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <map>
#include <vector>

#include "util/Ecce.H"
#include "util/LocalData.H"
#include "util/PreferenceLabels.H"
#include "util/Preferences.H"

namespace {

// Every process that opens the folder holds a shared lock on this file
// until it exits; a move needs the exclusive one.
const char *IN_USE_FILE = ".ecce-in-use";

string trim(const string& in)
{
  string s = in;
  while (s.size() > 1 && s[s.size()-1] == '/') s.erase(s.size()-1);
  return s;
}

string expand(const string& in)
{
  if (in == "~" || in.compare(0, 2, "~/") == 0)
    return Ecce::realUserHome() + in.substr(1);
  return in;
}

bool isDir(const string& p)
{
  struct stat st;
  return stat(p.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

void mkdirs(const string& path, mode_t mode)
{
  string sofar;
  size_t pos = 0;
  while (pos != string::npos) {
    pos = path.find('/', pos + 1);
    sofar = path.substr(0, pos);
    if (!sofar.empty()) mkdir(sofar.c_str(), mode);
  }
}

// Relative path -> size (-1 for a directory, -2 for a symlink).
typedef std::map<string, long long> Listing;

bool list(const string& root, const string& rel, Listing& out)
{
  string path = rel.empty() ? root : root + "/" + rel;
  DIR *d = opendir(path.c_str());
  if (!d) return false;
  bool ok = true;
  struct dirent *e;
  while ((e = readdir(d)) != 0) {
    string name = e->d_name;
    if (name == "." || name == "..") continue;
    string r = rel.empty() ? name : rel + "/" + name;
    struct stat st;
    if (lstat((root + "/" + r).c_str(), &st) != 0) { ok = false; continue; }
    if (S_ISDIR(st.st_mode)) {
      out[r] = -1;
      if (!list(root, r, out)) ok = false;
    } else if (S_ISLNK(st.st_mode)) {
      out[r] = -2;
    } else {
      out[r] = st.st_size;
    }
  }
  closedir(d);
  return ok;
}

bool copyOne(const string& from, const string& to, const struct stat& st)
{
  if (S_ISLNK(st.st_mode)) {
    char buf[4096];
    ssize_t n = readlink(from.c_str(), buf, sizeof(buf) - 1);
    if (n < 0) return false;
    buf[n] = 0;
    return symlink(buf, to.c_str()) == 0;
  }
  int in = open(from.c_str(), O_RDONLY);
  if (in < 0) return false;
  int out = open(to.c_str(), O_WRONLY | O_CREAT | O_EXCL, st.st_mode & 07777);
  if (out < 0) { close(in); return false; }
  bool ok = true;
  char buf[65536];
  ssize_t n;
  while ((n = read(in, buf, sizeof(buf))) > 0) {
    if (write(out, buf, n) != n) { ok = false; break; }
  }
  if (n < 0) ok = false;
  close(in);
  if (close(out) != 0) ok = false;
  struct timespec times[2] = { st.st_atim, st.st_mtim };
  utimensat(AT_FDCWD, to.c_str(), times, 0);
  return ok;
}

bool copyTree(const string& from, const string& to)
{
  struct stat st;
  if (lstat(from.c_str(), &st) != 0) return false;
  if (!S_ISDIR(st.st_mode)) return copyOne(from, to, st);
  if (mkdir(to.c_str(), st.st_mode & 07777) != 0 && errno != EEXIST)
    return false;
  DIR *d = opendir(from.c_str());
  if (!d) return false;
  bool ok = true;
  struct dirent *e;
  while (ok && (e = readdir(d)) != 0) {
    string name = e->d_name;
    if (name == "." || name == "..") continue;
    ok = copyTree(from + "/" + name, to + "/" + name);
  }
  closedir(d);
  return ok;
}

bool removeTree(const string& path)
{
  struct stat st;
  if (lstat(path.c_str(), &st) != 0) return errno == ENOENT;
  if (!S_ISDIR(st.st_mode)) return unlink(path.c_str()) == 0;
  DIR *d = opendir(path.c_str());
  if (!d) return false;
  bool ok = true;
  struct dirent *e;
  while ((e = readdir(d)) != 0) {
    string name = e->d_name;
    if (name == "." || name == "..") continue;
    if (!removeTree(path + "/" + name)) ok = false;
  }
  closedir(d);
  return rmdir(path.c_str()) == 0 && ok;
}

} // namespace


string LocalData::defaultDir()
{
  return string(Ecce::realUserHome()) + "/.ECCE-local";
}

string LocalData::dir()
{
  string ret;
  const char *env = getenv("ECCE_LOCAL_DATA");
  if (env) {
    ret = *env ? trim(expand(env)) : "";
  } else if (getenv("ECCE_REMOTE_SERVER")) {
    ret = "";
  } else {
    // Read once: a process stays in the mode it started in.
    static bool read = false;
    static string fromPref;
    if (!read) {
      read = true;
      if (prefEnabled()) fromPref = prefFolder();
    }
    ret = fromPref;
  }
  if (!ret.empty()) hold(ret);
  return ret;
}

string LocalData::userHome()
{
  string d = dir();
  if (d.empty()) return "";
  return d + "/users/" + Ecce::serverUser();
}

bool LocalData::prefEnabled()
{
  Preferences pref(PrefLabels::GLOBALPREFFILE);
  bool on = false;
  return pref.getBool(PrefLabels::LOCALDATA, on) && on;
}

string LocalData::prefFolder()
{
  Preferences pref(PrefLabels::GLOBALPREFFILE);
  string folder;
  pref.getString(PrefLabels::LOCALDATAFOLDER, folder);
  return folder.empty() ? defaultDir() : trim(expand(folder));
}

string LocalData::prefMoveTo()
{
  Preferences pref(PrefLabels::GLOBALPREFFILE);
  string to;
  pref.getString(PrefLabels::LOCALDATAMOVETO, to);
  return to.empty() ? "" : trim(expand(to));
}

void LocalData::setPref(bool enabled, const string& folder,
                        const string& moveTo)
{
  Preferences pref(PrefLabels::GLOBALPREFFILE);
  pref.setBool(PrefLabels::LOCALDATA, enabled);
  if (folder.empty() || trim(folder) == defaultDir())
    pref.remove_entry(PrefLabels::LOCALDATAFOLDER);
  else
    pref.setString(PrefLabels::LOCALDATAFOLDER, trim(folder));
  if (moveTo.empty())
    pref.remove_entry(PrefLabels::LOCALDATAMOVETO);
  else
    pref.setString(PrefLabels::LOCALDATAMOVETO, trim(moveTo));
  pref.saveFile();
}

namespace {
  // The folder this process holds, and its lock.  Built on first use:
  // dir() can run from another file's static initialiser.
  string& heldPath() { static string s; return s; }
  int& heldLock() { static int fd = -1; return fd; }
}

void LocalData::hold(const string& path)
{
  int& fd = heldLock();
  string& held = heldPath();
  if (held == path) return;
  if (fd >= 0) close(fd);
  mkdirs(path, 0700);
  fd = open((path + "/" + IN_USE_FILE).c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
  if (fd >= 0) {
    while (flock(fd, LOCK_SH) != 0 && errno == EINTR) {}
  }
  held = path;
}

bool LocalData::isEmptyOrMissing(const string& path)
{
  if (!isDir(path)) {
    struct stat st;
    return lstat(path.c_str(), &st) != 0;
  }
  Listing l;
  list(path, "", l);
  l.erase(IN_USE_FILE);
  return l.empty();
}

bool LocalData::inUse(const string& path)
{
  int fd = open((trim(path) + "/" + IN_USE_FILE).c_str(), O_RDWR | O_CLOEXEC);
  if (fd < 0) return false;
  bool busy = flock(fd, LOCK_EX | LOCK_NB) != 0 && errno == EWOULDBLOCK;
  close(fd);
  return busy;
}

LocalData::MoveResult LocalData::move(const string& fromIn, const string& toIn,
                                      string& msg)
{
  string from = trim(expand(fromIn)), to = trim(expand(toIn));
  msg = "";
  if (from == to) return SAME;
  if (to.compare(0, from.size() + 1, from + "/") == 0) {
    msg = to + " is inside " + from + ".";
    return FAILED;
  }
  if (!isDir(from)) {
    msg = from + " does not exist.";
    return FAILED;
  }
  if (!isEmptyOrMissing(to)) {
    msg = to + " is not empty.";
    return TARGET_NOT_EMPTY;
  }
  // This process's own hold (the Gateway's, which asks) is not a user.
  if (heldPath() == from && heldLock() >= 0) {
    close(heldLock());
    heldLock() = -1;
    heldPath() = "";
  }
  // The exclusive lock is held for the whole move, so a session starting
  // meanwhile waits rather than opening a half-moved folder.
  int fd = open((from + "/" + IN_USE_FILE).c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
  if (fd < 0 || flock(fd, LOCK_EX | LOCK_NB) != 0) {
    if (fd >= 0) close(fd);
    msg = "ECCE is still using " + from +
          " (another session, or a job being followed).";
    return IN_USE;
  }

  string parent = to.substr(0, to.rfind('/'));
  if (!parent.empty()) mkdirs(parent, 0755);
  // An empty target directory is replaced; rename() does that itself.
  if (isDir(to)) {
    string lock = to + "/" + IN_USE_FILE;
    unlink(lock.c_str());
  }
  if (rename(from.c_str(), to.c_str()) == 0) {
    close(fd);
    return MOVED;
  }
  if (errno != EXDEV) {
    msg = string("Cannot move ") + from + " to " + to + ": " + strerror(errno);
    close(fd);
    return FAILED;
  }

  bool targetExisted = isDir(to);
  Listing before, after;
  bool ok = list(from, "", before) && copyTree(from, to) && list(to, "", after);
  if (ok && before != after) {
    ok = false;
    msg = "The copy in " + to + " does not match " + from +
          " (a file is missing or has a different size).";
  } else if (!ok) {
    msg = string("Copying ") + from + " to " + to + " failed: " +
          strerror(errno) + ".";
  }
  if (!ok) {
    if (targetExisted) {
      Listing l;
      list(to, "", l);
      for (Listing::reverse_iterator it = l.rbegin(); it != l.rend(); ++it)
        removeTree(to + "/" + it->first);
    } else {
      removeTree(to);
    }
    close(fd);
    msg += " Your calculations are still in " + from + ".";
    return FAILED;
  }
  close(fd);
  if (!removeTree(from)) {
    msg = "Your calculations were copied to " + to + ", but " + from +
          " could not be removed completely; delete it yourself.";
  }
  return MOVED;
}
