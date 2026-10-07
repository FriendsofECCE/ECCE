#include "comm/SshTransport.H"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

#include <dirent.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#ifdef __linux__
#include <linux/tcp.h>   // glibc's struct tcp_info stops short of the byte counters
#else
#include <netinet/tcp.h>
#endif
#include <unistd.h>

#include <libssh/libssh.h>
#include <libssh/server.h>   // ssh_send_keepalive
#include <libssh/sftp.h>
#include "util/PipeCloexec.H"

namespace {

typedef std::chrono::steady_clock Clock;

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

const char kB64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string b64Encode(const std::string& in)
{
  std::string o;
  for (size_t i = 0; i < in.size(); i += 3) {
    unsigned v = (unsigned char)in[i] << 16;
    if (i + 1 < in.size()) v |= (unsigned char)in[i+1] << 8;
    if (i + 2 < in.size()) v |= (unsigned char)in[i+2];
    o += kB64[(v >> 18) & 63];
    o += kB64[(v >> 12) & 63];
    o += i + 1 < in.size() ? kB64[(v >> 6) & 63] : '=';
    o += i + 2 < in.size() ? kB64[v & 63] : '=';
    if ((i / 3) % 19 == 18) o += '\n';
  }
  return o + "\n";
}

std::string b64Decode(const std::string& in)
{
  std::string o;
  unsigned v = 0;
  int bits = 0;
  for (size_t i = 0; i < in.size(); i++) {
    const char* p = in[i] == '=' ? 0 : strchr(kB64, in[i]);
    if (!p || !in[i]) continue;
    v = (v << 6) | (unsigned)(p - kB64);
    bits += 6;
    if (bits >= 8) { bits -= 8; o += (char)((v >> bits) & 255); }
  }
  return o;
}

bool validName(const std::string& n)
{
  if (n.empty() || isdigit((unsigned char)n[0])) return false;
  for (size_t i = 0; i < n.size(); i++)
    if (!isalnum((unsigned char)n[i]) && n[i] != '_') return false;
  return true;
}

void wipe(std::string& s)
{
  volatile char* p = s.empty() ? 0 : &s[0];
  for (size_t i = 0; i < s.size(); i++) p[i] = 0;
  s.clear();
}

// Seconds of silence before a keepalive (ECCE_SSH_KEEPALIVE; 0 = off).  A
// link is called dead after three intervals without any traffic.
int keepaliveSec()
{
  const char* e = getenv("ECCE_SSH_KEEPALIVE");
  if (!e || !*e) return 30;
  int v = atoi(e);
  return v < 0 ? 0 : v;
}

// Kernel TCP keepalive: ends a blocked read on a link whose host is gone.  It
// cannot tell a frozen sshd from a live one, which the ssh keepalive can.
void setTcpKeepalive(ssh_session s, int sec)
{
  if (sec <= 0) return;
  int fd = ssh_get_fd(s);
  if (fd < 0) return;
  int on = 1, idle = sec, intvl = std::max(1, sec / 3), cnt = 3;
  setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof on);
#ifdef TCP_KEEPIDLE
  setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof idle);
  setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof intvl);
  setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof cnt);
#endif
}

// Bytes the kernel has received on the connection so far.  libssh does not
// report the reply to a keepalive, but any packet it brings shows up here.
// -1 where the kernel cannot say.
template <class T>
auto rxBytes(const T& ti, int) -> decltype((long long)ti.tcpi_bytes_received)
{ return (long long)ti.tcpi_bytes_received; }
template <class T>
long long rxBytes(const T&, long) { return -1; }

long long receivedBytes(int fd)
{
#ifdef TCP_INFO
  struct tcp_info ti;
  socklen_t len = sizeof ti;
  memset(&ti, 0, sizeof ti);
  if (fd >= 0 && getsockopt(fd, IPPROTO_TCP, TCP_INFO, &ti, &len) == 0)
    return rxBytes(ti, 0);
#endif
  return -1;
}

// Sleeps until the channel has data or eof, or the timeout passes.
void waitChannel(ssh_channel ch, struct timeval& tv)
{
  ssh_channel chans[2] = { ch, 0 };
  ssh_channel outc[2] = { 0, 0 };
  fd_set fds;
  FD_ZERO(&fds);
  ssh_select(chans, outc, 0, &fds, &tv);
}

}  // namespace

static void closeJump(SshJump* j);

SshTransport::SshTransport(const std::string& host, int port,
                           const std::string& user)
  : p_host(host), p_user(user), p_port(port), p_useConfig(true),
    p_connectTimeout(15), p_passwordAttempts(1), p_jumpPort(0),
    p_feMode(FE_AUTO), p_nested(false), p_forwardWorked(false), p_authFailed(false), p_link(0),
    p_session(0), p_lent(false), p_sftp(0)
{
  const char* e = getenv("ECCE_SSH_FRONTEND");
  if (e && !strcmp(e, "forward")) p_feMode = FE_FORWARD;
  else if (e && !strcmp(e, "nested")) p_feMode = FE_NESTED;
}

void SshTransport::setJumpHost(const std::string& host, int port,
                               const std::string& user)
{
  p_jumpHost = host;
  p_jumpPort = port;
  p_jumpUser = user;
}

SshTransport::~SshTransport() { disconnect(); }

void SshTransport::disconnect()
{
  if (p_sftp) { sftp_free(p_sftp); p_sftp = 0; }
  if (p_session) {
    ssh_disconnect(p_session);
    ssh_free(p_session);
    p_session = 0;
  }
  if (p_link) { closeJump(p_link); p_link = 0; }
}

bool SshTransport::connect(std::string& error)
{
  disconnect();
  p_session = newSession(error, &p_link);
  if (!p_session) return false;
  if (p_nested) {
    // ssh from the front end can fail for reasons the front end alone
    // knows (unknown host key, no key); find out now, with its message.
    TransportResult r = runImpl("exit 0\n", 60, false);
    if (r.status != 0) {
      std::string why = r.error.empty() ? r.err : r.error;
      while (!why.empty() && (why[why.size()-1] == '\n' || why[why.size()-1] == '\r'))
        why.erase(why.size() - 1);
      error = "ssh from " + p_jumpHost + " to " + p_host + " failed: " + why +
              " (the front end needs a key for " + p_host + " and " + p_host +
              " in its known_hosts; try \"ssh " + p_host + "\" there once)";
      disconnect();
      return false;
    }
  }
  return true;
}

std::string SshTransport::execLine(const std::string& remoteCmd) const
{
  if (!p_nested) return remoteCmd;
  // Both login shells parse this line, so it sticks to quotes they agree on.
  std::string c = "ssh -T -o BatchMode=yes -o LogLevel=ERROR -o ConnectTimeout=" +
                  std::to_string(p_connectTimeout);
  if (p_port > 0) c += " -p " + std::to_string(p_port);
  if (!p_user.empty()) c += " -l " + shQuote(p_user);
  return c + " " + shQuote(p_host) + " " + shQuote(remoteCmd);
}

// ---- the forwarded connection ----

// One authenticated session per front end is shared by every inner session
// of the process, so a front end that asks for a second factor asks once.
// libssh objects must never be used by two threads at once, so a front
// end's session and its channels belong to one thread from login to logout
// (frontEndMain).  The other threads reach it by a mutex-protected request
// queue and a wake-up pipe, and reach their direct-tcpip channel through a
// socketpair, which is a plain descriptor: an inner session's transport
// (libssh has no way to run a session over a channel) is one end of the
// pair, and the owner pumps the other end to and from the channel.
struct SshJump {
  SshJump() : appFd(-1) {}
  int appFd;
};

