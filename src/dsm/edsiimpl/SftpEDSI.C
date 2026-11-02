#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <memory>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>

#include "comm/RCommand.H"
#include "dsm/FileEDSI.H"
#include "dsm/SftpEDSI.H"
#include "util/ServerList.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/VDoc.H"
#include "util/ResourceUtils.H"

// The mirror keeps the server's modification times; utime() is the call
// MinGW has as well (utimes() is POSIX only).
static void putEnv(const char* name, const char* value)
{
#ifdef _WIN32
  _putenv_s(name, value ? value : "");   // "" removes it on Windows
#else
  if (value) setenv(name, value, 1); else unsetenv(name);
#endif
}

static void setMtime(const std::string& path, long t)
{
  struct utimbuf ub;
  ub.actime = ub.modtime = (time_t)t;
  utime(path.c_str(), &ub);
}

namespace fs = std::filesystem;

namespace {

// RCommand is not thread safe and a login is worth keeping between the
// operations of one process (without ECCE's master each would log in again).
std::mutex gMutex;
std::map<string, RCommand*> gLogins;
unsigned gCounter = 0;
// Prompts are only for the Organizer's connect step.  Every other process
// (job store, monitor, editors) rides the session's master or fails, so a
// launch never stops on a hidden password prompt.
bool gInteractive = false;

// host is the URL's authority, host[:port]: a login (and its master and
// stored password) belongs to one port.
RCommand* newLogin(const string& authority, const string& user)
{
  string host;
  int port = 22;
  if (!ServerList::splitHostPort(authority, host, port)) host = authority;
  string saved;
  const char* e = getenv("ECCE_ASKPASS");
  const bool had = e != 0;
  if (e) saved = e;
  if (!gInteractive) putEnv("ECCE_ASKPASS", "/bin/false");
  RCommand* rc = new RCommand(host, "ssh", "bash", user, "", "", "", "", "", "",
                              port == 22 ? 0 : port);
  if (!gInteractive) {
    putEnv("ECCE_ASKPASS", had ? saved.c_str() : 0);
  }
  return rc;
}

string q(const string& s)
{
  string o = "'";
  for (char c : s) { if (c == '\'') o += "'\\''"; else o += c; }
  return o + "'";
}

string trimSlash(string p)
{
  while (p.size() > 1 && p.back() == '/') p.pop_back();
  return p;
}

string parentOf(const string& path)
{
  string p = trimSlash(path);
  size_t pos = p.rfind('/');
  if (pos == string::npos) return ".";
  return pos == 0 ? "/" : p.substr(0, pos);
}

string b64(const string& in)
{
  static const char *t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  string o;
  size_t i = 0;
  for (; i + 2 < in.size(); i += 3) {
    unsigned v = ((unsigned char)in[i] << 16) | ((unsigned char)in[i+1] << 8) | (unsigned char)in[i+2];
    o += t[v >> 18]; o += t[(v >> 12) & 63]; o += t[(v >> 6) & 63]; o += t[v & 63];
  }
  if (i + 1 == in.size()) {
    unsigned v = (unsigned char)in[i] << 16;
    o += t[v >> 18]; o += t[(v >> 12) & 63]; o += "==";
  } else if (i + 2 == in.size()) {
    unsigned v = ((unsigned char)in[i] << 16) | ((unsigned char)in[i+1] << 8);
    o += t[v >> 18]; o += t[(v >> 12) & 63]; o += t[(v >> 6) & 63]; o += '=';
  }
  return o;
}

string unb64(const string& in)
{
  string o;
  unsigned v = 0; int bits = 0;
  for (char c : in) {
    int d;
    if (c >= 'A' && c <= 'Z') d = c - 'A';
    else if (c >= 'a' && c <= 'z') d = c - 'a' + 26;
    else if (c >= '0' && c <= '9') d = c - '0' + 52;
    else if (c == '+') d = 62;
    else if (c == '/') d = 63;
    else continue;
    v = (v << 6) | d; bits += 6;
    if (bits >= 8) { bits -= 8; o += (char)((v >> bits) & 0xff); }
  }
  return o;
}

bool readFile(const string& p, string& out)
{
  std::ifstream in(p.c_str(), std::ios::binary);
  if (!in) return false;
  std::ostringstream ss;
  ss << in.rdbuf();
  out = ss.str();
  return true;
}

bool writeFile(const string& p, const string& data)
{
  std::ofstream out(p.c_str(), std::ios::binary | std::ios::trunc);
  out << data;
  out.close();
  return !!out;
}

// One script on the login of user@host.  A lost connection is retried once
// on a fresh login; a script that fails or goes silent for the timeout is
// reported, never waited for.
bool runRemote(const string& user, const string& host, const string& script,
               string& out, string& err, int timeout = 60)
{
  std::lock_guard<std::mutex> g(gMutex);
  const string key = user + "@" + host;
  for (int attempt = 0; attempt < 2; attempt++) {
    RCommand *&rc = gLogins[key];
    if (!rc || !rc->isOpen()) {
      delete rc;
      rc = newLogin(host, user);
      if (!rc->isOpen()) {
        err = "Cannot log in to " + key + ": " + rc->commError();
        delete rc; rc = 0;
        return false;
      }
    }
    if (rc->execout(script, out, "", timeout)) {
      out.erase(std::remove(out.begin(), out.end(), '\r'), out.end());
      return true;
    }
    err = rc->commError();
    bool timedOut = rc->timedOut();
    bool open = rc->isOpen();
    if (open || timedOut) {
      if (timedOut) { delete rc; rc = 0; err = "No answer from " + key + " (" + err + ")"; }
      return false;
    }
    delete rc; rc = 0;
    err = "Connection to " + key + " lost: " + err;
  }
  return false;
}

bool copyRemote(const string& user, const string& host, bool put,
                const string& local, const string& remote, string& err)
{
  std::lock_guard<std::mutex> g(gMutex);
  const string key = user + "@" + host;
  RCommand *&rc = gLogins[key];
  if (!rc || !rc->isOpen()) {
    delete rc;
    rc = newLogin(host, user);
    if (!rc->isOpen()) {
      err = "Cannot log in to " + key + ": " + rc->commError();
      delete rc; rc = 0;
      return false;
    }
  }
  vector<string> files(1, put ? local : remote);
  bool ok = rc->copyOnLogin(put, files, put ? remote : local, err);
  if (!ok && err.empty()) err = "copy to/from " + key + " failed";
  return ok;
}

const char *MIRROR_FUNCS =
  "st() { stat -c '%s %Y' -- \"$1\" 2>/dev/null || stat -f '%z %m' -- \"$1\" 2>/dev/null || echo '0 0'; }\n"
  "sidecar() { if [ -f \"$1/.ecce-meta\" ]; then printf 'M\\t%s\\t%s\\n' \"$1\" \"$(base64 < \"$1/.ecce-meta\" | tr -d '\\n')\"; fi; }\n"
  "walk() { d=$1; dep=$2; sidecar \"$d\"\n"
  "  for e in \"$d\"/* \"$d\"/.[!.]* \"$d\"/..?*; do\n"
  "    [ -e \"$e\" ] || [ -L \"$e\" ] || continue\n"
  "    case ${e##*/} in .ecce-meta*) continue;; esac\n"
  "    if [ -d \"$e\" ]; then printf 'E\\td\\t%s\\t%s\\n' \"$(st \"$e\")\" \"$e\"\n"
  "      if [ \"$dep\" -gt 1 ]; then walk \"$e\" $((dep-1)); else sidecar \"$e\"; fi\n"
  "    else printf 'E\\tf\\t%s\\t%s\\n' \"$(st \"$e\")\" \"$e\"; fi\n"
  "  done; }\n";

} // namespace

