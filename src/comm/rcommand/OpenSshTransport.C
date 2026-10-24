#include "comm/OpenSshTransport.H"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>
#include <set>
#include <atomic>
#include <mutex>
#include <thread>

#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "comm/DirectTransport.H"
#include "util/PipeCloexec.H"

namespace {

std::string shQuote(const std::string& s)
{
  std::string q = "'";
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '\'') q += "'\\''";
    else q += s[i];
  }
  return q + "'";
}

// A path for the target's sh; a leading ~ still expands.
std::string shPath(const std::string& p)
{
  if (p == "~") return "\"$HOME\"";
  if (p.compare(0, 2, "~/") == 0) return "\"$HOME\"/" + shQuote(p.substr(2));
  return shQuote(p);
}

bool validName(const std::string& n)
{
  if (n.empty() || isdigit((unsigned char)n[0])) return false;
  for (size_t i = 0; i < n.size(); i++)
    if (!isalnum((unsigned char)n[i]) && n[i] != '_') return false;
  return true;
}

std::vector<std::string> splitLines(const std::string& t)
{
  std::vector<std::string> v;
  size_t pos = 0;
  while (pos < t.size()) {
    size_t nl = t.find('\n', pos);
    if (nl == std::string::npos) nl = t.size();
    if (nl > pos) v.push_back(t.substr(pos, nl - pos));
    pos = nl + 1;
  }
  return v;
}

std::string baseName(std::string p)
{
  while (p.size() > 1 && p[p.size()-1] == '/') p.erase(p.size()-1);
  size_t slash = p.rfind('/');
  return slash == std::string::npos ? p : p.substr(slash + 1);
}

std::string joinPath(const std::string& dir, const std::string& name)
{
  if (dir.empty() || dir[dir.size()-1] == '/') return dir + name;
  return dir + "/" + name;
}

std::string lower(std::string s)
{
  for (size_t i = 0; i < s.size(); i++) s[i] = (char)tolower((unsigned char)s[i]);
  return s;
}

// The script command, as in SshTransport: stdin carries the script, which
// reads it from fd 3 so that nothing it runs can consume it.
const char* const kScriptExec = "sh -c 'exec 3<&0 </dev/null; . /dev/fd/3'";

// The script is the first line of stdin, escaped for printf %b; whatever
// follows on stdin is left to the script (file data, the monitor protocol).
// The login shell parses this line, so it sticks to quotes csh and sh share.
const char* const kDataExec =
  "sh -c 'IFS= read -r l; eval \"$(printf %b \"$l\")\"'";

std::string oneLine(const std::string& script)
{
  std::string e;
  for (size_t i = 0; i < script.size(); i++) {
    if (script[i] == '\\') e += "\\\\";
    else if (script[i] == '\n') e += "\\n";
    else e += script[i];
  }
  return e + "\n";
}

int keepaliveSec()
{
  const char* e = getenv("ECCE_SSH_KEEPALIVE");
  if (!e || !*e) return 30;
  int v = atoi(e);
  return v < 0 ? 0 : v;
}

struct OpenSshStream : RemoteStream {
  OpenSshStream() : pid(-1), appFd(-1) {}
  long pid;
  int appFd;
};

}  // namespace

OpenSshTransport::OpenSshTransport(const std::string& host, int port,
                                   const std::string& user)
  : p_host(host), p_user(user), p_program("ssh"), p_port(port), p_jumpPort(0),
    p_connectTimeout(15), p_askTimeout(120), p_connected(false),
    p_cancelled(false)
{
}

OpenSshTransport::~OpenSshTransport() {}

void OpenSshTransport::setJumpHost(const std::string& host, int port,
                                   const std::string& user)
{
  p_jumpHost = host;
  p_jumpPort = port;
  p_jumpUser = user;
}

// ---- backend selection ----

OpenSshTransport::Backend OpenSshTransport::requestedBackend()
{
  const char* e = getenv("ECCE_SSH_BACKEND");
  if (e && !strcmp(e, "libssh")) return BACKEND_LIBSSH;
  if (e && !strcmp(e, "openssh")) return BACKEND_OPENSSH;
  return BACKEND_AUTO;
}

bool OpenSshTransport::sharesConnection(const std::string& out)
{
  bool shares = false;
  for (const std::string& line : splitLines(out)) {
    size_t sp = line.find(' ');
    if (sp == std::string::npos) continue;
    std::string key = lower(line.substr(0, sp));
    std::string val = lower(line.substr(sp + 1));
    // ssh -G prints "controlmaster false" for the default and for "no".
    if (key == "controlmaster" && val != "false" && val != "no") shares = true;
    if (key == "controlpath" && val != "none") shares = true;
  }
  return shares;
}

bool OpenSshTransport::proxiesConnection(const std::string& out)
{
  for (const std::string& line : splitLines(out)) {
    size_t sp = line.find(' ');
    if (sp == std::string::npos) continue;
    std::string key = lower(line.substr(0, sp));
    std::string val = lower(line.substr(sp + 1));
    // "proxyusefdpass" is a different key and is always printed.
    if ((key == "proxyjump" || key == "proxycommand") && val != "none" &&
        !val.empty())
      return true;
  }
  return false;
}