namespace {

struct FwdReq {
  FwdReq() : stop(false), port(0), done(false), ok(false), refused(false), appFd(-1) {}
  bool stop;                       // not a connection request: end the thread
  std::function<ssh_session(std::string&)> login;
  std::string host;
  int port;
  bool done, ok, refused;          // the reply
  std::string error;
  int appFd;
};

struct SharedFrontEnd {
  SharedFrontEnd() : idleSec(0), logins(0) { ctl[0] = ctl[1] = -1; }
  ~SharedFrontEnd() { if (ctl[0] >= 0) close(ctl[0]); if (ctl[1] >= 0) close(ctl[1]); }
  std::string key;
  std::mutex mu;
  std::condition_variable cv;
  std::deque<FwdReq*> queue;
  int ctl[2];
  int idleSec;                     // how long an unused session stays logged in
  std::atomic<bool> forwardRefused{false};
  std::atomic<int> logins;
};

std::mutex poolMu;                 // before SharedFrontEnd::mu, always
std::map<std::string, std::shared_ptr<SharedFrontEnd> > pool;
std::atomic<int> totalLogins(0);
std::atomic<bool> poolStopping(false);
std::atomic<int> uniqueKey(0);

struct Chan {
  Chan() : ch(0), pumpFd(-1), off(0), sockEof(false), chEof(false) {}
  ssh_channel ch;
  int pumpFd;
  std::string toCh, toApp;
  size_t off;
  bool sockEof, chEof;
};

void closeChan(Chan& c)
{
  if (c.pumpFd >= 0) {
    shutdown(c.pumpFd, SHUT_RDWR);
    close(c.pumpFd);
    c.pumpFd = -1;
  }
  if (c.ch) {
    ssh_channel_close(c.ch);
    ssh_channel_free(c.ch);
    c.ch = 0;
  }
}

// One non-blocking pass over a channel.  True when it is finished.
// connectionLost is set when libssh reports an error on the session.
bool pumpChan(Chan& c, bool& connectionLost)
{
  char buf[16384];
  if (!c.sockEof && c.off >= c.toCh.size()) {
    ssize_t n = read(c.pumpFd, buf, sizeof buf);
    if (n > 0) { c.toCh.assign(buf, n); c.off = 0; }
    else if (n == 0 || (errno != EAGAIN && errno != EINTR)) c.sockEof = true;
  }
  if (c.off < c.toCh.size()) {
    int n = ssh_channel_write(c.ch, c.toCh.data() + c.off,
                              (uint32_t)(c.toCh.size() - c.off));
    if (n == SSH_ERROR) { connectionLost = true; return true; }
    if (n > 0) c.off += n;
  } else if (c.sockEof) {
    return true;
  }
  if (c.toApp.empty() && !c.chEof) {
    int n = ssh_channel_read_nonblocking(c.ch, buf, sizeof buf, 0);
    if (n == SSH_EOF) c.chEof = true;
    else if (n == SSH_ERROR) { connectionLost = true; return true; }
    else if (n > 0) c.toApp.append(buf, n);
    ssh_channel_read_nonblocking(c.ch, buf, sizeof buf, 1);
  }
  if (!c.toApp.empty()) {
    ssize_t w = send(c.pumpFd, c.toApp.data(), c.toApp.size(), MSG_NOSIGNAL | MSG_DONTWAIT);
    if (w > 0) c.toApp.erase(0, w);
    else if (w < 0 && errno != EAGAIN && errno != EINTR) return true;
  }
  return (c.chEof || ssh_channel_is_closed(c.ch)) && c.toApp.empty();
}

void finishReq(SharedFrontEnd* fe, FwdReq* r)
{
  std::lock_guard<std::mutex> lock(fe->mu);
  r->done = true;
  fe->cv.notify_all();
}

// The thread that owns one front end's session and channels.
void frontEndMain(std::shared_ptr<SharedFrontEnd> fep)
{
  SharedFrontEnd* fe = fep.get();
  ssh_session sess = 0;
  std::vector<Chan> chans;
  Clock::time_point idleSince = Clock::now();

  auto dropSession = [&]() {
    for (size_t i = 0; i < chans.size(); i++) closeChan(chans[i]);
    chans.clear();
    if (sess) {
      ssh_set_blocking(sess, 1);
      ssh_disconnect(sess);
      ssh_free(sess);
      sess = 0;
    }
  };

  // One forward over the shared session, logging in first when there is none.
  auto open = [&](FwdReq* r) {
    for (int attempt = 0; attempt < 2; attempt++) {
      if (sess && !ssh_is_connected(sess)) dropSession();
      if (!sess) {
        std::string err;
        sess = r->login(err);
        if (!sess) { r->error = err; return; }
        fe->logins++;
        totalLogins++;
      }
      ssh_set_blocking(sess, 1);
      ssh_channel fw = ssh_channel_new(sess);
      int rc = fw ? ssh_channel_open_forward(fw, r->host.c_str(), r->port,
                                             "127.0.0.1", 0) : SSH_ERROR;
      ssh_set_blocking(sess, 0);
      if (rc != SSH_OK) {
        std::string why = ssh_get_error(sess);
        if (fw) ssh_channel_free(fw);
        // A dead session is not a refusal: log in again, once.
        if (!ssh_is_connected(sess) && attempt == 0) continue;
        r->error = why;
        r->refused = ssh_is_connected(sess);
        if (r->refused) {
          fe->forwardRefused = true;
          // A session that forwards nothing is of no use: log out.
          if (chans.empty()) dropSession();
        }
        return;
      }
      int sp[2];
      if (socketpairCloexec(AF_UNIX, SOCK_STREAM, 0, sp) != 0) {
        r->error = strerror(errno);
        ssh_channel_free(fw);
        return;
      }
      Chan c;
      c.ch = fw;
      c.pumpFd = sp[1];
      fcntl(c.pumpFd, F_SETFL, fcntl(c.pumpFd, F_GETFL) | O_NONBLOCK);
      chans.push_back(c);
      r->appFd = sp[0];
      r->ok = true;
      return;
    }
  };

  bool stop = false;
  while (!stop) {
    std::deque<FwdReq*> reqs;
    {
      std::lock_guard<std::mutex> lock(fe->mu);
      reqs.swap(fe->queue);
    }
    for (size_t i = 0; i < reqs.size(); i++) {
      if (reqs[i]->stop) stop = true;
      else open(reqs[i]);
      finishReq(fe, reqs[i]);
    }
    if (stop || poolStopping.load()) break;

    bool lost = false;
    for (size_t i = 0; i < chans.size();) {
      if (pumpChan(chans[i], lost)) {
        closeChan(chans[i]);
        chans.erase(chans.begin() + i);
      } else {
        i++;
      }
    }
    if (lost && sess && !ssh_is_connected(sess)) dropSession();

    if (!chans.empty()) {
      idleSince = Clock::now();
    } else if (std::chrono::duration_cast<std::chrono::seconds>(
                 Clock::now() - idleSince).count() >= fe->idleSec) {
      // Nothing uses the session any more.  Leave under the pool's lock, so
      // that nobody queues a request for a thread that has gone.
      if (sess) dropSession();
      std::lock_guard<std::mutex> pl(poolMu);
      std::lock_guard<std::mutex> lock(fe->mu);
      if (fe->queue.empty()) {
        pool.erase(fe->key);
        break;
      }
    }

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fe->ctl[0], &rfds);
    int maxfd = fe->ctl[0];
    bool busy = false;
    std::vector<ssh_channel> watch;
    for (size_t i = 0; i < chans.size(); i++) {
      Chan& c = chans[i];
      watch.push_back(c.ch);
      if (!c.sockEof && c.off >= c.toCh.size()) {
        FD_SET(c.pumpFd, &rfds);
        maxfd = std::max(maxfd, c.pumpFd);
      } else {
        busy = true;
      }
      if (!c.toApp.empty()) busy = true;
    }
    watch.push_back(0);
    std::vector<ssh_channel> out(watch.size(), (ssh_channel)0);
    struct timeval tv = { 0, busy ? 5000 : 100000 };
    if (chans.empty()) {
      tv.tv_sec = 0; tv.tv_usec = 200000;
      select(maxfd + 1, &rfds, 0, 0, &tv);
    } else {
      ssh_select(&watch[0], &out[0], maxfd + 1, &rfds, &tv);
    }
    if (FD_ISSET(fe->ctl[0], &rfds)) {
      char junk[16];
      if (read(fe->ctl[0], junk, sizeof junk) < 0) {}
    }
  }