// One operation: a scratch mirror of the directories it touches, the
// server-side locks, and the remote changes the mirror shows afterwards.
class SftpOp {
  public:
    SftpOp(SftpEDSI& e) : p_e(e), p_ok(true)
    {
      string path;
      p_ok = SftpEDSI::parseURL(e.getURL().toString(false), p_user, p_host, path);
      p_path = trimSlash(path);
      p_base = "sftp://" + (p_user.empty() ? string() : p_user + "@") + p_host;
      const char *td = getenv("TMPDIR");
      string tmpl = string(td && *td ? td : "/tmp") + "/ecce-sftp-XXXXXX";
      vector<char> buf(tmpl.begin(), tmpl.end()); buf.push_back(0);
      if (mkdtemp(&buf[0])) p_scratch = &buf[0];
      else p_ok = false;
      if (!p_ok) p_e.m_msgStack.add("UNABLE_TO_COMPLETE_REQUEST", "Not a usable sftp URL or no scratch directory");
    }
    ~SftpOp()
    {
      release();
      if (!p_scratch.empty()) { std::error_code ec; fs::remove_all(p_scratch, ec); }
    }
    bool ok() const { return p_ok; }
    const string& path() const { return p_path; }
    string local(const string& remote) const { return p_scratch + remote; }

    string toRemoteURL(const string& s) const
    {
      string t = s;
      if (t.compare(0, 7, "file://") == 0) t = t.substr(7);
      if (t.compare(0, p_scratch.size(), p_scratch) == 0) return p_base + t.substr(p_scratch.size());
      return s;
    }
    string toRemotePath(const string& s) const
    {
      string t = s;
      if (t.compare(0, 7, "file://") == 0) t = t.substr(7);
      if (t.compare(0, p_scratch.size(), p_scratch) == 0) return t.substr(p_scratch.size());
      return t;
    }
    // The url of another resource on this server.
    EcceURL remoteURL(const string& path) const { return EcceURL(p_base + path); }

    bool fail(const string& what)
    {
      p_e.m_msgStack.add("UNABLE_TO_COMPLETE_REQUEST", what.c_str());
      return false;
    }
    bool run(const string& script, string& out, int timeout = 60)
    {
      string err;
      if (runRemote(p_user, p_host, script, out, err, timeout)) return true;
      fail(err);
      return false;
    }