namespace {
struct SshG { bool shares, proxied; };

SshG sshG(const std::string& host, const std::string& user,
          const std::string& configFile)
{
  static std::mutex mu;
  static std::map<std::string, SshG> cache;
  const std::string key = configFile + "\n" + user + "\n" + host;
  std::lock_guard<std::mutex> lock(mu);
  std::map<std::string, SshG>::iterator it = cache.find(key);
  if (it != cache.end()) return it->second;

  std::vector<std::string> args;
  args.push_back("ssh");
  args.push_back("-G");
  if (!configFile.empty()) { args.push_back("-F"); args.push_back(configFile); }
  if (!user.empty()) { args.push_back("-l"); args.push_back(user); }
  args.push_back(host);
  TransportResult r = DirectTransport::runProcess(args, "", -1, -1, 15);
  SshG g;
  g.shares = r.status == 0 && OpenSshTransport::sharesConnection(r.out);
  g.proxied = r.status == 0 && OpenSshTransport::proxiesConnection(r.out);
  cache[key] = g;
  return g;
}
}  // namespace

bool OpenSshTransport::configSharesConnection(const std::string& host,
                                              const std::string& user,
                                              const std::string& configFile)
{
  return sshG(host, user, configFile).shares;
}

bool OpenSshTransport::configProxiesConnection(const std::string& host,
                                               const std::string& user,
                                               const std::string& configFile)
{
  return sshG(host, user, configFile).proxied;
}

namespace {
bool nestedRequested(const std::string& jumpHost)
{
  const char* e = getenv("ECCE_SSH_FRONTEND");
  return !jumpHost.empty() && e && !strcmp(e, "nested");
}

bool sshOnPath()
{
  const char* p = getenv("PATH");
  std::string path = p ? p : "";
  size_t pos = 0;
  while (pos <= path.size()) {
    size_t c = path.find(':', pos);
    if (c == std::string::npos) c = path.size();
    std::string d = path.substr(pos, c - pos);
    if (access(((d.empty() ? "." : d) + "/ssh").c_str(), X_OK) == 0) return true;
    pos = c + 1;
  }
  return false;
}

// ssh binds <path>.<16 random chars> before renaming, and a unix socket
// path holds 104 bytes on macOS; room is kept for an overflow suffix "-NN".
const size_t kSocketLimit = 104, kSocketExtra = 17, kNameLen = 16 + 3;

void logOnce(const std::string& why)
{
  static std::mutex mu;
  static std::string last;
  std::lock_guard<std::mutex> lock(mu);
  if (why == last) return;
  last = why;
  fprintf(stderr, "ecce: ssh control directory not used: %s\n", why.c_str());
}

std::string hex16(unsigned long long h)
{
  char b[17];
  snprintf(b, sizeof b, "%016llx", h);
  return b;
}
}  // namespace

namespace {
std::mutex gNoFwdMu;
std::set<std::string> gNoFwd;
}  // namespace

bool OpenSshTransport::forwardingRefused(const std::string& host,
                                         const std::string& jumpHost)
{
  if (jumpHost.empty()) return false;
  std::lock_guard<std::mutex> lock(gNoFwdMu);
  return gNoFwd.count(host + "\n" + jumpHost) > 0;
}

// A front end that refuses TCP forwarding cannot serve ssh -J; the host then
// goes to libssh (nested) for the rest of the process.
void OpenSshTransport::noteForwardingRefused(const TransportResult& r)
{
  if (!ownsFromDir() || p_jumpHost.empty() || r.timedOut || r.status != 255)
    return;
  const std::string low = lower(r.err);
  const bool stdio = low.find("stdio forwarding failed") != std::string::npos;
  if (!stdio && (low.find("administratively prohibited") == std::string::npos ||
                 masterAlive()))
    return;
  std::lock_guard<std::mutex> lock(gNoFwdMu);
  if (gNoFwd.insert(p_host + "\n" + p_jumpHost).second)
    fprintf(stderr, "ecce: %s refuses forwarding for ssh -J to %s; "
            "using the built-in ssh client for it\n",
            p_jumpHost.c_str(), p_host.c_str());
}

std::string OpenSshTransport::ownedControlDir()
{
#ifdef _WIN32
  return "";
#else
  const char* e = getenv("ECCE_SSH_CONTROL_DIR");
  if (!e || !*e) return "";
  std::string dir = e;
  while (dir.size() > 1 && dir[dir.size()-1] == '/') dir.erase(dir.size()-1);
  struct stat sb;
  if (lstat(dir.c_str(), &sb) != 0 || !S_ISDIR(sb.st_mode) ||
      sb.st_uid != geteuid() || (sb.st_mode & 0777) != 0700) {
    logOnce(dir + " is not a directory of ours with mode 0700");
    return "";
  }
  if (dir.size() + 1 + kNameLen + kSocketExtra >= kSocketLimit) {
    logOnce(dir + " is too long for a control socket");
    return "";
  }
  return dir;
#endif
}

OpenSshTransport::Backend OpenSshTransport::backendFor(const std::string& host,
                                                       const std::string& user,
                                                       const std::string& configFile,
                                                       const std::string& jumpHost)
{
  Backend b = requestedBackend();
  if (b != BACKEND_AUTO) return b;
  // ECCE owns the master for every host; -J cannot serve a front end that
  // only allows nested ssh.
  if (!nestedRequested(jumpHost) && !ownedControlDir().empty() && sshOnPath() &&
      !forwardingRefused(host, jumpHost))
    return BACKEND_OPENSSH;
  // libssh cannot prompt on a proxy's own login, and a shared connection
  // is something only the ssh command can use.
  return configSharesConnection(host, user, configFile) ||
         configProxiesConnection(host, user, configFile) ? BACKEND_OPENSSH
                                                         : BACKEND_LIBSSH;
}