  dropSession();
  // Requests that arrived while leaving are answered, not left waiting.
  {
    std::lock_guard<std::mutex> pl(poolMu);
    std::lock_guard<std::mutex> lock(fe->mu);
    pool.erase(fe->key);
    for (size_t i = 0; i < fe->queue.size(); i++) {
      fe->queue[i]->error = "front end session closed";
      fe->queue[i]->done = true;
    }
    fe->queue.clear();
    fe->cv.notify_all();
  }
}

// Opens a direct-tcpip channel to host:port over the shared session for
// key, logging in with login() when there is none.  The request is answered
// by the owner thread; this one waits.
bool openForward(const std::string& key, int idleSec,
                 const std::function<ssh_session(std::string&)>& login,
                 const std::string& host, int port, int& appFd, bool& refused,
                 bool& alreadyRefused, std::string& error)
{
  FwdReq req;
  req.login = login;
  req.host = host;
  req.port = port;
  std::shared_ptr<SharedFrontEnd> fe;
  {
    std::lock_guard<std::mutex> pl(poolMu);
    std::map<std::string, std::shared_ptr<SharedFrontEnd> >::iterator it = pool.find(key);
    alreadyRefused = it != pool.end() && it->second->forwardRefused.load();
    if (alreadyRefused) return false;
    if (it == pool.end()) {
      fe.reset(new SharedFrontEnd);
      fe->key = key;
      fe->idleSec = idleSec;
      if (pipeCloexec(fe->ctl) != 0) {
        error = strerror(errno);
        return false;
      }
      pool[key] = fe;
      std::thread(frontEndMain, fe).detach();
    } else {
      fe = it->second;
    }
    std::lock_guard<std::mutex> lock(fe->mu);
    fe->queue.push_back(&req);
    // Under the pool's lock the owner cannot be leaving: it leaves only
    // when its queue is empty and it holds that lock.
    if (write(fe->ctl[1], "r", 1) < 0) {}
  }
  {
    std::unique_lock<std::mutex> lock(fe->mu);
    fe->cv.wait(lock, [&] { return req.done; });
  }
  if (req.ok) { appFd = req.appFd; refused = false; return true; }
  refused = req.refused;
  error = req.error;
  return false;
}

}  // namespace

int SshTransport::frontEndLogins() { return totalLogins.load(); }

void SshTransport::shutdownFrontEnds()
{
  poolStopping = true;
  for (int i = 0; i < 300; i++) {
    {
      std::lock_guard<std::mutex> pl(poolMu);
      if (pool.empty()) break;
      for (std::map<std::string, std::shared_ptr<SharedFrontEnd> >::iterator it = pool.begin();
           it != pool.end(); ++it)
        if (write(it->second->ctl[1], "x", 1) < 0) {}
    }
    usleep(10000);
  }
  poolStopping = false;
}

// Call after the inner session is freed: the owner thread sees EOF on its
// end of the pair and closes the channel.
static void closeJump(SshJump* j)
{
  if (!j) return;
  close(j->appFd);
  delete j;
}

ssh_session SshTransport::newSession(std::string& error, SshJump** linkOut)
{
  if (linkOut) *linkOut = 0;
  if (p_jumpHost.empty())
    return rawSession(p_host, p_port, p_user, p_prompt, -1, error);

  const PromptFn jumpPrompt = p_jumpPrompt ? p_jumpPrompt : p_prompt;
  if (p_feMode == FE_NESTED || p_nested) {
    p_nested = true;
    return rawSession(p_jumpHost, p_jumpPort, p_jumpUser, jumpPrompt, -1, error);
  }

  // One session per front end, user and way of logging in; with
  // ECCE_SSH_FRONTEND_POOL=0 each inner session gets one of its own.
  std::string key = p_jumpUser + "@" + p_jumpHost + ":" + std::to_string(p_jumpPort) +
                    "|" + p_configFile + "|" + p_knownHosts;
  for (size_t i = 0; i < p_identities.size(); i++) key += "|" + p_identities[i];
  const char* pe = getenv("ECCE_SSH_FRONTEND_POOL");
  const bool pooled = !(pe && !strcmp(pe, "0"));
  if (!pooled) key += "|#" + std::to_string(uniqueKey++);
  const char* ie = getenv("ECCE_SSH_FRONTEND_IDLE");
  const int idleSec = !pooled ? 0 : ie && *ie ? atoi(ie) : 120;

  int port = p_port > 0 ? p_port : 22;
  int appFd = -1;
  bool refused = false, alreadyRefused = false;
  std::string why;
  auto login = [this, &jumpPrompt](std::string& err) {
    return rawSession(p_jumpHost, p_jumpPort, p_jumpUser, jumpPrompt, -1, err);
  };
  if (!openForward(key, idleSec, login, p_host, port, appFd, refused,
                   alreadyRefused, why)) {
    if (alreadyRefused) { refused = true; why = "forwarding was refused earlier"; }
    if (refused && p_feMode == FE_AUTO && (alreadyRefused || !p_forwardWorked)) {
      if (getenv("ECCE_RCOM_LOGMODE"))
        fprintf(stderr, "%s refused a forward to %s (%s); running ssh there "
                "instead\n", p_jumpHost.c_str(), p_host.c_str(), why.c_str());
      p_nested = true;
      return rawSession(p_jumpHost, p_jumpPort, p_jumpUser, jumpPrompt, -1, error);
    }
    if (refused)
      error = p_jumpHost + " cannot forward a connection to " + p_host + ": " + why;
    else
      error = why;
    return 0;
  }
  p_forwardWorked = true;

  SshJump* j = new SshJump;
  j->appFd = appFd;

  // libssh closes the descriptor it is given; we keep the original so the
  // owner thread sees EOF only when we say so.
  int fdc = dup(appFd);
  p_authFailed = false;
  ssh_session inner = rawSession(p_host, p_port, p_user, p_prompt, fdc, error);
  if (!inner) {
    closeJump(j);
    // The target may accept only the front end's keys, as cluster nodes
    // often do; ssh from the front end is what the pty path always did.
    if (p_authFailed && p_feMode == FE_AUTO) {
      std::string e2;
      ssh_session again = rawSession(p_jumpHost, p_jumpPort, p_jumpUser,
                                     jumpPrompt, -1, e2);
      if (again) {
        if (getenv("ECCE_RCOM_LOGMODE"))
          fprintf(stderr, "%s; running ssh from %s instead\n", error.c_str(),
                  p_jumpHost.c_str());
        p_nested = true;
        return again;
      }
    }
    return 0;
  }
  if (linkOut) *linkOut = j;
  else closeJump(j);   // not reached: every caller wants the link
  return inner;
}