    // Takes <dir>/.ecce-meta.lockdir for each directory, in name order.  A
    // mode of "auto" locks the path if it is a directory and else its
    // parent, which is the directory whose sidecar describes it.  A
    // directory that is missing or not writable is skipped: the operation
    // then fails on its own with the usual message.  A lock older than two
    // minutes is taken as left by a dead client.
    bool lock(const vector<string>& dirs, bool autoMode)
    {
      if (getenv("ECCE_SFTP_NOLOCK")) return true;   // test only: shows the lock is what keeps updates
      std::set<string> sorted(dirs.begin(), dirs.end());
      string script = "lockone() { d=$1; n=0\n"
        "  while :; do\n"
        "    if mkdir \"$d/.ecce-meta.lockdir\" 2>/dev/null; then printf 'L\\t%s\\n' \"$d\"; return 0; fi\n"
        "    [ -d \"$d\" ] || return 0\n"
        "    [ -w \"$d\" ] || return 0\n"
        "    if [ -n \"$(find \"$d/.ecce-meta.lockdir\" -maxdepth 0 -mmin +2 2>/dev/null)\" ]; then\n"
        "      mv \"$d/.ecce-meta.lockdir\" \"$d/.ecce-meta.lockdir.old.$$\" 2>/dev/null && rmdir \"$d/.ecce-meta.lockdir.old.$$\" 2>/dev/null; continue; fi\n"
        "    n=$((n+1)); if [ $n -gt 150 ]; then printf 'B\\t%s\\n' \"$d\"; return 1; fi\n"
        "    sleep 0.2 2>/dev/null || sleep 1\n"
        "  done; }\n";
      for (const string& d : sorted) {
        if (autoMode)
          script += "if [ -d " + q(d) + " ]; then t=" + q(d) + "; else t=" + q(parentOf(d)) +
                    "; fi\nlockone \"$t\" || exit 0\n";
        else
          script += "lockone " + q(d) + " || exit 0\n";
      }
      string out;
      if (!run(script, out, 90)) return false;
      std::istringstream in(out);
      string line;
      bool busy = false;
      while (std::getline(in, line)) {
        if (line.compare(0, 2, "L\t") == 0) p_locks.push_back(line.substr(2));
        else if (line.compare(0, 2, "B\t") == 0) busy = true;
      }
      if (busy) { release(); return fail("Locked by another client: " + out); }
      return true;
    }
    void release()
    {
      if (p_locks.empty()) return;
      string script, out;
      for (const string& d : p_locks) script += "rmdir " + q(d + "/.ecce-meta.lockdir") + " 2>/dev/null\n";
      script += "true\n";
      string err;
      runRemote(p_user, p_host, script, out, err, 30);   // stale after 2 min if this fails
      p_locks.clear();
    }

    // Stubs of what is on the server below path (path's parent when it is not
    // a directory) go into the scratch directory, with the real sidecars:
    // same names, sizes and times, no content.  kind is d, f or x (absent).
    bool mirror(const string& path, int depth, char *kind = 0)
    {
      string script = string(MIRROR_FUNCS) + "P=" + q(path) + "\n"
        "if [ -d \"$P\" ]; then printf 'R\\td\\n'; printf 'E\\td\\t%s\\t%s\\n' \"$(st \"$P\")\" \"$P\"; [ " + std::to_string(depth) + " -ge 1 ] && walk \"$P\" " + std::to_string(depth) + "\n"
        "else\n"
        "  if [ -e \"$P\" ] || [ -L \"$P\" ]; then printf 'R\\tf\\n'; else printf 'R\\tx\\n'; fi\n"
        "  if [ -e \"$P\" ] || [ -L \"$P\" ]; then printf 'E\\tf\\t%s\\t%s\\n' \"$(st \"$P\")\" \"$P\"; fi\n"
        "  par=${P%/*}; [ -z \"$par\" ] && par=/\n"
        "  if [ -d \"$par\" ]; then printf 'E\\td\\t%s\\t%s\\n' \"$(st \"$par\")\" \"$par\"; [ " + std::to_string(depth) + " -ge 1 ] && walk \"$par\" 1; fi\n"
        "fi\ntrue\n";
      string out;
      if (!run(script, out)) return false;
      std::istringstream in(out);
      string line;
      vector<std::pair<string, long>> dirTimes;
      char k = 0;
      while (std::getline(in, line)) {
        vector<string> f;
        size_t a = 0;
        for (int i = 0; i < 3; i++) {
          size_t b = line.find('\t', a);
          if (b == string::npos) break;
          f.push_back(line.substr(a, b - a));
          a = b + 1;
        }
        f.push_back(line.substr(a));
        if (f[0] == "R" && f.size() >= 2) k = f[1][0];
        else if (f[0] == "M" && f.size() >= 3) {
          string dir = f[1], data = unb64(f[2]);
          fs::create_directories(local(dir));
          writeFile(local(dir) + "/.ecce-meta", data);
          p_before[dir] = data;
        } else if (f[0] == "E" && f.size() == 4) {
          long size = 0, mtime = 0;
          sscanf(f[2].c_str(), "%ld %ld", &size, &mtime);
          string lp = local(f[3]);
          if (f[1] == "d") {
            fs::create_directories(lp);
            dirTimes.push_back(std::make_pair(lp, mtime));
            p_entries[f[3]] = 'd';
          } else {
            fs::create_directories(fs::path(lp).parent_path());
            std::ofstream(lp.c_str()).close();
            std::error_code ec;
            fs::resize_file(lp, (std::uintmax_t)size, ec);
            setMtime(lp, mtime);
            p_entries[f[3]] = 'f';
          }
        }
      }
      for (size_t i = dirTimes.size(); i-- > 0;) {
        setMtime(dirTimes[i].first, dirTimes[i].second);
      }
      if (kind) *kind = k;
      if (!k) return fail("No answer from " + p_host);
      // The top directory exists on the server even if only a stub.
      fs::create_directories(fs::path(local(path)).parent_path());
      return true;
    }