namespace {
const char* const kHostKeyText[] = {
  "host key verification failed", "remote host identification has changed", 0 };
const char* const kLoginText[] = {
  "permission denied", "no more authentication methods",
  "too many authentication failures", 0 };

bool mentions(const std::string& low, const char* const* list)
{
  for (int i = 0; list[i]; i++)
    if (low.find(list[i]) != std::string::npos) return true;
  return false;
}
}  // namespace

std::string OpenSshTransport::explainFailure(const std::string& host,
                                             const std::string& err,
                                             bool sharedConnection)
{
  const char* const* hostKey = kHostKeyText;
  const char* const* login = kLoginText;
  static const char* const connection[] = {
    "could not resolve hostname", "connection refused", "connection timed out",
    "no route to host", "connection closed by", "connection reset by",
    "network is unreachable", "kex_exchange_identification",
    "mux_client_request_session", "broken pipe", "ssh: connect to host",
    "timed out during banner exchange", "operation timed out",
    "control socket", "connection to ", 0 };

  const std::string low = lower(err);
  for (int i = 0; hostKey[i]; i++)
    if (low.find(hostKey[i]) != std::string::npos)
      return "The host key of " + host + " is not known or has changed. Run "
             "\"ssh " + host + "\" once in a terminal to check and accept it, "
             "then try again.";
  for (int i = 0; login[i]; i++)
    if (low.find(login[i]) != std::string::npos) {
      if (sharedConnection)
        return "No shared ssh connection to " + host + " is open. Run \"ssh " +
               host + "\" once in a terminal (that opens it), then try again.";
      return "ssh to " + host + " needs an interactive login; log in once "
             "with 'ssh " + host + "' (your connection sharing / two-factor), "
             "then try again.";
    }
  for (int i = 0; connection[i]; i++) {
    size_t at = low.find(connection[i]);
    if (at == std::string::npos) continue;
    size_t b = low.rfind('\n', at);
    b = b == std::string::npos ? 0 : b + 1;
    size_t e = err.find('\n', at);
    std::string line = err.substr(b, e == std::string::npos ? std::string::npos : e - b);
    while (!line.empty() && (line[line.size()-1] == '\r')) line.erase(line.size()-1);
    return "ssh connection to " + host + " failed: " + line;
  }
  return "";
}

// ---- commands ----

std::vector<std::string> OpenSshTransport::sshArgs(const std::string& remoteCmd) const
{
  std::vector<std::string> a;
  a.push_back(p_program);
  a.push_back("-T");
  a.push_back("-x");
  const std::string opts[] = {
    "BatchMode=yes", "LogLevel=ERROR", "ClearAllForwardings=yes",
    "ConnectTimeout=" + std::to_string(p_connectTimeout) };
  for (size_t i = 0; i < sizeof opts / sizeof opts[0]; i++) {
    a.push_back("-o");
    a.push_back(opts[i]);
  }
  // A link that goes silent ends the command instead of hanging it.
  if (int ka = keepaliveSec()) {
    a.push_back("-o"); a.push_back("ServerAliveInterval=" + std::to_string(ka));
    a.push_back("-o"); a.push_back("ServerAliveCountMax=3");
  }
  std::vector<std::string> t = targetArgs();
  a.insert(a.end(), t.begin(), t.end());
  a.push_back(p_host);
  a.push_back(remoteCmd);
  return a;
}

std::vector<std::string> OpenSshTransport::targetArgs() const
{
  std::vector<std::string> a;
  if (!p_configFile.empty()) { a.push_back("-F"); a.push_back(p_configFile); }
  if (p_port > 0) { a.push_back("-p"); a.push_back(std::to_string(p_port)); }
  if (!p_user.empty()) { a.push_back("-l"); a.push_back(p_user); }
  if (!p_jumpHost.empty()) {
    std::string j = p_jumpUser.empty() ? p_jumpHost : p_jumpUser + "@" + p_jumpHost;
    if (p_jumpPort > 0) j += ":" + std::to_string(p_jumpPort);
    a.push_back("-J");
    a.push_back(j);
  }
  std::vector<std::string> c = controlArgs();
  a.insert(a.end(), c.begin(), c.end());
  return a;
}

std::vector<std::string> OpenSshTransport::controlArgs() const
{
  std::vector<std::string> a;
  if (ownsFromDir()) {
    const char* pe = getenv("ECCE_SSH_CONTROL_PERSIST");
    std::string persist = pe && *pe ? pe : "8h";
    for (size_t i = 0; i < persist.size(); i++)
      if (!isalnum((unsigned char)persist[i])) persist = "8h";
    a.push_back("-o"); a.push_back("ControlMaster=auto");
    a.push_back("-o"); a.push_back("ControlPath=" + socketFor(overflowIndex()));
    a.push_back("-o"); a.push_back("ControlPersist=" + persist);
  } else if (legacyOwnsControl()) {
    // Short, so that %C (a hash) keeps the socket under the 108-byte limit.
    const char* h = getenv("ECCE_REALUSERHOME");
    if (!h || !*h) h = getenv("HOME");
    const std::string dir = std::string(h ? h : "/tmp") + "/.ECCE/cm";
    mkdir((std::string(h ? h : "/tmp") + "/.ECCE").c_str(), 0700);
    mkdir(dir.c_str(), 0700);
    a.push_back("-o"); a.push_back("ControlMaster=auto");
    a.push_back("-o"); a.push_back("ControlPath=" + dir + "/%C");
    a.push_back("-o"); a.push_back("ControlPersist=10m");
  }
  return a;
}