ssh_session SshTransport::rawSession(const std::string& host, int port,
                                     const std::string& user,
                                     const PromptFn& prompt, int fd,
                                     std::string& error)
{
  ssh_session s = ssh_new();
  if (!s) { error = "ssh_new failed"; return 0; }

  ssh_options_set(s, SSH_OPTIONS_HOST, host.c_str());
  if (port > 0) ssh_options_set(s, SSH_OPTIONS_PORT, &port);
  if (!user.empty()) ssh_options_set(s, SSH_OPTIONS_USER, user.c_str());
  ssh_options_set(s, SSH_OPTIONS_TIMEOUT, &p_connectTimeout);
  if (p_useConfig) {
    const char* cf = p_configFile.empty() ? 0 : p_configFile.c_str();
    if (ssh_options_parse_config(s, cf) != 0) {
      error = "cannot read ssh configuration";
      ssh_free(s);
      return 0;
    }
  } else {
    int off = 0;
    ssh_options_set(s, SSH_OPTIONS_PROCESS_CONFIG, &off);
  }
  // Over a forwarded channel the name is only for known_hosts: a HostName
  // in the config would record the key under a name the user never typed.
  if (fd >= 0) {
    ssh_options_set(s, SSH_OPTIONS_HOST, host.c_str());
    ssh_options_set(s, SSH_OPTIONS_FD, &fd);
  }
  if (!p_knownHosts.empty())
    ssh_options_set(s, SSH_OPTIONS_KNOWNHOSTS, p_knownHosts.c_str());
  else if (const char* home = getenv("HOME"))
    mkdir((std::string(home) + "/.ssh").c_str(), 0700);
  for (size_t i = 0; i < p_identities.size(); i++)
    ssh_options_set(s, SSH_OPTIONS_ADD_IDENTITY, p_identities[i].c_str());

  if (ssh_connect(s) != SSH_OK) {
    error = std::string("cannot connect to ") + host + ": " + ssh_get_error(s);
    ssh_free(s);
    return 0;
  }
  if (!checkHostKey(s, host, error) || !authenticate(s, host, user, prompt, error)) {
    ssh_disconnect(s);
    ssh_free(s);
    return 0;
  }
  setTcpKeepalive(s, keepaliveSec());
  return s;
}

bool SshTransport::checkHostKey(ssh_session s, const std::string& host, std::string& error)
{
  enum ssh_known_hosts_e st = ssh_session_is_known_server(s);
  if (st == SSH_KNOWN_HOSTS_OK) return true;

  if (st == SSH_KNOWN_HOSTS_CHANGED || st == SSH_KNOWN_HOSTS_OTHER) {
    error = "the host key of " + host + " has CHANGED since it was recorded "
            "in known_hosts (or differs in type); refusing to connect. "
            "If the change is expected, remove the old entry and retry.";
    return false;
  }
  if (st == SSH_KNOWN_HOSTS_ERROR) {
    error = std::string("host key check failed: ") + ssh_get_error(s);
    return false;
  }

  ssh_key key = 0;
  if (ssh_get_server_publickey(s, &key) != SSH_OK) {
    error = "cannot read the server's host key";
    return false;
  }
  std::string keyType;
  {
    const char* kt = ssh_key_type_to_char(ssh_key_type(key));
    keyType = kt ? kt : "";
    if (keyType.compare(0, 4, "ssh-") == 0) keyType = keyType.substr(4);
    if (keyType.compare(0, 6, "ecdsa-") == 0) keyType = "ecdsa";
    for (size_t i = 0; i < keyType.size(); i++)
      keyType[i] = (char)toupper((unsigned char)keyType[i]);
  }
  unsigned char* hash = 0;
  size_t hlen = 0;
  int rc = ssh_get_publickey_hash(key, SSH_PUBLICKEY_HASH_SHA256, &hash, &hlen);
  ssh_key_free(key);
  if (rc != 0) { error = "cannot fingerprint the host key"; return false; }
  char* fp = ssh_get_fingerprint_hash(SSH_PUBLICKEY_HASH_SHA256, hash, hlen);
  std::string fingerprint = fp ? fp : "";
  ssh_string_free_char(fp);
  ssh_clean_pubkey_hash(&hash);

  if (!p_hostKey || !p_hostKey(host, fingerprint, keyType)) {
    error = "host key of " + host + " (" + fingerprint + ") was not accepted";
    return false;
  }
  if (ssh_session_update_known_hosts(s) != SSH_OK) {
    error = std::string("cannot record the host key: ") + ssh_get_error(s);
    return false;
  }
  return true;
}

// Sets rc to the libssh result; false means the user aborted.
bool SshTransport::authKeyboardInteractive(ssh_session s, const PromptFn& prompt_, int& rc, bool& asked, std::string& error)
{
  rc = ssh_userauth_kbdint(s, 0, 0);
  while (rc == SSH_AUTH_INFO) {
    int n = ssh_userauth_kbdint_getnprompts(s);
    const char* ins = ssh_userauth_kbdint_getinstruction(s);
    std::string instr = ins ? ins : "";
    for (int i = 0; i < n; i++) {
      char echo = 0;
      const char* pr = ssh_userauth_kbdint_getprompt(s, i, &echo);
      std::string prompt = pr ? pr : "";
      if (i == 0 && !instr.empty()) prompt = instr + "\n" + prompt;
      std::string answer;
      asked = true;
      if (!prompt_ || !prompt_(prompt, echo != 0, answer)) {
        error = "authentication cancelled";
        wipe(answer);
        return false;
      }
      ssh_userauth_kbdint_setanswer(s, i, answer.c_str());
      wipe(answer);
    }
    rc = ssh_userauth_kbdint(s, 0, 0);
  }
  return true;
}

bool SshTransport::authenticate(ssh_session s, const std::string& host,
                                const std::string& user, const PromptFn& prompt,
                                std::string& error)
{
  bool kbdintTried = false;
  int passwordsLeft = p_passwordAttempts;
  p_authFailed = false;
  // A partial success (key, then a second factor) needs another round.
  for (int round = 0; round < 4; round++) {
    int rc = ssh_userauth_none(s, 0);
    if (rc == SSH_AUTH_SUCCESS) return true;
    int methods = ssh_userauth_list(s, 0);

    if (methods & SSH_AUTH_METHOD_PUBLICKEY) {
      rc = ssh_userauth_publickey_auto(s, 0, 0);
      if (rc == SSH_AUTH_SUCCESS) return true;
    }
    int partial = (rc == SSH_AUTH_PARTIAL);

    if (!partial && (methods & SSH_AUTH_METHOD_INTERACTIVE) && !kbdintTried) {
      kbdintTried = true;
      bool asked = false;
      if (!authKeyboardInteractive(s, prompt, rc, asked, error)) return false;
      if (rc == SSH_AUTH_SUCCESS) return true;
      partial = (rc == SSH_AUTH_PARTIAL);
      // A failed answered prompt is a failed login; asking the same
      // question again as a password would only repeat a wrong answer.
      if (!partial && asked) {
        error = "authentication failed for " + user + "@" + host;
        p_authFailed = true;
        return false;
      }
    }
    if (partial) continue;

    if ((methods & SSH_AUTH_METHOD_PASSWORD) && prompt) {
      while (passwordsLeft-- > 0) {
        std::string pw;
        if (!prompt("Password for " + (user.empty() ? "" : user + "@") +
                      host + ": ", false, pw)) {
          wipe(pw);
          error = "authentication cancelled";
          return false;
        }
        rc = ssh_userauth_password(s, 0, pw.c_str());
        wipe(pw);
        if (rc == SSH_AUTH_SUCCESS) return true;
        if (rc == SSH_AUTH_PARTIAL) break;
      }
      if (rc == SSH_AUTH_PARTIAL) continue;
    }
    error = "authentication failed for " + user + "@" + host;
    p_authFailed = true;
    return false;
  }
  error = "authentication failed for " + user + "@" + host;
  p_authFailed = true;
  return false;
}