    // The real content of a file, in place of its stub.
    bool download(const string& path)
    {
      string err;
      fs::create_directories(fs::path(local(path)).parent_path());
      if (!copyRemote(p_user, p_host, false, local(path), path, err)) return fail(err);
      return true;
    }
    bool upload(const string& localFile, const string& remote)
    {
      string err;
      if (!copyRemote(p_user, p_host, true, localFile, remote, err)) return fail(err);
      return true;
    }
    string tmpName(const string& remote, const char *tag)
    {
      return parentOf(remote) + "/." + tag + "." + std::to_string(getpid()) + "." +
             std::to_string(++gCounter) + "." + remote.substr(remote.rfind('/') + 1);
    }
    string scratchFile(const string& name) { return p_scratch + "/.tmp-" + name; }

    // What the inner operation changed in the mirror, as a remote script.
    // Sidecars always; created and deleted entries when structural (a
    // removal or a new name), not for move and copy, which the server does
    // itself with mv and cp -R before this runs (skipRoots are theirs).
    bool apply(bool structural, const vector<string>& skipRoots, const string& pre = "")
    {
      std::map<string, char> after;
      std::map<string, string> sidecars;
      std::error_code ec;
      for (fs::recursive_directory_iterator it(p_scratch, ec), end; !ec && it != end; it.increment(ec)) {
        string rp = it->path().string().substr(p_scratch.size());
        string name = it->path().filename().string();
        if (name.compare(0, 6, ".tmp-") == 0 && it->path().parent_path() == p_scratch) continue;
        if (name == ".ecce-meta") {
          string data;
          readFile(it->path().string(), data);
          sidecars[parentOf(rp)] = data;
          continue;
        }
        if (name.compare(0, 11, ".ecce-meta.") == 0) continue;
        after[rp] = it->is_directory() ? 'd' : 'f';
      }
      auto skipped = [&](const string& p) {
        for (const string& s : skipRoots)
          if (p == s || p.compare(0, s.size() + 1, s + "/") == 0) return true;
        return false;
      };
      string script = pre;
      if (structural) {
        for (auto& e : p_entries)
          if (!after.count(e.first) && !skipped(e.first)) script += "rm -rf -- " + q(e.first) + "\n";
        for (auto& e : after)
          if (!p_entries.count(e.first) && !skipped(e.first))
            script += e.second == 'd' ? "mkdir -p -- " + q(e.first) + "\n"
                                     : "[ -e " + q(e.first) + " ] || : > " + q(e.first) + "\n";
      }
      for (auto& s : sidecars) {
        if (skipped(s.first)) continue;
        auto b = p_before.find(s.first);
        if (b != p_before.end() && b->second == s.second) continue;
        string sc = s.first == "/" ? "/.ecce-meta" : s.first + "/.ecce-meta";
        string tmp = s.first + "/.ecce-meta.new." + std::to_string(getpid());
        script += "printf %s " + q(b64(s.second)) + " | base64 -d > " + q(tmp) + " && mv -f " + q(tmp) + " " + q(sc) + "\n";
      }
      for (auto& b : p_before) {
        if (sidecars.count(b.first) || skipped(b.first)) continue;
        if (after.count(b.first) || b.first == p_path || p_entries.count(b.first))
          script += "rm -f " + q(b.first + "/.ecce-meta") + "\n";
      }
      if (script.empty()) return true;
      string out;
      if (!run("(\n" + script + "\n) && echo APPLIED\n", out)) return false;
      if (out.find("APPLIED") == string::npos) return fail("Remote update failed: " + out);
      return true;
    }

    FileEDSI *inner(const string& remotePath) { return new FileEDSI(EcceURL(local(remotePath))); }
    void takeMessages(FileEDSI& f) { p_e.m_msgStack.clear(); p_e.m_msgStack.add(f.m_msgStack); }

    SftpEDSI& p_e;
    bool p_ok;
    string p_user, p_host, p_path, p_base, p_scratch;
    vector<string> p_locks;
    std::map<string, string> p_before;      // sidecar content by directory
    std::map<string, char> p_entries;       // mirrored entries
};

// ---------------------------------------------------------------------------