// A proxied host with no sharing of its own gets ECCE's, so that the proxy
// is logged in to once and not for every command.
bool OpenSshTransport::ownsControl() const
{
  return ownsFromDir() || legacyOwnsControl();
}

bool OpenSshTransport::legacyOwnsControl() const
{
  return p_jumpHost.empty() &&
         configProxiesConnection(p_host, p_user, p_configFile) &&
         !configSharesConnection(p_host, p_user, p_configFile);
}

// The user's own sharing wins over ours.
bool OpenSshTransport::ownsFromDir() const
{
  return !nestedRequested(p_jumpHost) && !ownedControlDir().empty() &&
         !configSharesConnection(p_host, p_user, p_configFile);
}

namespace {
std::mutex gIdxMu;
std::map<std::string, int> gOverflow;   // socket hash -> master in use
const int kMaxOverflow = 8;
}  // namespace

std::string OpenSshTransport::socketFor(int index) const
{
  char lh[256] = "";
  gethostname(lh, sizeof lh - 1);
  // Everything that makes two connections different, so that one socket
  // never serves another host, user or route.
  std::string id = std::string(lh) + "\n" + p_user + "\n" + p_host + "\n" +
                   std::to_string(p_port) + "\n" + p_jumpHost + "\n" +
                   std::to_string(p_jumpPort) + "\n" + p_jumpUser + "\n" +
                   p_configFile;
  unsigned long long h = 1469598103934665603ULL;   // FNV-1a
  for (size_t i = 0; i < id.size(); i++) {
    h ^= (unsigned char)id[i];
    h *= 1099511628211ULL;
  }
  std::string name = hex16(h);
  if (index > 0) name += "-" + std::to_string(index);
  return ownedControlDir() + "/" + name;
}

int OpenSshTransport::overflowIndex() const
{
  std::lock_guard<std::mutex> lock(gIdxMu);
  std::map<std::string, int>::const_iterator i = gOverflow.find(socketFor(0));
  return i == gOverflow.end() ? 0 : i->second;
}

std::string OpenSshTransport::controlSocketPath() const
{
  return ownsFromDir() ? socketFor(overflowIndex()) : "";
}

// MaxSessions: one master carries about ten sessions, and every monitor
// holds one for the whole job.
bool OpenSshTransport::nextOverflowMaster(const TransportResult& r)
{
  if (!ownsFromDir() || r.timedOut || r.status != 255) return false;
  const std::string low = lower(r.err);
  if (low.find("administratively prohibited") == std::string::npos &&
      low.find("session open refused") == std::string::npos &&
      low.find("session request failed") == std::string::npos)
    return false;
  const std::string base = socketFor(0);
  {
    std::lock_guard<std::mutex> lock(gIdxMu);
    int& cur = gOverflow[base];
    if (cur >= kMaxOverflow) return false;
    cur++;
  }
  // A master another process opened is joined; otherwise open one.
  return masterAlive() || (!p_askpass.empty() && openMaster());
}

bool OpenSshTransport::sharedConnection() const
{
  return ownsControl() || configSharesConnection(p_host, p_user, p_configFile);
}

// ---- opening the shared connection ----

namespace {
std::mutex gMasterMu;
// Hosts whose attempt was refused or cancelled, and when: a monitor that
// reconnects every few seconds must not put up a dialog every time.
std::map<std::string, time_t> gMasterFailed;
std::set<std::string> gMasterCancelled;   // of those, the ones the user cancelled
const int kRetryAfterSec = 120;
}  // namespace

void OpenSshTransport::forgetMasterAttempts()
{
  std::lock_guard<std::mutex> lock(gMasterMu);
  gMasterFailed.clear();
  gMasterCancelled.clear();
}

bool OpenSshTransport::masterAlive() const
{
  std::vector<std::string> a;
  a.push_back(p_program);
  a.push_back("-O");
  a.push_back("check");
  std::vector<std::string> t = targetArgs();
  a.insert(a.end(), t.begin(), t.end());
  a.push_back(p_host);
  return DirectTransport::runProcess(a, "", -1, -1, 15).status == 0;
}

bool OpenSshTransport::wantsMaster(const TransportResult& r) const
{
  if (p_askpass.empty() || r.status != 255 || r.timedOut) return false;
  const std::string low = lower(r.err);
  if (mentions(low, kLoginText) || mentions(low, kHostKeyText))
    return sharedConnection();
  // A refused login on a proxy's hop says nothing at LogLevel=ERROR; 255
  // with no master up is then all there is.  With one up, 255 is the
  // script's own status and must not run it again.
  return ownsControl() && !masterAlive();
}