std::string SshTransport::envPrefix(std::string& error) const
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

TransportResult SshTransport::run(const std::string& script, int timeoutSec)
{
  return runImpl(script, timeoutSec, true);
}

TransportResult SshTransport::runImpl(const std::string& script, int timeoutSec,
                                      bool useDir)
{
  TransportResult res;
  if (!p_session) { res.error = "not connected"; return res; }
  if (p_lent) { res.error = "busy: a stream is using this login"; return res; }
  std::string err;
  std::string envp = envPrefix(err);
  if (!err.empty()) { res.error = err; return res; }

  // fd 3 carries the script, stdin is /dev/null, as in DirectTransport.
  std::string full = "exec 3<&-\n" + envp + (useDir ? withDir(script) : script);

  ssh_channel ch = ssh_channel_new(p_session);
  if (!ch) { res.error = "cannot create channel"; return res; }
  if (ssh_channel_open_session(ch) != SSH_OK ||
      ssh_channel_request_exec(ch, execLine("sh -c 'exec 3<&0 </dev/null; . /dev/fd/3'").c_str()) != SSH_OK) {
    res.error = std::string("cannot start command: ") + ssh_get_error(p_session);
    ssh_channel_free(ch);
    return res;
  }

  ssh_set_blocking(p_session, 0);
  size_t sent = 0;
  bool eofSent = false, outEof = false, errEof = false, failed = false;
  Clock::time_point deadline = Clock::now() + std::chrono::seconds(timeoutSec);
  char buf[32768];

  while (!failed && !(outEof && errEof)) {
    bool progress = false;
    if (sent < full.size()) {
      int n = ssh_channel_write(ch, full.data() + sent,
                                (uint32_t)std::min<size_t>(full.size() - sent, 16384));
      if (n == SSH_ERROR) { failed = true; break; }
      if (n > 0) { sent += n; progress = true; }
    } else if (!eofSent) {
      int rc = ssh_channel_send_eof(ch);
      if (rc == SSH_OK) { eofSent = true; progress = true; }
      else if (rc != SSH_AGAIN) { failed = true; break; }
    }
    for (int is_err = 0; is_err < 2; is_err++) {
      bool& eof = is_err ? errEof : outEof;
      if (eof) continue;
      for (;;) {
        int n = ssh_channel_read_nonblocking(ch, buf, sizeof buf, is_err);
        if (n == SSH_EOF) { eof = true; break; }
        if (n == SSH_ERROR) { failed = true; eof = true; break; }
        if (n == 0) break;
        (is_err ? res.err : res.out).append(buf, n);
        progress = true;
      }
    }
    if (ssh_channel_is_eof(ch) || ssh_channel_is_closed(ch)) {
      // Drain whatever is still buffered, then stop.
      if (ssh_channel_poll(ch, 0) <= 0 && ssh_channel_poll(ch, 1) <= 0)
        outEof = errEof = true;
    }
    if (progress) {
      if (timeoutSec > 0) deadline = Clock::now() + std::chrono::seconds(timeoutSec);
      continue;
    }
    if (timeoutSec > 0 && Clock::now() >= deadline) {
      res.timedOut = true;
      res.error = "timed out after " + std::to_string(timeoutSec) + " s";
      break;
    }
    struct timeval tv = { 0, 50000 };
    waitChannel(ch, tv);
  }

  ssh_set_blocking(p_session, 1);
  if (res.timedOut || failed) {
    if (failed) res.error = std::string("connection error: ") + ssh_get_error(p_session);
    ssh_channel_close(ch);
    ssh_channel_free(ch);
    return res;
  }

  // The exit status arrives just before the channel closes.
  res.status = -1;
  ssh_set_blocking(p_session, 0);
  Clock::time_point until = Clock::now() + std::chrono::seconds(5);
  for (;;) {
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 11, 0)
    uint32_t code = 0;
    char* sig = 0;
    int got = ssh_channel_get_exit_state(ch, &code, &sig, 0);
    if (got == SSH_OK) {
      // A signal name, without the "SIG" prefix, as in the exit-signal request.
      static const struct { const char* n; int v; } sigs[] = {
        {"HUP", 1}, {"INT", 2}, {"QUIT", 3}, {"ABRT", 6}, {"KILL", 9},
        {"SEGV", 11}, {"PIPE", 13}, {"ALRM", 14}, {"TERM", 15}};
      res.status = (int)code;
      if (sig) {
        res.status = -1;
        for (size_t k = 0; k < sizeof sigs / sizeof sigs[0]; k++)
          if (!strcmp(sig, sigs[k].n)) res.status = 128 + sigs[k].v;
      }
      ssh_string_free_char(sig);
      break;
    }
#else
    // 0.10 has no exit-signal accessor: a killed command reads as -1.
    int st = ssh_channel_get_exit_status(ch);
    if (st != -1) { res.status = st; break; }
#endif
    if (ssh_channel_is_closed(ch) || Clock::now() >= until) break;
    struct timeval tv = { 0, 20000 };
    waitChannel(ch, tv);
    ssh_channel_read_nonblocking(ch, buf, sizeof buf, 0);
  }
  ssh_set_blocking(p_session, 1);
  ssh_channel_close(ch);
  ssh_channel_free(ch);
  return res;
}