namespace {

bool isVDocSidecar(const string& data, const string& key)
{
  string want = VDoc::getEcceNamespace() + ":resourcetype";
  std::istringstream in(data);
  string line;
  while (std::getline(in, line)) {
    size_t a = line.find('\t');
    size_t b = a == string::npos ? a : line.find('\t', a + 1);
    size_t c = b == string::npos ? b : line.find('\t', b + 1);
    if (c == string::npos || line.substr(0, a) != key || line.substr(a + 1, b - a - 1) != want) continue;
    return ResourceUtils::stringToResourceType(line.substr(c + 1)) ==
           ResourceDescriptor::RT_VIRTUAL_DOCUMENT;
  }
  return false;
}

typedef std::unique_ptr<FileEDSI> InnerPtr;

}

bool SftpEDSI::parseURL(const string& url, string& user, string& host, string& path)
{
  if (url.compare(0, 7, "sftp://") != 0) return false;
  size_t slash = url.find('/', 7);
  string auth = url.substr(7, slash == string::npos ? string::npos : slash - 7);
  path = slash == string::npos ? "/" : url.substr(slash);
  size_t at = auth.rfind('@');
  user = at == string::npos ? "" : auth.substr(0, at);
  host = at == string::npos ? auth : auth.substr(at + 1);
  // The port stays in host (gLogins and messages tell ports apart); 22 is
  // dropped so that host and host:22 are one login.
  string bare;
  int port;
  if (!ServerList::splitHostPort(host, bare, port)) return false;
  host = ServerList::hostPort(bare, port);
  return !host.empty();
}

void SftpEDSI::setInteractive(bool on) { std::lock_guard<std::mutex> g(gMutex); gInteractive = on; }

SftpEDSI::Probe SftpEDSI::probe(const string& user, const string& host,
                                const string& folder, bool create, string& message)
{
  const string who = user + "@" + host;
  string out, err;
  const string d = q(folder);
  setInteractive(true);
  bool ran = runRemote(user, host, "d=" + d + "\n" + (create ? "[ -e \"$d\" ] || mkdir -p \"$d\" 2>/dev/null\n" : "") +
                       "if [ -d \"$d\" ]; then if [ -w \"$d\" ]; then echo ECCE_OK; else echo ECCE_RO; fi; "
                       "else echo ECCE_MISSING; fi\ntrue\n", out, err, 60);
  setInteractive(false);
  if (!ran) { message = err.empty() ? "Cannot log in to " + who + "." : err; return PROBE_NO_LOGIN; }
  if (out.find("ECCE_OK") != string::npos) { message = ""; return PROBE_OK; }
  if (out.find("ECCE_RO") != string::npos) {
    message = "You can log in to " + who + ", but " + folder + " is not writable for " + user + ".";
    return PROBE_NOT_WRITABLE;
  }
  message = "You can log in to " + who + ", but the folder " + folder + " does not exist.";
  return PROBE_MISSING;
}

SftpEDSI::SftpEDSI() : EDSI(), p_authListener(0), p_progressListener(0) {}
SftpEDSI::SftpEDSI(const EcceURL& url) : EDSI(url), p_authListener(0), p_progressListener(0) {}
SftpEDSI::~SftpEDSI() {}

bool SftpEDSI::describeServerMetaData(vector<string>& metadata)
{
  metadata.clear();
  return true;
}

bool SftpEDSI::checkServer()
{
  m_msgStack.clear();
  SftpOp op(*this);
  string out;
  if (op.ok() && op.run("echo ECCE_SFTP_OK\n", out, 30) && out.find("ECCE_SFTP_OK") != string::npos)
    return true;
  m_msgStack.add("SERVER_NOT_FOUND", p_url.toString().c_str());
  return false;
}

string SftpEDSI::getClassName() { return "SftpEDSI"; }

bool SftpEDSI::isLocked(string& locker)
{
  m_msgStack.clear();
  locker = "";
  return false;
}

bool SftpEDSI::exists(const bool&)
{
  m_msgStack.clear();
  SftpOp op(*this);
  char k = 0;
  return op.ok() && op.mirror(op.path(), 0, &k) && k != 'x';
}

bool SftpEDSI::isWritable()
{
  m_msgStack.clear();
  SftpOp op(*this);
  if (!op.ok()) return false;
  string out;
  string p = q(op.path());
  return op.run("if [ -e " + p + " ]; then [ -w " + p + " ]; else [ -w " + q(parentOf(op.path())) +
                " ]; fi && echo WRITABLE\ntrue\n", out) &&
         out.find("WRITABLE") != string::npos;
}

// ----- data -----

istream* SftpEDSI::getDataSubSet(int start, int length)
{
  std::ostringstream os;
  if (!getDataSubSet(os, start, length)) return NULL;
  return new std::istringstream(os.str());
}

bool SftpEDSI::getDataSubSet(ostream& dest, int start, int length)
{
  m_msgStack.clear();
  SftpOp op(*this);
  char k = 0;
  if (!op.ok() || !op.mirror(op.path(), 1, &k)) return false;
  if (k == 'f' && !op.download(op.path())) return false;
  InnerPtr in(op.inner(op.path()));
  bool ret = in->getDataSubSet(dest, start, length);
  op.takeMessages(*in);
  return ret;
}

