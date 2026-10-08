#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>

#include "util/Ecce.H"
#include "util/PosixCompat.H"
#include "util/WaitingJobs.H"

using std::string;
using std::vector;

namespace {

// FNV-1a: a file name for any URL, stable across processes and builds.
string hashOf(const string& url)
{
  unsigned long long h = 1469598103934665603ULL;
  for (size_t i = 0; i < url.size(); i++) {
    h ^= (unsigned char)url[i];
    h *= 1099511628211ULL;
  }
  char buf[17];
  snprintf(buf, sizeof(buf), "%016llx", h);
  return buf;
}

string path(const string& url, const char* ext)
{
  return WaitingJobs::dir() + "/" + hashOf(url) + ext;
}

bool ensureDir()
{
  string d = WaitingJobs::dir();
  string parent = d.substr(0, d.rfind('/'));
  (void)mkdir(parent.c_str(), 0700);
  return mkdir(d.c_str(), 0700) == 0 || errno == EEXIST;
}

}


string WaitingJobs::dir()
{
  string pref = Ecce::realUserPrefPath();
  while (!pref.empty() && pref[pref.size() - 1] == '/')
    pref.resize(pref.size() - 1);
  return pref + "/waiting";
}


bool WaitingJobs::add(const string& url)
{
  if (url.empty() || !ensureDir())
    return false;
  // Written aside and renamed, so a reader never sees half a URL.
  string target = path(url, ".job");
  string tmp = target + ".tmp";
  {
    std::ofstream out(tmp.c_str(), std::ios::trunc);
    if (!out)
      return false;
    out << url << "\n";
    if (!out)
      return false;
  }
  return renameReplace(tmp.c_str(), target.c_str()) == 0;
}


void WaitingJobs::remove(const string& url)
{
  if (!url.empty())
    (void)unlink(path(url, ".job").c_str());
}


vector<string> WaitingJobs::list()
{
  vector<std::pair<time_t, string> > found;
  DIR* d = opendir(dir().c_str());
  if (!d)
    return vector<string>();
  struct dirent* e;
  while ((e = readdir(d)) != 0) {
    string name = e->d_name;
    if (name.size() < 5 || name.compare(name.size() - 4, 4, ".job") != 0)
      continue;
    string file = dir() + "/" + name;
    std::ifstream in(file.c_str());
    string url;
    if (!std::getline(in, url) || url.empty())
      continue;
    struct stat st;
    found.push_back(std::make_pair(stat(file.c_str(), &st) == 0 ? st.st_mtime
                                                                : (time_t)0,
                                   url));
  }
  closedir(d);
  std::sort(found.begin(), found.end());
  vector<string> urls;
  for (size_t i = 0; i < found.size(); i++)
    urls.push_back(found[i].second);
  return urls;
}


int WaitingJobs::watch(const string& url)
{
  if (url.empty() || !ensureDir())
    return -1;
  int fd = open(path(url, ".lock").c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
  if (fd < 0)
    return -1;
  if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
    close(fd);
    return -1;
  }
  return fd;
}


void WaitingJobs::unwatch(const string& url, int fd)
{
  if (fd < 0)
    return;
  (void)unlink(path(url, ".lock").c_str());
  close(fd);
}


bool WaitingJobs::watched(const string& url)
{
  int fd = open(path(url, ".lock").c_str(), O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    return false;
  bool held = flock(fd, LOCK_SH | LOCK_NB) != 0;
  close(fd);
  return held;
}


bool WaitingJobs::firstInSession(const string& key)
{
  if (key.empty() || !ensureDir())
    return true;
  string marker = dir() + "/session";
  std::ifstream in(marker.c_str());
  string seen;
  if (std::getline(in, seen) && seen == key)
    return false;
  std::ofstream out(marker.c_str(), std::ios::trunc);
  out << key << "\n";
  return true;
}