bool OpenSshTransport::openMaster()
{
  std::lock_guard<std::mutex> lock(gMasterMu);
  // Another process has the same wish after a dropped link: one holds this
  // lock and asks, the rest find the master and do not.  CLOEXEC, so that
  // the master it leaves behind does not keep the lock.
  struct Lock {
    int fd;
    Lock() : fd(-1) {}
    ~Lock() { if (fd >= 0) close(fd); }
  } flk;
  const bool owned = ownsFromDir();
  if (owned) {
    flk.fd = open((socketFor(overflowIndex()) + ".lock").c_str(),
                  O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    if (flk.fd >= 0) {
      while (flock(flk.fd, LOCK_EX) != 0 && errno == EINTR) {}
    }
  }
  // Another session may have opened it while this one waited its turn.
  if (masterAlive()) return true;
  const std::string key = p_configFile + "\n" + p_user + "\n" + p_host + "\n" +
                          p_jumpHost + "\n" + std::to_string(overflowIndex());
  std::map<std::string, time_t>::iterator f = gMasterFailed.find(key);
  if (f != gMasterFailed.end() && time(0) - f->second < kRetryAfterSec) {
    p_cancelled = gMasterCancelled.count(key) > 0;
    return false;
  }

  // ssh asks again after a refusal; askpass records a cancel in this file
  // (its directory is ours alone) so that one cancel is one dialog.
  const char* tmpdir = getenv("TMPDIR");
  std::string stateDir = std::string(tmpdir && *tmpdir ? tmpdir : "/tmp") +
                         "/ecce-askpass.XXXXXX";
  std::vector<char> tmpl(stateDir.begin(), stateDir.end());
  tmpl.push_back('\0');
  const bool haveState = mkdtemp(&tmpl[0]) != 0;
  if (haveState) stateDir = &tmpl[0];

  // A known password reaches askpass through a FIFO in the state directory
  // (0700): not argv, environment or a file, and ssh closes inherited fds.
  std::string fifo;
  std::thread writer;
  std::atomic<bool> giveUp(false);
  if (owned && haveState && !p_password.empty()) {
    fifo = stateDir + "/password";
    if (mkfifo(fifo.c_str(), 0600) == 0) {
      const std::string pw = p_password + "\n";
      writer = std::thread([fifo, pw, &giveUp]() {
        int fd = open(fifo.c_str(), O_WRONLY | O_CLOEXEC);  // waits for askpass
        if (fd < 0) return;
        if (!giveUp) {
          size_t done = 0;
          while (done < pw.size()) {
            ssize_t w = write(fd, pw.data() + done, pw.size() - done);
            if (w > 0) done += w;
            else if (!(w < 0 && errno == EINTR)) break;
          }
        }
        close(fd);
      });
    } else fifo.clear();
  }

  // The user's own ssh configuration decides ControlMaster and
  // ControlPersist unless ECCE owns the master; -f leaves the master behind
  // once the login is done.
  std::vector<std::string> a;
  a.push_back("env");
  a.push_back("SSH_ASKPASS=" + p_askpass);
  if (haveState) a.push_back("ECCE_ASKPASS_STATE=" + stateDir + "/cancelled");
  if (!fifo.empty()) a.push_back("ECCE_ASKPASS_PWFIFO=" + fifo);
  a.push_back("SSH_ASKPASS_REQUIRE=force");
  a.push_back("ECCE_ASKPASS_HOST=" + p_host);
  a.push_back("ECCE_ASKPASS_USER=" + p_user);
  a.push_back(p_program);
  a.push_back("-f");
  a.push_back("-N");
  a.push_back("-x");
  a.push_back("-o");
  a.push_back("BatchMode=no");
  a.push_back("-o");
  a.push_back("ConnectTimeout=" + std::to_string(p_connectTimeout));
  if (owned) {
    // A silent link ends the master instead of leaving every command on it hanging.
    a.push_back("-o"); a.push_back("ServerAliveInterval=" + std::to_string(keepaliveSec() ? keepaliveSec() : 30));
    a.push_back("-o"); a.push_back("ServerAliveCountMax=3");
  }
  std::vector<std::string> t = targetArgs();
  a.insert(a.end(), t.begin(), t.end());
  a.push_back(p_host);
  TransportResult r = DirectTransport::runProcess(a, "", -1, -1, p_askTimeout);
  if (writer.joinable()) {
    // ssh never asked for a password: release the writer, which waits for a reader.
    giveUp = true;
    int rd = open(fifo.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    writer.join();
    if (rd >= 0) close(rd);
  }
  bool ok = !r.timedOut && r.status == 0 && masterAlive();
  p_cancelled = !ok && haveState &&
                access((stateDir + "/cancelled").c_str(), F_OK) == 0;
  if (!ok && !p_cancelled) {
    std::string why = r.timedOut ? "no answer in time" : r.err;
    while (!why.empty() && (why[why.size()-1] == '\n' || why[why.size()-1] == '\r'))
      why.erase(why.size() - 1);
    fprintf(stderr, "ecce: could not open the ssh connection to %s: %s\n",
            p_host.c_str(), why.c_str());
  }
  if (haveState) {
    unlink((stateDir + "/password").c_str());
    unlink((stateDir + "/cancelled").c_str());
    rmdir(stateDir.c_str());
  }
  if (ok) gMasterFailed.erase(key);
  else gMasterFailed[key] = time(0);
  if (p_cancelled) gMasterCancelled.insert(key);
  else gMasterCancelled.erase(key);
  return ok;
}

std::string OpenSshTransport::envPrefix(std::string& error) const
{
  std::string s;
  for (std::map<std::string, bool>::const_iterator i = p_unset.begin();
       i != p_unset.end(); ++i) {
    if (!validName(i->first)) { error = "bad variable name " + i->first; return ""; }
    s += "unset " + i->first + "\n";
  }
  for (std::map<std::string, std::string>::const_iterator i = p_env.begin();
       i != p_env.end(); ++i) {
    if (!validName(i->first)) { error = "bad variable name " + i->first; return ""; }
    s += "export " + i->first + "=" + shQuote(i->second) + "\n";
  }
  return s;
}

// 255 is ssh's own failure status; the script's stderr is mixed into err, so
// only text that reads like an ssh message turns it into a connection error.
void OpenSshTransport::explain(TransportResult& r) const
{
  if (r.timedOut) return;
  if (r.status == 255) {
    // "Run ssh in a terminal" helps only when the user's own config shares.
    std::string why = explainFailure(p_host, r.err,
      configSharesConnection(p_host, p_user, p_configFile));
    if (ownsFromDir() && mentions(lower(r.err), kLoginText))
      why = p_cancelled ? "The login to " + p_host + " was cancelled."
                        : "The login to " + p_host + " was refused; check the "
                          "user name and password, then try again.";
    if (!why.empty()) { r.error = why; r.status = -1; }
  } else if (r.status < 0 && !r.error.empty() &&
             r.error.find("command not found") != std::string::npos) {
    r.error = "cannot run " + p_program + ": " + r.error;
  }
}

TransportResult OpenSshTransport::runImpl(const std::string& script, int timeoutSec,
                                          bool useDir)
{
  TransportResult res;
  std::string err;
  std::string envp = envPrefix(err);
  if (!err.empty()) { res.error = err; return res; }
  // The script is read from fd 3 and stdin is /dev/null, as in DirectTransport.
  std::string full = "exec 3<&-\n" + envp + (useDir ? withDir(script) : script);
  res = DirectTransport::runProcess(sshArgs(kScriptExec), full, -1, -1, timeoutSec);
  if (wantsMaster(res) && openMaster())
    res = DirectTransport::runProcess(sshArgs(kScriptExec), full, -1, -1,
                                      timeoutSec);
  noteForwardingRefused(res);
  for (int n = 0; n < kMaxOverflow && nextOverflowMaster(res); n++)
    res = DirectTransport::runProcess(sshArgs(kScriptExec), full, -1, -1,
                                      timeoutSec);
  explain(res);
  return res;
}

TransportResult OpenSshTransport::run(const std::string& script, int timeoutSec)
{
  return runImpl(script, timeoutSec, true);
}

TransportResult OpenSshTransport::runWithData(const std::string& script, int inFd,
                                              int outFd, int timeoutSec)
{
  TransportResult res = DirectTransport::runProcess(
    sshArgs(kDataExec), oneLine(script), inFd, outFd, timeoutSec);
  // The failed attempt may have taken some of the data; a descriptor that
  // cannot be rewound is not retried.
  if (wantsMaster(res) &&
      (inFd < 0 || lseek(inFd, 0, SEEK_SET) == 0) &&
      (outFd < 0 || (lseek(outFd, 0, SEEK_SET) == 0 && ftruncate(outFd, 0) == 0)) &&
      openMaster())
    res = DirectTransport::runProcess(sshArgs(kDataExec), oneLine(script), inFd,
                                      outFd, timeoutSec);
  noteForwardingRefused(res);
  for (int n = 0; n < kMaxOverflow && nextOverflowMaster(res) &&
       (inFd < 0 || lseek(inFd, 0, SEEK_SET) == 0) &&
       (outFd < 0 || (lseek(outFd, 0, SEEK_SET) == 0 && ftruncate(outFd, 0) == 0));
       n++)
    res = DirectTransport::runProcess(sshArgs(kDataExec), oneLine(script), inFd,
                                      outFd, timeoutSec);
  explain(res);
  return res;
}

bool OpenSshTransport::connect(std::string& error)
{
  p_connected = false;
  TransportResult r = runImpl("exit 0\n", 60, false);
  if (r.status != 0) {
    std::string why = r.error;
    if (why.empty()) {
      why = r.err;
      while (!why.empty() && (why[why.size()-1] == '\n' || why[why.size()-1] == '\r'))
        why.erase(why.size() - 1);
    }
    error = why.empty() ? "cannot connect to " + p_host : why;
    return false;
  }
  p_connected = true;
  return true;
}

long OpenSshTransport::spawnDetached(const std::string& script, std::string& error,
                                     const std::string& logFile)
{
  std::string target = logFile.empty() ? "/dev/null" : shQuote(logFile);
  TransportResult r = run("setsid nohup sh -c " + shQuote(script) + " >>" +
                          target + " 2>&1 </dev/null &\necho $!\n", 30);
  if (r.status != 0 || !r.error.empty()) {
    error = r.error.empty() ? "spawn failed: " + r.err : r.error;
    return -1;
  }
  long pid = strtol(r.out.c_str(), 0, 10);
  if (pid <= 0) {
    error = "no process id returned: " + r.out;
    return -1;
  }
  return pid;
}

// ---- files: streamed through cat, so the mode and every byte survive ----

bool OpenSshTransport::put(const std::string& localPath, const std::string& remotePath,
                           std::string& error)
{
  int in = open(localPath.c_str(), O_RDONLY | O_CLOEXEC);
  if (in < 0) { error = "cannot read " + localPath + ": " + strerror(errno); return false; }
  struct stat sb;
  fstat(in, &sb);
  char mode[16];
  snprintf(mode, sizeof mode, "%04o", (unsigned)(sb.st_mode & 07777));
  std::string p = shPath(remotePath);
  // The server's umask trimmed nothing here: chmod restores what was asked.
  TransportResult r = runWithData("cat > " + p + " && chmod " + mode + " " + p +
                                  "\n", in, -1, 120);
  close(in);
  if (r.status != 0 || !r.error.empty()) {
    std::string why = r.error.empty() ? r.err : r.error;
    error = "cannot write " + remotePath + ": " + why;
    return false;
  }
  return true;
}

bool OpenSshTransport::get(const std::string& remotePath, const std::string& localPath,
                           std::string& error)
{
  std::string p = shPath(remotePath);
  TransportResult m = runImpl("p=" + p + "\n[ -f \"$p\" ] && [ -r \"$p\" ] || "
    "{ echo \"cannot open $p\" >&2; exit 2; }\n"
    "stat -c %a \"$p\" 2>/dev/null || stat -f %Lp \"$p\" || echo 644\n", 60, false);
  if (m.status != 0 || !m.error.empty()) {
    error = "cannot read " + remotePath + ": " + (m.error.empty() ? m.err : m.error);
    return false;
  }
  mode_t mode = (mode_t)strtol(m.out.c_str(), 0, 8);
  int out = open(localPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
  if (out < 0) { error = "cannot write " + localPath + ": " + strerror(errno); return false; }
  TransportResult r = runWithData("cat < " + p + "\n", -1, out, 600);
  bool ok = r.status == 0 && r.error.empty();
  if (!ok)
    error = "cannot read " + remotePath + ": " + (r.error.empty() ? r.err : r.error);
  else
    fchmod(out, mode & 07777);
  close(out);
  return ok;
}

int OpenSshTransport::remoteKind(const std::string& remotePath)
{
  TransportResult r = runImpl("p=" + shPath(remotePath) + "\nif [ -d \"$p\" ]; "
    "then echo 1; elif [ -e \"$p\" ]; then echo 0; else echo -1; fi\n", 30, false);
  return r.status == 0 ? atoi(r.out.c_str()) : -1;
}

bool OpenSshTransport::remoteGlob(const std::string& pattern,
                                  std::vector<std::string>& out, std::string& error)
{
  out.clear();
  std::string pat = pattern;
  if (pat == "~") pat = ".";
  else if (pat.compare(0, 2, "~/") == 0) pat = pat.substr(2).empty() ? "." : pat.substr(2);
  if (pat.find_first_of("*?[") == std::string::npos) {
    out.push_back(pattern);
    return true;
  }
  std::string word;
  for (size_t i = 0; i < pat.size(); i++) {
    unsigned char c = pat[i];
    if (isalnum(c) || strchr("/._-+,:=@%*?[]", c)) word += (char)c;
    else { word += '\\'; word += (char)c; }
  }
  TransportResult r = run("for f in " + word + "; do if [ -e \"$f\" ] || "
                          "[ -L \"$f\" ]; then printf '%s\\n' \"$f\"; fi; done", 30);
  if (r.status != 0 || !r.error.empty()) {
    error = r.error.empty() ? "cannot expand " + pattern : r.error;
    return false;
  }
  out = splitLines(r.out);
  if (out.empty()) {
    error = pattern + ": No such file or directory";
    return false;
  }
  return true;
}

// Copies local (a file or a tree) to remote, which is the final name.
bool OpenSshTransport::putInto(const std::string& local, const std::string& remote,
                               std::string& error)
{
  struct stat sb;
  if (stat(local.c_str(), &sb) != 0) {
    error = "cannot read " + local + ": " + strerror(errno);
    return false;
  }
  if (!S_ISDIR(sb.st_mode)) return put(local, remote, error);
  std::string p = shPath(remote);
  TransportResult r = runImpl("[ -d " + p + " ] || mkdir " + p + "\n", 30, false);
  if (r.status != 0 || !r.error.empty()) {
    error = "cannot create directory " + remote + ": " +
            (r.error.empty() ? r.err : r.error);
    return false;
  }
  DIR* d = opendir(local.c_str());
  if (!d) { error = "cannot read " + local + ": " + strerror(errno); return false; }
  std::vector<std::string> names;
  while (struct dirent* e = readdir(d)) {
    std::string n = e->d_name;
    if (n != "." && n != "..") names.push_back(n);
  }
  closedir(d);
  std::sort(names.begin(), names.end());
  for (size_t i = 0; i < names.size(); i++)
    if (!putInto(joinPath(local, names[i]), joinPath(remote, names[i]), error))
      return false;
  return true;
}

bool OpenSshTransport::putTree(const std::string& localPath,
                               const std::string& remotePath, std::string& error)
{
  std::string target = remotePath;
  if (remoteKind(remotePath) == 1) target = joinPath(target, baseName(localPath));
  return putInto(localPath, target, error);
}

bool OpenSshTransport::getTree(const std::string& remotePath,
                               const std::string& localPath, std::string& error)
{
  struct stat nb;
  std::string tgt = localPath;
  if (stat(localPath.c_str(), &nb) == 0 && S_ISDIR(nb.st_mode))
    tgt = joinPath(localPath, baseName(remotePath));
  int kind = remoteKind(remotePath);
  if (kind < 0) { error = "cannot stat " + remotePath; return false; }
  if (kind == 0) return get(remotePath, tgt, error);
  std::string top = shPath(remotePath);
  TransportResult d = runImpl("cd " + top + " && find . -type d\n", 60, false);
  TransportResult f = runImpl("cd " + top + " && find . ! -type d\n", 60, false);
  if (d.status != 0 || f.status != 0) {
    error = "cannot list " + remotePath + ": " + d.err + f.err +
            (d.error.empty() ? f.error : d.error);
    return false;
  }
  std::vector<std::string> dirs = splitLines(d.out), files = splitLines(f.out);
  for (size_t i = 0; i < dirs.size(); i++) {
    std::string dst = dirs[i] == "." ? tgt : joinPath(tgt, dirs[i].substr(2));
    if (mkdir(dst.c_str(), 0755) != 0 && errno != EEXIST) {
      error = "cannot create " + dst + ": " + strerror(errno);
      return false;
    }
  }
  for (size_t i = 0; i < files.size(); i++)
    if (files[i].size() > 2 && !get(joinPath(remotePath, files[i].substr(2)),
                                    joinPath(tgt, files[i].substr(2)), error))
      return false;
  return true;
}

// ---- the monitor stream ----

RemoteStream* OpenSshTransport::openStream(const std::string& script, int& fd,
                                           std::string& error)
{
  std::string err;
  std::string envp = envPrefix(err);
  if (!err.empty()) { error = err; return 0; }
  // The remote stderr joins stdout; ssh's own messages are dropped, so the
  // reader sees the script's output or EOF and nothing else.
  std::string header = oneLine("exec 2>&1\n" + envp + withDir(script));

  // With an owned master, ssh's stderr is kept (unlinked file) for as long
  // as it takes to see whether the master refused the session.
  const bool owned = ownsFromDir();
  for (int attempt = 0;; attempt++) {
  int sp[2];
  if (socketpairCloexec(AF_UNIX, SOCK_STREAM, 0, sp) != 0) {
    error = strerror(errno);
    return 0;
  }
  int devnull = -1;
  if (owned) {
    const char* td = getenv("TMPDIR");
    std::string t = std::string(td && *td ? td : "/tmp") + "/ecce-ssherr.XXXXXX";
    std::vector<char> tm(t.begin(), t.end());
    tm.push_back('\0');
    devnull = mkstemp(&tm[0]);
    if (devnull >= 0) {
      unlink(&tm[0]);
      fcntl(devnull, F_SETFD, FD_CLOEXEC);
    }
  }
  if (devnull < 0) devnull = open("/dev/null", O_WRONLY | O_CLOEXEC);
  if (devnull < 0) {
    error = strerror(errno);
    close(sp[0]); close(sp[1]);
    return 0;
  }
  long pid = DirectTransport::spawnProcess(sshArgs(kDataExec), sp[1], sp[1],
                                           devnull, error);
  close(sp[1]);
  if (pid < 0) { close(devnull); close(sp[0]); return 0; }

  // The script goes first.  ssh reads stdin as soon as it is connected, so
  // this returns then; if it cannot connect it exits and the send fails.
  size_t done = 0;
  while (done < header.size()) {
    ssize_t w = send(sp[0], header.data() + done, header.size() - done, MSG_NOSIGNAL);
    if (w > 0) done += w;
    else if (w < 0 && errno == EINTR) continue;
    else break;
  }
  bool refused = false;
  if (owned && attempt < kMaxOverflow) {
    // The master says so at once, but ssh then tries a connection of its own
    // (which may succeed or only fail later): the message is what counts.
    struct pollfd pf = { sp[0], POLLIN, 0 };
    poll(&pf, 1, 250);
    char eb[2048];
    ssize_t n = pread(devnull, eb, sizeof eb - 1, 0);
    if (n > 0) {
      TransportResult fake;
      fake.status = 255;
      fake.err.assign(eb, n);
      refused = nextOverflowMaster(fake);
    }
  }
  close(devnull);
  if (refused || done < header.size()) {
    DirectTransport::Stream st;
    st.pid = pid;
    st.rfd = sp[0];
    DirectTransport d;
    d.closeStream(st, 0);
    if (refused) continue;
    error = "cannot start command on " + p_host;
    return 0;
  }
  OpenSshStream* s = new OpenSshStream;
  s->pid = pid;
  s->appFd = sp[0];
  fd = sp[0];
  return s;
  }
}

// ssh has no way to signal a remote command without a tty; ending the client
// closes the session, and the monitor then sees EOF on its stdin.
void OpenSshTransport::interruptStream(RemoteStream* rs)
{
  OpenSshStream* s = static_cast<OpenSshStream*>(rs);
  if (s && s->pid > 0) kill(-(pid_t)s->pid, SIGINT);
}

void OpenSshTransport::closeStream(RemoteStream* rs, int graceMs)
{
  OpenSshStream* s = static_cast<OpenSshStream*>(rs);
  if (!s) return;
  DirectTransport::Stream st;
  st.pid = s->pid;
  st.rfd = s->appFd;   // closing it is the EOF the monitor waits for
  DirectTransport d;
  d.closeStream(st, graceMs);
  delete s;
}