unsigned long SftpEDSI::getDataSetSize()
{
  m_msgStack.clear();
  SftpOp op(*this);
  if (!op.ok() || !op.mirror(op.path(), 0)) return 0;
  InnerPtr in(op.inner(op.path()));
  unsigned long n = in->getDataSetSize();
  op.takeMessages(*in);
  return n;
}

bool SftpEDSI::getDataSet(ostream& dest)
{
  m_msgStack.clear();
  SftpOp op(*this);
  char k = 0;
  if (!op.ok() || !op.mirror(op.path(), 0, &k)) return false;
  if (k == 'f' && !op.download(op.path())) return false;
  InnerPtr in(op.inner(op.path()));
  bool ret = in->getDataSet(dest);
  op.takeMessages(*in);
  return ret;
}

istream *SftpEDSI::getDataSet()
{
  std::ostringstream os;
  return getDataSet(os) ? new std::istringstream(os.str()) : NULL;
}

// A new file is built beside its name and renamed over it, so a reader
// never sees half of it.
static bool putBytes(SftpEDSI& e, SftpOp& op, const string& data)
{
  const string path = op.path();
  string lf = op.scratchFile("put");
  string tmp = op.tmpName(path, "ecce-put");
  string out;
  if (!writeFile(lf, data) || !op.upload(lf, tmp)) {
    e.m_msgStack.add("UNABLE_TO_WRITE", path.c_str());
    return false;
  }
  if (!op.run("if [ -d " + q(path) + " ]; then rm -f " + q(tmp) + "; exit 1; fi\n"
              "mv -f " + q(tmp) + " " + q(path) + " && echo PUT_OK || rm -f " + q(tmp) + "\ntrue\n", out) ||
      out.find("PUT_OK") == string::npos) {
    e.m_msgStack.add("UNABLE_TO_WRITE", path.c_str());
    return false;
  }
  return true;
}

bool SftpEDSI::putDataSet(const char *putStream)
{
  m_msgStack.clear();
  SftpOp op(*this);
  return op.ok() && putBytes(*this, op, putStream ? putStream : "");
}

bool SftpEDSI::putDataSet(istream& putStream)
{
  m_msgStack.clear();
  std::ostringstream ss;
  ss << putStream.rdbuf();
  SftpOp op(*this);
  return op.ok() && putBytes(*this, op, ss.str());
}

// Truncate bytesToOverwrite off the end, then append, in one step under the
// directory's lock, as FileEDSI does.
static bool appendBytes(SftpEDSI& e, const string& data, int overwrite)
{
  SftpOp op(e);
  if (!op.ok()) return false;
  const string path = op.path();
  string lf = op.scratchFile("app");
  string chunk = op.tmpName(path, "ecce-app");
  if (!writeFile(lf, data) || !op.upload(lf, chunk)) {
    e.m_msgStack.add("UNABLE_TO_WRITE", path.c_str());
    return false;
  }
  vector<string> dirs(1, parentOf(path));
  if (!op.lock(dirs, false)) return false;
  string f = q(path), a = q(chunk), t = q(op.tmpName(path, "ecce-new"));
  string out;
  string script =
    "f=" + f + "; a=" + a + "; t=" + t + "; n=" + std::to_string(overwrite > 0 ? overwrite : 0) + "\n"
    "size=0; [ -f \"$f\" ] && size=$(wc -c < \"$f\" | tr -d ' ')\n"
    "keep=$size; if [ $n -gt 0 ] && [ $size -gt 0 ]; then keep=$((size-n)); [ $keep -lt 0 ] && keep=0; fi\n"
    "{ if [ $keep -gt 0 ]; then head -c $keep \"$f\"; fi; cat \"$a\"; } > \"$t\" && mv -f \"$t\" \"$f\" && echo APP_OK\n"
    "rm -f \"$a\" \"$t\"\ntrue\n";
  if (!op.run(script, out) || out.find("APP_OK") == string::npos) {
    e.m_msgStack.add("UNABLE_TO_WRITE", path.c_str());
    return false;
  }
  return true;
}

bool SftpEDSI::appendDataSet(const char *putStream, int bytesToOverwrite)
{
  m_msgStack.clear();
  return appendBytes(*this, putStream ? putStream : "", bytesToOverwrite);
}

bool SftpEDSI::appendDataSet(istream& putStream, int bytesToOverwrite)
{
  m_msgStack.clear();
  std::ostringstream ss;
  ss << putStream.rdbuf();
  return appendBytes(*this, ss.str(), bytesToOverwrite);
}

// ----- listing and metadata -----

bool SftpEDSI::listCollection(vector<ResourceMetaDataResult>& result)
{
  return listCollection(vector<MetaDataRequest>(), result);
}