long SshTransport::spawnDetached(const std::string& script, std::string& error,
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

bool SshTransport::sftp(std::string& error)
{
  if (!p_session) { error = "not connected"; return false; }
  if (p_lent) { error = "busy: a stream is using this login"; return false; }
  if (p_sftp) return true;
  p_sftp = sftp_new(p_session);
  if (!p_sftp || sftp_init(p_sftp) != SSH_OK) {
    error = std::string("cannot start SFTP: ") + ssh_get_error(p_session);
    if (p_sftp) { sftp_free(p_sftp); p_sftp = 0; }
    return false;
  }
  return true;
}

bool SshTransport::put(const std::string& localPath, const std::string& remotePath,
                       std::string& error)
{
  if (p_nested) return nestedPut(localPath, remotePath, error);
  int in = open(localPath.c_str(), O_RDONLY);
  if (in < 0) { error = "cannot read " + localPath + ": " + strerror(errno); return false; }
  struct stat sb;
  fstat(in, &sb);
  if (!sftp(error)) { close(in); return false; }
  sftp_file f = sftp_open(p_sftp, remotePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
                          sb.st_mode & 07777);
  if (!f) {
    error = "cannot create " + remotePath + ": " + ssh_get_error(p_session);
    close(in);
    return false;
  }
  bool ok = true;
  char buf[32768];
  for (;;) {
    ssize_t n = read(in, buf, sizeof buf);
    if (n < 0) { error = "read error on " + localPath; ok = false; break; }
    if (n == 0) break;
    for (ssize_t off = 0; off < n;) {
      ssize_t w = sftp_write(f, buf + off, n - off);
      if (w < 0) {
        error = "write to " + remotePath + " failed: " + ssh_get_error(p_session);
        ok = false;
        break;
      }
      off += w;
    }
    if (!ok) break;
  }
  sftp_close(f);
  close(in);
  // The server's umask trimmed the creation mode; restore what was asked.
  if (ok && sftp_chmod(p_sftp, remotePath.c_str(), sb.st_mode & 07777) != SSH_OK) {
    error = "cannot set mode of " + remotePath + ": " + ssh_get_error(p_session);
    ok = false;
  }
  return ok;
}

bool SshTransport::get(const std::string& remotePath, const std::string& localPath,
                       std::string& error)
{
  if (p_nested) return nestedGet(remotePath, localPath, error);
  if (!sftp(error)) return false;
  sftp_attributes attr = sftp_stat(p_sftp, remotePath.c_str());
  if (!attr) {
    error = "cannot stat " + remotePath + ": " + ssh_get_error(p_session);
    return false;
  }
  mode_t mode = (attr->flags & SSH_FILEXFER_ATTR_PERMISSIONS) ? (attr->permissions & 07777) : 0644;
  sftp_attributes_free(attr);
  sftp_file f = sftp_open(p_sftp, remotePath.c_str(), O_RDONLY, 0);
  if (!f) {
    error = "cannot open " + remotePath + ": " + ssh_get_error(p_session);
    return false;
  }
  int out = open(localPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (out < 0) {
    error = "cannot write " + localPath + ": " + strerror(errno);
    sftp_close(f);
    return false;
  }
  bool ok = true;
  char buf[32768];
  for (;;) {
    ssize_t n = sftp_read(f, buf, sizeof buf);
    if (n < 0) { error = "read of " + remotePath + " failed: " + ssh_get_error(p_session); ok = false; break; }
    if (n == 0) break;
    for (ssize_t off = 0; off < n;) {
      ssize_t w = write(out, buf + off, n - off);
      if (w < 0) { error = "write error on " + localPath; ok = false; break; }
      off += w;
    }
    if (!ok) break;
  }
  sftp_close(f);
  if (ok) fchmod(out, mode);
  close(out);
  return ok;
}


// ---- files without SFTP (nested): base64 through the script channel ----

bool SshTransport::nestedPut(const std::string& local, const std::string& remote,
                             std::string& error)
{
  int in = open(local.c_str(), O_RDONLY);
  if (in < 0) { error = "cannot read " + local + ": " + strerror(errno); return false; }
  struct stat sb;
  fstat(in, &sb);
  std::string data;
  char buf[32768];
  for (ssize_t n; (n = read(in, buf, sizeof buf)) > 0;) data.append(buf, n);
  close(in);
  char mode[16];
  snprintf(mode, sizeof mode, "%04o", (unsigned)(sb.st_mode & 07777));
  std::string p = shPath(remote);
  TransportResult r = runImpl("base64 -d > " + p + " <<'ECCE_B64_END'\n" +
    b64Encode(data) + "ECCE_B64_END\nrc=$?\n[ $rc = 0 ] && chmod " + mode + " " + p +
    "\nexit $?\n", 120, false);
  if (r.status != 0 || !r.error.empty()) {
    error = "cannot write " + remote + ": " + (r.error.empty() ? r.err : r.error);
    return false;
  }
  return true;
}

bool SshTransport::nestedGet(const std::string& remote, const std::string& local,
                             std::string& error)
{
  std::string p = shPath(remote);
  TransportResult r = runImpl("p=" + p + "\n[ -f \"$p\" ] && [ -r \"$p\" ] || "
    "{ echo \"cannot open $p\" >&2; exit 2; }\n"
    "stat -c %a \"$p\" 2>/dev/null || stat -f %Lp \"$p\" || echo 644\n"
    "base64 < \"$p\"\n", 120, false);
  if (r.status != 0 || !r.error.empty()) {
    error = "cannot read " + remote + ": " + (r.error.empty() ? r.err : r.error);
    return false;
  }
  size_t nl = r.out.find('\n');
  mode_t mode = nl == std::string::npos ? 0644 : (mode_t)strtol(r.out.c_str(), 0, 8);
  std::string data = nl == std::string::npos ? "" : b64Decode(r.out.substr(nl + 1));
  int out = open(local.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (out < 0) { error = "cannot write " + local + ": " + strerror(errno); return false; }
  bool ok = true;
  for (size_t off = 0; off < data.size();) {
    ssize_t w = write(out, data.data() + off, data.size() - off);
    if (w < 0) { error = "write error on " + local; ok = false; break; }
    off += w;
  }
  if (ok) fchmod(out, mode & 07777);
  close(out);
  return ok;
}

bool SshTransport::nestedPutInto(const std::string& local, const std::string& remote,
                                 std::string& error)
{
  struct stat sb;
  if (stat(local.c_str(), &sb) != 0) {
    error = "cannot read " + local + ": " + strerror(errno);
    return false;
  }
  if (!S_ISDIR(sb.st_mode)) return nestedPut(local, remote, error);
  std::string p = shPath(remote);
  TransportResult r = runImpl("[ -d " + p + " ] || mkdir " + p + "\n", 30, false);
  if (r.status != 0) { error = "cannot create directory " + remote + ": " + r.err; return false; }
  DIR* d = opendir(local.c_str());
  if (!d) { error = "cannot read " + local + ": " + strerror(errno); return false; }
  std::vector<std::string> names;
  while (struct dirent* e = readdir(d)) {
    std::string n = e->d_name;
    if (n != "." && n != "..") names.push_back(n);
  }
  closedir(d);
  std::sort(names.begin(), names.end());
  for (size_t i = 0; i < names.size(); i++) {
    std::string l = local + (local.empty() || local[local.size()-1] == '/' ? "" : "/") + names[i];
    std::string rm = remote + (remote.empty() || remote[remote.size()-1] == '/' ? "" : "/") + names[i];
    if (!nestedPutInto(l, rm, error)) return false;
  }
  return true;
}

// ---- scp -r style trees ----

namespace {

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

// sftp has no shell to expand a leading ~.
std::string homeRelative(const std::string& p)
{
  if (p == "~") return ".";
  if (p.compare(0, 2, "~/") == 0) return p.substr(2).empty() ? "." : p.substr(2);
  return p;
}

}  // namespace

int SshTransport::remoteKind(const std::string& remotePath)
{
  if (p_nested) {
    TransportResult r = runImpl("p=" + shPath(remotePath) + "\nif [ -d \"$p\" ]; "
      "then echo 1; elif [ -e \"$p\" ]; then echo 0; else echo -1; fi\n", 30, false);
    return r.status == 0 ? atoi(r.out.c_str()) : -1;
  }
  std::string err;
  if (!sftp(err)) return -1;
  sftp_attributes a = sftp_stat(p_sftp, homeRelative(remotePath).c_str());
  if (!a) return -1;
  int kind = a->type == SSH_FILEXFER_TYPE_DIRECTORY ? 1 : 0;
  sftp_attributes_free(a);
  return kind;
}

bool SshTransport::remoteGlob(const std::string& pattern,
                              std::vector<std::string>& out, std::string& error)
{
  out.clear();
  std::string pat = homeRelative(pattern);
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
  size_t pos = 0;
  while (pos < r.out.size()) {
    size_t nl = r.out.find('\n', pos);
    if (nl == std::string::npos) nl = r.out.size();
    if (nl > pos) out.push_back(r.out.substr(pos, nl - pos));
    pos = nl + 1;
  }
  if (out.empty()) {
    error = pattern + ": No such file or directory";
    return false;
  }
  return true;
}

static bool putInto(SshTransport* t, sftp_session sf, ssh_session ss,
                    const std::string& local, const std::string& remote,
                    std::string& error, bool (SshTransport::*putFile)(
                      const std::string&, const std::string&, std::string&))
{
  struct stat sb;
  if (stat(local.c_str(), &sb) != 0) {
    error = "cannot read " + local + ": " + strerror(errno);
    return false;
  }
  if (!S_ISDIR(sb.st_mode)) return (t->*putFile)(local, remote, error);

  sftp_attributes a = sftp_stat(sf, remote.c_str());
  if (!a) {
    if (sftp_mkdir(sf, remote.c_str(), (sb.st_mode & 0777) | 0700) != SSH_OK) {
      error = "cannot create directory " + remote + ": " + ssh_get_error(ss);
      return false;
    }
  } else {
    sftp_attributes_free(a);
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
    if (!putInto(t, sf, ss, joinPath(local, names[i]), joinPath(remote, names[i]),
                 error, putFile))
      return false;
  return true;
}

bool SshTransport::putTree(const std::string& localPath,
                           const std::string& remotePath, std::string& error)
{
  if (p_nested) {
    std::string target = remotePath;
    if (remoteKind(remotePath) == 1) target = joinPath(target, baseName(localPath));
    return nestedPutInto(localPath, target, error);
  }
  if (!sftp(error)) return false;
  std::string target = homeRelative(remotePath);
  if (remoteKind(remotePath) == 1) target = joinPath(target, baseName(localPath));
  return putInto(this, p_sftp, p_session, localPath, target, error,
                 &SshTransport::put);
}

bool SshTransport::getTree(const std::string& remotePath,
                           const std::string& localPath, std::string& error)
{
  if (p_nested) {
    struct stat nb;
    std::string tgt = localPath;
    if (stat(localPath.c_str(), &nb) == 0 && S_ISDIR(nb.st_mode))
      tgt = joinPath(localPath, baseName(remotePath));
    int kind = remoteKind(remotePath);
    if (kind < 0) { error = "cannot stat " + remotePath; return false; }
    if (kind == 0) return nestedGet(remotePath, tgt, error);
    std::string top = shPath(remotePath);
    TransportResult d = runImpl("cd " + top + " && find . -type d\n", 60, false);
    TransportResult f = runImpl("cd " + top + " && find . ! -type d\n", 60, false);
    if (d.status != 0 || f.status != 0) {
      error = "cannot list " + remotePath + ": " + d.err + f.err;
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
      if (files[i].size() > 2 && !nestedGet(joinPath(remotePath, files[i].substr(2)),
                                            joinPath(tgt, files[i].substr(2)), error))
        return false;
    return true;
  }
  if (!sftp(error)) return false;
  std::string remote = homeRelative(remotePath);
  struct stat sb;
  std::string target = localPath;
  if (stat(localPath.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode))
    target = joinPath(localPath, baseName(remote));

  struct Walk {
    static bool go(SshTransport* t, sftp_session sf, ssh_session ss,
                   const std::string& remote, const std::string& local,
                   std::string& error)
    {
      sftp_attributes a = sftp_stat(sf, remote.c_str());
      if (!a) {
        error = "cannot stat " + remote + ": " + ssh_get_error(ss);
        return false;
      }
      bool dir = a->type == SSH_FILEXFER_TYPE_DIRECTORY;
      mode_t mode = (a->flags & SSH_FILEXFER_ATTR_PERMISSIONS)
                    ? (a->permissions & 0777) : 0755;
      sftp_attributes_free(a);
      if (!dir) return t->get(remote, local, error);

      if (mkdir(local.c_str(), mode | 0700) != 0 && errno != EEXIST) {
        error = "cannot create " + local + ": " + strerror(errno);
        return false;
      }
      sftp_dir d = sftp_opendir(sf, remote.c_str());
      if (!d) {
        error = "cannot read " + remote + ": " + ssh_get_error(ss);
        return false;
      }
      std::vector<std::string> names;
      while (sftp_attributes e = sftp_readdir(sf, d)) {
        std::string n = e->name;
        sftp_attributes_free(e);
        if (n != "." && n != "..") names.push_back(n);
      }
      sftp_closedir(d);
      std::sort(names.begin(), names.end());
      for (size_t i = 0; i < names.size(); i++)
        if (!go(t, sf, ss, joinPath(remote, names[i]), joinPath(local, names[i]),
                error))
          return false;
      return true;
    }
  };
  return Walk::go(this, p_sftp, p_session, remote, target, error);
}

// ---- the monitor stream ----

struct SshStream : RemoteStream {
  SshStream() : session(0), ownsSession(true), channel(0), jump(0), appFd(-1),
                pumpFd(-1), graceMs(2000), intr(false), stop(false)
  { ctl[0] = ctl[1] = -1; }
  ssh_session session;
  bool ownsSession;        // false: the transport's own login, lent to it
  ssh_channel channel;
  SshJump* jump;
  int appFd, pumpFd, ctl[2];
  std::string header;      // the script, first thing on the channel's stdin
  std::thread thread;
  std::atomic<int> graceMs;
  std::atomic<bool> intr, stop;
};

namespace {

// The exec request is a one-liner that reads the script from the first
// line of stdin and evals it, leaving the rest of stdin to the script.
// Newlines and backslashes are escaped for printf %b so it is one line.
const char* const kStreamExec =
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

void pump(SshStream* st)
{
  ssh_session s = st->session;
  ssh_channel ch = st->channel;
  ssh_set_blocking(s, 0);

  std::string toCh = st->header, toApp;
  size_t toChOff = 0;
  bool sockEof = false, eofSent = false, outEof = false, errEof = false;
  bool fail = false, stopping = false;
  Clock::time_point deadline;
  char buf[16384];

  const int kaSec = keepaliveSec();
  const int kaFd = ssh_get_fd(s);
  long long lastRx = receivedBytes(kaFd);
  Clock::time_point lastHeard = Clock::now(), lastPing = lastHeard;

  while (!fail) {
    if (kaSec > 0 && lastRx >= 0 && !stopping) {
      Clock::time_point now = Clock::now();
      long long rx = receivedBytes(kaFd);
      if (rx != lastRx) { lastRx = rx; lastHeard = lastPing = now; }
      std::chrono::seconds quiet =
        std::chrono::duration_cast<std::chrono::seconds>(now - lastHeard);
      if (quiet.count() >= 3 * kaSec) {
        fprintf(stderr, "ssh keepalive: nothing heard from the host for %d s; "
                "treating the connection as dead\n", (int)quiet.count());
        fflush(stderr);
        fail = true;
        break;
      }
      if (now - lastPing >= std::chrono::seconds(kaSec)) {
        ssh_send_keepalive(s);
        lastPing = now;
      }
    }

    if (st->intr.exchange(false))
      ssh_channel_request_send_signal(ch, "INT");
    if (st->stop.load() && !stopping) {
      stopping = true;
      sockEof = true;
      deadline = Clock::now() + std::chrono::milliseconds(st->graceMs.load());
    }

    if (!sockEof && toChOff >= toCh.size()) {
      ssize_t n = read(st->pumpFd, buf, 4096);
      if (n > 0) { toCh.assign(buf, n); toChOff = 0; }
      else if (n == 0 || (errno != EAGAIN && errno != EINTR)) sockEof = true;
    }
    if (toChOff < toCh.size()) {
      int n = ssh_channel_write(ch, toCh.data() + toChOff,
                                (uint32_t)(toCh.size() - toChOff));
      if (n == SSH_ERROR) { fail = true; break; }
      if (n > 0) toChOff += n;
    } else if (sockEof && !eofSent) {
      int rc = ssh_channel_send_eof(ch);
      if (rc == SSH_OK) eofSent = true;
      else if (rc != SSH_AGAIN) { fail = true; break; }
    }

    if (toApp.empty()) {
      for (int is_err = 0; is_err < 2; is_err++) {
        bool& eof = is_err ? errEof : outEof;
        if (eof) continue;
        int n = ssh_channel_read_nonblocking(ch, buf, sizeof buf, is_err);
        if (n == SSH_EOF) eof = true;
        else if (n == SSH_ERROR) { fail = true; break; }
        else if (n > 0) { toApp.append(buf, n); break; }
      }
    }
    if (!toApp.empty()) {
      ssize_t w = send(st->pumpFd, toApp.data(), toApp.size(),
                       MSG_NOSIGNAL | MSG_DONTWAIT);
      if (w > 0) toApp.erase(0, w);
      else if (w < 0 && errno != EAGAIN && errno != EINTR) toApp.clear();
    }

    bool done = (outEof && errEof) || ssh_channel_is_closed(ch);
    if (done && toApp.empty()) break;
    if (stopping && Clock::now() >= deadline) break;

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(st->ctl[0], &rfds);
    int maxfd = st->ctl[0];
    if (!sockEof && toChOff >= toCh.size()) {
      FD_SET(st->pumpFd, &rfds);
      maxfd = std::max(maxfd, st->pumpFd);
    }
    struct timeval tv = { 0, toApp.empty() && toChOff >= toCh.size() ? 100000 : 5000 };
    if (outEof && errEof) {
      select(maxfd + 1, &rfds, 0, 0, &tv);
    } else {
      ssh_channel chans[2] = { ch, 0 };
      ssh_channel outc[2] = { 0, 0 };
      ssh_select(chans, outc, maxfd + 1, &rfds, &tv);
    }
    if (FD_ISSET(st->ctl[0], &rfds)) {
      char c[16];
      if (read(st->ctl[0], c, sizeof c) < 0) {}
    }
  }

  // The app sees EOF first: closing a dead link can take a while.
  shutdown(st->pumpFd, SHUT_RDWR);
  close(st->pumpFd);
  st->pumpFd = -1;
  ssh_channel_close(ch);
  ssh_channel_free(ch);
  if (st->ownsSession) {
    ssh_disconnect(s);
    ssh_free(s);
    closeJump(st->jump);
  } else {
    ssh_set_blocking(s, 1);
  }
  st->jump = 0;
  st->channel = 0;
  st->session = 0;
}

}  // namespace

RemoteStream* SshTransport::openStream(const std::string& script, int& fd,
                                       std::string& error)
{
  std::string err;
  std::string envp = envPrefix(err);
  if (!err.empty()) { error = err; return 0; }

  SshJump* link = 0;
  ssh_session s = newSession(error, &link);
  if (!s) return 0;

  ssh_channel ch = ssh_channel_new(s);
  if (!ch || ssh_channel_open_session(ch) != SSH_OK ||
      ssh_channel_request_exec(ch, execLine(kStreamExec).c_str()) != SSH_OK) {
    error = std::string("cannot start command: ") + ssh_get_error(s);
    if (ch) ssh_channel_free(ch);
    ssh_disconnect(s);
    ssh_free(s);
    closeJump(link);
    return 0;
  }

  int sp[2], ctl[2];
  if (socketpairCloexec(AF_UNIX, SOCK_STREAM, 0, sp) != 0) {
    error = strerror(errno);
  } else if (pipeCloexec(ctl) != 0) {
    error = strerror(errno);
    close(sp[0]); close(sp[1]);
  } else {
    SshStream* st = new SshStream;
    st->session = s;
    st->channel = ch;
    st->jump = link;
    st->appFd = sp[0];
    st->pumpFd = sp[1];
    st->ctl[0] = ctl[0];
    st->ctl[1] = ctl[1];
    st->header = oneLine(envp + withDir(script));
    fcntl(st->pumpFd, F_SETFL, fcntl(st->pumpFd, F_GETFL) | O_NONBLOCK);
    st->thread = std::thread(pump, st);
    fd = st->appFd;
    return st;
  }
  ssh_channel_close(ch);
  ssh_channel_free(ch);
  ssh_disconnect(s);
  ssh_free(s);
  closeJump(link);
  return 0;
}

// A channel on p_session itself: no second login, so no second password or
// one-time code.  libssh objects belong to one thread at a time, so the pump
// thread has the session to itself until closeStream() joins it.
RemoteStream* SshTransport::openStreamOnLogin(const std::string& script, int& fd,
                                              std::string& error)
{
  if (!p_session) { error = "not connected"; return 0; }
  if (p_lent) { error = "busy: a stream is using this login"; return 0; }
  std::string err;
  std::string envp = envPrefix(err);
  if (!err.empty()) { error = err; return 0; }

  ssh_channel ch = ssh_channel_new(p_session);
  if (!ch || ssh_channel_open_session(ch) != SSH_OK ||
      ssh_channel_request_exec(ch, execLine(kStreamExec).c_str()) != SSH_OK) {
    error = std::string("cannot start command: ") + ssh_get_error(p_session);
    if (ch) ssh_channel_free(ch);
    return 0;
  }

  int sp[2], ctl[2];
  if (socketpairCloexec(AF_UNIX, SOCK_STREAM, 0, sp) != 0) {
    error = strerror(errno);
  } else if (pipeCloexec(ctl) != 0) {
    error = strerror(errno);
    close(sp[0]); close(sp[1]);
  } else {
    SshStream* st = new SshStream;
    st->session = p_session;
    st->ownsSession = false;
    st->channel = ch;
    st->appFd = sp[0];
    st->pumpFd = sp[1];
    st->ctl[0] = ctl[0];
    st->ctl[1] = ctl[1];
    st->header = oneLine(envp + withDir(script));
    fcntl(st->pumpFd, F_SETFL, fcntl(st->pumpFd, F_GETFL) | O_NONBLOCK);
    p_lent = true;
    st->thread = std::thread(pump, st);
    fd = st->appFd;
    return st;
  }
  ssh_channel_close(ch);
  ssh_channel_free(ch);
  return 0;
}

void SshTransport::interruptStream(RemoteStream* rs)
{
  SshStream* st = static_cast<SshStream*>(rs);
  if (!st) return;
  st->intr = true;
  if (write(st->ctl[1], "i", 1) < 0) {}
}

void SshTransport::closeStream(RemoteStream* rs, int graceMs)
{
  SshStream* st = static_cast<SshStream*>(rs);
  if (!st) return;
  st->graceMs = graceMs;
  st->stop = true;
  if (write(st->ctl[1], "s", 1) < 0) {}
  st->thread.join();
  if (!st->ownsSession) p_lent = false;
  close(st->appFd);
  close(st->ctl[0]);
  close(st->ctl[1]);
  delete st;
}