bool SftpEDSI::listCollection(const vector<MetaDataRequest>& requests,
                              vector<ResourceMetaDataResult>& result)
{
  m_msgStack.clear();
  SftpOp op(*this);
  if (!op.ok() || !op.mirror(op.path(), 1)) return false;
  InnerPtr in(op.inner(op.path()));
  vector<ResourceMetaDataResult> tmp;
  bool ret = in->listCollection(requests, tmp);
  op.takeMessages(*in);
  for (size_t i = 0; i < tmp.size(); i++) {
    tmp[i].url = EcceURL(op.toRemoteURL(tmp[i].url.toString(false)));
    result.push_back(tmp[i]);
  }
  return ret;
}

bool SftpEDSI::listCollection(vector<ResourceResult>& result)
{
  m_msgStack.clear();
  SftpOp op(*this);
  if (!op.ok() || !op.mirror(op.path(), 1)) return false;
  InnerPtr in(op.inner(op.path()));
  vector<ResourceResult> tmp;
  bool ret = in->listCollection(tmp);
  op.takeMessages(*in);
  for (size_t i = 0; i < tmp.size(); i++) {
    tmp[i].url = EcceURL(op.toRemoteURL(tmp[i].url.toString(false)));
    result.push_back(tmp[i]);
  }
  return ret;
}

// A calculation'"'"'s properties are gathered from everything below it, so only
// a virtual document is mirrored in depth.
static bool mirrorForMeta(SftpOp& op, bool vdocDepth)
{
  char k = 0;
  if (!op.mirror(op.path(), 1, &k)) return false;
  if (vdocDepth && k == 'd') {
    auto it = op.p_before.find(op.path());
    if (it != op.p_before.end() && isVDocSidecar(it->second, ".") &&
        !op.mirror(op.path(), 1000)) return false;
  }
  return true;
}

bool SftpEDSI::getMetaData(const vector<MetaDataRequest>& requests,
                           vector<MetaDataResult>& results, bool getVDoc)
{
  m_msgStack.clear();
  SftpOp op(*this);
  if (!op.ok() || !mirrorForMeta(op, getVDoc)) return false;
  InnerPtr in(op.inner(op.path()));
  bool ret = in->getMetaData(requests, results, getVDoc);
  op.takeMessages(*in);
  return ret;
}

bool SftpEDSI::getMetaData(const vector<MetaDataRequest>& requests,
                           ResourceMetaDataResult& results, bool getVDoc)
{
  m_msgStack.clear();
  SftpOp op(*this);
  if (!op.ok() || !mirrorForMeta(op, getVDoc)) return false;
  InnerPtr in(op.inner(op.path()));
  bool ret = in->getMetaData(requests, results, getVDoc);
  op.takeMessages(*in);
  results.url = EcceURL(op.toRemoteURL(results.url.toString(false)));
  return ret;
}

bool SftpEDSI::describeMetaData(vector<MetaDataResult>& names)
{
  m_msgStack.clear();
  SftpOp op(*this);
  if (!op.ok() || !op.mirror(op.path(), 1)) return false;
  InnerPtr in(op.inner(op.path()));
  bool ret = in->describeMetaData(names);
  op.takeMessages(*in);
  return ret;
}

// Metadata and removals: lock the directory whose sidecar is involved, look
// at the server, let FileEDSI do it on the mirror, send back what changed.
static bool mutateOne(SftpEDSI& e, bool structural,
                      const std::function<bool(FileEDSI&)>& fn)
{
  SftpOp op(e);
  if (!op.ok()) return false;
  if (!op.lock(vector<string>(1, op.path()), true)) return false;
  if (!op.mirror(op.path(), 1)) return false;
  InnerPtr in(op.inner(op.path()));
  bool ret = fn(*in);
  op.takeMessages(*in);
  return ret && op.apply(structural, vector<string>());
}

bool SftpEDSI::putMetaData(const vector<MetaDataResult>& results)
{
  m_msgStack.clear();
  return mutateOne(*this, false, [&](FileEDSI& f) { return f.putMetaData(results); });
}

bool SftpEDSI::removeMetaData(const vector<MetaDataRequest>& requests)
{
  m_msgStack.clear();
  return mutateOne(*this, false, [&](FileEDSI& f) { return f.removeMetaData(requests); });
}

bool SftpEDSI::removeResource()
{
  m_msgStack.clear();
  return mutateOne(*this, true, [&](FileEDSI& f) { return f.removeResource(); });
}

bool SftpEDSI::removeResources(const vector<EcceURL> urls)
{
  m_msgStack.clear();
  bool ret = true;
  for (size_t i = 0; i < urls.size(); i++) {
    SftpEDSI one(urls[i]);
    if (!one.removeResource()) { ret = false; m_msgStack.add(one.m_msgStack); }
  }
  return ret;
}

string SftpEDSI::uniqueName(const string& guess, const string& pattern)
{
  SftpOp op(*this);
  if (!op.ok() || !op.mirror(op.path(), 1)) return guess;
  InnerPtr in(op.inner(op.path()));
  return in->uniqueName(guess, pattern);
}

EcceURL *SftpEDSI::makeCollection(const string& base, const string& pattern)
{
  m_msgStack.clear();
  SftpOp op(*this);
  if (!op.ok() || !op.lock(vector<string>(1, op.path()), false) || !op.mirror(op.path(), 1)) return NULL;
  InnerPtr in(op.inner(op.path()));
  EcceURL *made = in->makeCollection(base, pattern);
  op.takeMessages(*in);
  if (!made) return NULL;
  string where = op.toRemotePath(made->getPath());
  delete made;
  if (!op.apply(true, vector<string>())) return NULL;
  return new EcceURL(op.remoteURL(where));
}

EcceURL *SftpEDSI::makeDataSet(const string& base)
{
  m_msgStack.clear();
  SftpOp op(*this);
  if (!op.ok() || !op.lock(vector<string>(1, op.path()), false) || !op.mirror(op.path(), 1)) return NULL;
  InnerPtr in(op.inner(op.path()));
  EcceURL *made = in->makeDataSet(base);
  op.takeMessages(*in);
  if (!made) return NULL;
  string where = op.toRemotePath(made->getPath());
  delete made;
  if (!op.apply(true, vector<string>())) return NULL;
  return new EcceURL(op.remoteURL(where));
}

// ----- move and copy -----

// The server moves or copies the data itself; FileEDSI on the mirror only
// works out the sidecar changes.
static bool moveOrCopy(SftpEDSI& e, EcceURL& target, EDSI::EDSIOverwrite ow, bool move)
{
  SftpOp op(e);
  if (!op.ok()) return false;
  string tu, th, tp;
  string ou, oh, op2;
  if (!SftpEDSI::parseURL(target.toString(false), tu, th, tp) || th != op.p_host || tu != op.p_user)
    return op.fail("Move and copy work within one server only");
  const string from = op.path();
  vector<string> dirs;
  dirs.push_back(parentOf(from));
  dirs.push_back(parentOf(trimSlash(tp)));
  if (!op.lock(dirs, false) || !op.mirror(from, 1) || !op.mirror(trimSlash(tp), 1)) return false;
  const string to = trimSlash(tp);
  bool toExisted = op.p_entries.count(to) > 0;
  InnerPtr in(op.inner(from));
  EcceURL local(op.local(to));
  bool ret = move ? in->moveResource(local, ow) : in->copyResource(local, ow);
  op.takeMessages(*in);
  if (!ret) return false;
  const string newTo = op.toRemotePath(local.getPath());
  string pre;
  if (ow == EDSI::YES && toExisted) pre += "rm -rf -- " + q(to) + "\n";
  if (move)
    pre += "[ ! -e " + q(newTo) + " ] || exit 1\nmv -- " + q(from) + " " + q(newTo) + "\n";
  else
    pre += "[ ! -e " + q(newTo) + " ] || exit 1\ncp -R -- " + q(from) + " " + q(newTo) + "\n"
           "find " + q(newTo) + " -name '.ecce-meta.lock*' -prune -exec rm -rf {} +\n";
  vector<string> skip;
  skip.push_back(from);
  skip.push_back(newTo);
  skip.push_back(to);
  if (!op.apply(false, skip, pre)) return false;
  if (newTo != to) target = op.remoteURL(newTo);
  return true;
}

bool SftpEDSI::moveResource(EcceURL& targetURL, EDSIOverwrite overwrite)
{
  m_msgStack.clear();
  return moveOrCopy(*this, targetURL, overwrite, true);
}

bool SftpEDSI::copyResource(EcceURL& targetURL, EDSIOverwrite overwrite)
{
  m_msgStack.clear();
  return moveOrCopy(*this, targetURL, overwrite, false);
}

// ----- search -----

bool SftpEDSI::efind(const string& key, const string& substring,
                     const EcceURL& start, vector<EcceURL>& matches)
{
  m_msgStack.clear();
  SftpOp op(*this);
  string u, h, p;
  if (!op.ok() || !SftpEDSI::parseURL(start.toString(false), u, h, p)) return false;
  p = trimSlash(p);
  if (!op.mirror(p, 1000)) return false;
  InnerPtr in(op.inner(p));
  vector<EcceURL> tmp;
  bool ret = in->efind(key, substring, EcceURL(op.local(p)), tmp);
  op.takeMessages(*in);
  for (size_t i = 0; i < tmp.size(); i++) matches.push_back(EcceURL(op.toRemoteURL(tmp[i].toString(false))));
  return ret;
}

bool SftpEDSI::efindProp(const string& substring, const EcceURL& start,
                         vector<EcceURL>& matches)
{
  m_msgStack.clear();
  SftpOp op(*this);
  string u, h, p;
  if (!op.ok() || !SftpEDSI::parseURL(start.toString(false), u, h, p)) return false;
  p = trimSlash(p);
  if (!op.mirror(p, 1000)) return false;
  InnerPtr in(op.inner(p));
  vector<EcceURL> tmp;
  bool ret = in->efindProp(substring, EcceURL(op.local(p)), tmp);
  op.takeMessages(*in);
  for (size_t i = 0; i < tmp.size(); i++) matches.push_back(EcceURL(op.toRemoteURL(tmp[i].toString(false))));
  return ret;
}
