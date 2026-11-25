#include "comm/SshMaster.H"
#include "comm/SshTransport.H"

#include <libssh/libssh.h>
#include <libssh/server.h>   // ssh_send_keepalive

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <sddl.h>
#include <io.h>
#include <fcntl.h>
#else
#include <fcntl.h>
#include <poll.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0   // macOS: SO_NOSIGPIPE is set on each socket instead
#endif
#endif

typedef std::chrono::steady_clock Clock;

namespace {

std::string hashHex(const std::string& s)
{
  unsigned long long h = 1469598103934665603ULL;
  for (size_t i = 0; i < s.size(); i++) {
    h ^= (unsigned char)s[i];
    h *= 1099511628211ULL;
  }
  char buf[17];
  snprintf(buf, sizeof buf, "%016llx", h);
  return buf;
}

std::string stateKey()
{
  const char* e = getenv("ECCE_REALUSERHOME");
  if (!e || !*e) e = getenv("HOME");
  if (!e || !*e) e = getenv("USERPROFILE");
  return e ? e : "";
}

#ifndef _WIN32
// The directory the sockets live in: ours, a directory, nobody else's bits.
std::string privateDir()
{
  std::string base;
  const char* x = getenv("XDG_RUNTIME_DIR");
#ifdef __APPLE__
  if (!x || !*x) x = getenv("TMPDIR");
#endif
  base = (x && *x) ? x : "/tmp";
  while (base.size() > 1 && base[base.size()-1] == '/') base.erase(base.size()-1);
  std::string d = base + "/ecce-sshm-" + std::to_string((unsigned long)getuid());
  mkdir(d.c_str(), 0700);
  struct stat sb;
  if (lstat(d.c_str(), &sb) != 0 || !S_ISDIR(sb.st_mode) ||
      sb.st_uid != getuid() || (sb.st_mode & 077) != 0)
    return "";
  return d;
}

void noSigpipe(int fd)
{
#ifdef SO_NOSIGPIPE
  int one = 1;
  setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof one);
#else
  (void)fd;
#endif
}

bool peerIsUs(int fd)
{
#if defined(__linux__)
  struct ucred cr;
  socklen_t l = sizeof cr;
  return getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cr, &l) == 0 && cr.uid == getuid();
#else
  uid_t u; gid_t g;
  return getpeereid(fd, &u, &g) == 0 && u == getuid();
#endif
}
#else
std::string winUser()
{
  char name[256];
  DWORD n = sizeof name;
  return GetUserNameA(name, &n) ? std::string(name) : std::string();
}

// The SID string of the user a token belongs to.
std::string tokenSid(HANDLE tok)
{
  DWORD len = 0;
  GetTokenInformation(tok, TokenUser, 0, 0, &len);
  if (!len) return "";
  std::string buf(len, '\0');
  if (!GetTokenInformation(tok, TokenUser, &buf[0], len, &len)) return "";
  char* s = 0;
  if (!ConvertSidToStringSidA(((TOKEN_USER*)&buf[0])->User.Sid, &s)) return "";
  std::string r = s;
  LocalFree(s);
  return r;
}

std::string ownSid()
{
  HANDLE tok = 0;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tok)) return "";
  std::string r = tokenSid(tok);
  CloseHandle(tok);
  return r;
}

// A pipe name is first come, first served: the server must be ours.
bool serverIsUs(HANDLE pipe)
{
  ULONG pid = 0;
  if (!GetNamedPipeServerProcessId(pipe, &pid)) return false;
  HANDLE p = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  if (!p) return false;
  HANDLE tok = 0;
  std::string sid;
  if (OpenProcessToken(p, TOKEN_QUERY, &tok)) {
    sid = tokenSid(tok);
    CloseHandle(tok);
  }
  CloseHandle(p);
  return !sid.empty() && sid == ownSid();
}
#endif

}  // namespace

namespace SshMaster {

std::string mode()
{
  const char* e = getenv("ECCE_SSH_MASTER");
  return e ? e : "";
}

int idleSeconds()
{
  const char* e = getenv("ECCE_SSH_MASTER_IDLE");
  int v = e ? atoi(e) : 0;
  return v > 0 ? v : 300;
}

std::string endpoint(const std::string& host, int port,
                     const std::string& user, const std::string& jump)
{
  const std::string key = user + "\n" + host + "\n" + std::to_string(port) +
                          "\n" + jump + "\n" + stateKey();
#ifdef _WIN32
  return "\\\\.\\pipe\\ecce-sshm-" + hashHex(winUser() + "\n" + key);
#else
  std::string d = privateDir();
  if (d.empty()) return "";
  std::string p = d + "/" + hashHex(key);
  // sun_path is 104 bytes on macOS; room for ".lock" beside it is not needed.
  return p.size() < 100 ? p : "";
#endif
}

std::string statusPath(const std::string& ep)
{
#ifdef _WIN32
  const char* t = getenv("TEMP");
  return std::string(t ? t : ".") + "\\ecce-sshm-" + hashHex(ep) + "-" +
         std::to_string((unsigned long)GetCurrentProcessId()) + ".st";
#else
  return ep + ".st" + std::to_string((long)getpid());
#endif
}

void writeStatus(const std::string& path, const std::string& text)
{
  if (path.empty()) return;
  FILE* f = fopen(path.c_str(), "wb");
  if (!f) return;
  fputs((text + "\n").c_str(), f);
  fclose(f);
}

std::string readStatus(const std::string& path)
{
  std::string r;
  FILE* f = fopen(path.c_str(), "rb");
  if (!f) return r;
  char buf[4096];
  size_t n;
  while ((n = fread(buf, 1, sizeof buf, f)) > 0) r.append(buf, n);
  fclose(f);
  return r;
}

// ---- client connection ----

#ifdef _WIN32
Conn::Conn() : p_h(INVALID_HANDLE_VALUE) {}
bool Conn::isOpen() const { return p_h != INVALID_HANDLE_VALUE; }

bool Conn::open(const std::string& ep, std::string& error)
{
  close();
  HANDLE h = INVALID_HANDLE_VALUE;
  for (int i = 0; i < 5; i++) {
    h = CreateFileA(ep.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0,
                    OPEN_EXISTING, 0, 0);
    if (h != INVALID_HANDLE_VALUE || GetLastError() != ERROR_PIPE_BUSY) break;
    WaitNamedPipeA(ep.c_str(), 2000);
  }
  if (h == INVALID_HANDLE_VALUE) {
    error = "no ssh master at " + ep;
    return false;
  }
  if (!serverIsUs(h)) {
    CloseHandle(h);
    error = "the ssh master pipe " + ep + " belongs to another user";
    return false;
  }
  p_h = h;
  return true;
}

bool Conn::writeAll(const std::string& s)
{
  size_t off = 0;
  while (off < s.size()) {
    DWORD w = 0;
    if (!WriteFile((HANDLE)p_h, s.data() + off, (DWORD)(s.size() - off), &w, 0))
      return false;
    off += w;
  }
  return true;
}

int Conn::readSome(char* buf, size_t n, int timeoutMs)
{
  if (!isOpen()) return -1;
  Clock::time_point until = Clock::now() + std::chrono::milliseconds(timeoutMs);
  for (;;) {
    DWORD avail = 0;
    if (!PeekNamedPipe((HANDLE)p_h, 0, 0, 0, &avail, 0)) return -1;
    if (avail > 0) {
      DWORD got = 0;
      if (!ReadFile((HANDLE)p_h, buf, (DWORD)std::min<size_t>(n, avail), &got, 0))
        return -1;
      return (int)got;
    }
    if (timeoutMs >= 0 && Clock::now() >= until) return 0;
    Sleep(5);
  }
}

int Conn::releaseFd()
{
  if (!isOpen()) return -1;
  int fd = _open_osfhandle((intptr_t)p_h, _O_BINARY);
  if (fd < 0) return -1;
  p_h = INVALID_HANDLE_VALUE;
  return fd;
}

void Conn::close()
{
  if (isOpen()) CloseHandle((HANDLE)p_h);
  p_h = INVALID_HANDLE_VALUE;
}
#else
Conn::Conn() : p_fd(-1) {}
bool Conn::isOpen() const { return p_fd >= 0; }

bool Conn::open(const std::string& ep, std::string& error)
{
  close();
  struct sockaddr_un a;
  memset(&a, 0, sizeof a);
  a.sun_family = AF_UNIX;
  if (ep.empty() || ep.size() >= sizeof a.sun_path) {
    error = "bad ssh master path";
    return false;
  }
  strcpy(a.sun_path, ep.c_str());
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) { error = strerror(errno); return false; }
  fcntl(fd, F_SETFD, FD_CLOEXEC);
  noSigpipe(fd);
  if (connect(fd, (struct sockaddr*)&a, sizeof a) != 0) {
    error = "no ssh master at " + ep + ": " + strerror(errno);
    ::close(fd);
    return false;
  }
  p_fd = fd;
  return true;
}

bool Conn::writeAll(const std::string& s)
{
  size_t off = 0;
  while (off < s.size()) {
    ssize_t w = send(p_fd, s.data() + off, s.size() - off, MSG_NOSIGNAL);
    if (w < 0 && errno == EINTR) continue;
    if (w <= 0) return false;
    off += (size_t)w;
  }
  return true;
}

int Conn::readSome(char* buf, size_t n, int timeoutMs)
{
  if (p_fd < 0) return -1;
  struct pollfd p = { p_fd, POLLIN, 0 };
  int r;
  do r = poll(&p, 1, timeoutMs); while (r < 0 && errno == EINTR);
  if (r == 0) return 0;
  if (r < 0) return -1;
  ssize_t got;
  do got = recv(p_fd, buf, n, 0); while (got < 0 && errno == EINTR);
  return got > 0 ? (int)got : -1;
}

int Conn::releaseFd()
{
  int fd = p_fd;
  p_fd = -1;
  return fd;
}

void Conn::close()
{
  if (p_fd >= 0) ::close(p_fd);
  p_fd = -1;
}
#endif

Conn::~Conn() { close(); }

bool Conn::readLine(std::string& line, int timeoutMs)
{
  line.clear();
  Clock::time_point until = Clock::now() + std::chrono::milliseconds(timeoutMs);
  for (;;) {
    int left = timeoutMs < 0 ? -1 : (int)std::chrono::duration_cast<
      std::chrono::milliseconds>(until - Clock::now()).count();
    if (timeoutMs >= 0 && left <= 0) return false;
    char c;
    int n = readSome(&c, 1, left);
    if (n < 0) return false;
    if (n == 0) continue;
    if (c == '\n') return true;
    line += c;
    if (line.size() > 4096) return false;
  }
}

bool live(const std::string& ep)
{
  if (ep.empty()) return false;
  Conn c;
  std::string err, line;
  return c.open(ep, err) && c.writeAll("HELLO\n") &&
         c.readLine(line, 5000) && line.compare(0, 16, "ECCE-SSH-MASTER ") == 0;
}

bool lockRoute(const std::string& ep)
{
#ifdef _WIN32
  HANDLE m = CreateMutexA(0, FALSE, ("Local\\ecce-sshm-" + hashHex(ep)).c_str());
  if (!m) return false;
  DWORD w = WaitForSingleObject(m, 0);
  return w == WAIT_OBJECT_0 || w == WAIT_ABANDONED;
#else
  int fd = ::open((ep + ".lock").c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
  if (fd < 0) return false;
  if (flock(fd, LOCK_EX | LOCK_NB) != 0) { ::close(fd); return false; }
  return true;   // fd stays open: the lock lasts as long as the process
#endif
}

// ---- the master ----

namespace {

enum State { REQ, CONTROL, RUN, STREAM, DONE };

struct Client {
#ifdef _WIN32
  HANDLE h = INVALID_HANDLE_VALUE;
#else
  int fd = -1;
#endif
  State st = REQ;
  std::string in, out;
  ssh_channel ch = 0;
  int id = 0;
  size_t stdinLeft = 0;
  bool clientEof = false, eofSent = false, outEof = false, errEof = false;
  bool finished = false;
  Clock::time_point finishedAt, graceUntil;
};

// >0 bytes, 0 nothing now, -1 the client is gone.
int clientRead(Client& c, char* buf, size_t n)
{
#ifdef _WIN32
  DWORD avail = 0;
  if (!PeekNamedPipe(c.h, 0, 0, 0, &avail, 0)) return -1;
  if (!avail) return 0;
  DWORD got = 0;
  if (!ReadFile(c.h, buf, (DWORD)std::min<size_t>(n, avail), &got, 0)) return -1;
  return (int)got;
#else
  ssize_t got = recv(c.fd, buf, n, MSG_DONTWAIT);
  if (got > 0) return (int)got;
  if (got < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) return 0;
  return -1;
#endif
}

int clientWrite(Client& c, const char* p, size_t n)
{
#ifdef _WIN32
  DWORD w = 0;
  // PIPE_NOWAIT: writes what fits.
  if (!WriteFile(c.h, p, (DWORD)n, &w, 0)) return -1;
  return (int)w;
#else
  ssize_t w = send(c.fd, p, n, MSG_DONTWAIT | MSG_NOSIGNAL);
  if (w >= 0) return (int)w;
  return (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) ? 0 : -1;
#endif
}

void clientClose(Client& c)
{
#ifdef _WIN32
  if (c.h != INVALID_HANDLE_VALUE) CloseHandle(c.h);
  c.h = INVALID_HANDLE_VALUE;
#else
  if (c.fd >= 0) close(c.fd);
  c.fd = -1;
#endif
}

void frame(std::string& out, char type, const std::string& data)
{
  unsigned n = (unsigned)data.size();
  out += type;
  out += (char)(n >> 24); out += (char)(n >> 16); out += (char)(n >> 8); out += (char)n;
  out += data;
}

void closeChannel(Client& c)
{
  if (!c.ch) return;
  ssh_channel_close(c.ch);
  ssh_channel_free(c.ch);
  c.ch = 0;
}

bool exitStatus(ssh_channel ch, std::string& out)
{
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 11, 0)
  uint32_t code = 0;
  char* sig = 0;
  if (ssh_channel_get_exit_state(ch, &code, &sig, 0) != SSH_OK) return false;
  if (sig) {
    out = std::string("sig:") + sig;
    ssh_string_free_char(sig);
  } else {
    out = std::to_string(code);
  }
  return true;
#else
  int st = ssh_channel_get_exit_status(ch);
  if (st == -1) return false;
  out = std::to_string(st);
  return true;
#endif
}

ssh_channel openExec(ssh_session s, const std::string& line, std::string& err)
{
  ssh_set_blocking(s, 1);
  ssh_channel ch = ssh_channel_new(s);
  if (!ch || ssh_channel_open_session(ch) != SSH_OK ||
      ssh_channel_request_exec(ch, line.c_str()) != SSH_OK) {
    err = ssh_get_error(s);
    if (ch) ssh_channel_free(ch);
    ch = 0;
  }
  ssh_set_blocking(s, 0);
  return ch;
}

const size_t kMaxQueued = 1 << 20;

}  // namespace

int serve(SshTransport& t, const std::string& ep, int idleSec,
          const std::string& statusFile)
{
  ssh_session s = t.sessionForMaster();
  if (!s) { writeStatus(statusFile, "failed\nnot logged in"); return 1; }

#ifdef _WIN32
  std::string sddl = "D:P(A;;GA;;;" + ownSid() + ")";
  PSECURITY_DESCRIPTOR psd = 0;
  if (ownSid().empty() ||
      !ConvertStringSecurityDescriptorToSecurityDescriptorA(sddl.c_str(),
                                                            SDDL_REVISION_1, &psd, 0)) {
    writeStatus(statusFile, "failed\ncannot make the pipe's access list");
    return 1;
  }
  SECURITY_ATTRIBUTES sa = { sizeof sa, psd, FALSE };
  auto instance = [&](bool first) {
    return CreateNamedPipeA(ep.c_str(), PIPE_ACCESS_DUPLEX |
                            (first ? FILE_FLAG_FIRST_PIPE_INSTANCE : 0),
                            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT |
                            PIPE_REJECT_REMOTE_CLIENTS,
                            PIPE_UNLIMITED_INSTANCES, 65536, 65536, 0, &sa);
  };
  HANDLE listening = instance(true);
  if (listening == INVALID_HANDLE_VALUE) {
    writeStatus(statusFile, "failed\ncannot create " + ep);
    return 1;
  }
#else
  unlink(ep.c_str());
  int ls = socket(AF_UNIX, SOCK_STREAM, 0);
  struct sockaddr_un a;
  memset(&a, 0, sizeof a);
  a.sun_family = AF_UNIX;
  strncpy(a.sun_path, ep.c_str(), sizeof a.sun_path - 1);
  mode_t old = umask(077);
  bool ok = ls >= 0 && bind(ls, (struct sockaddr*)&a, sizeof a) == 0;
  umask(old);
  if (!ok || listen(ls, 32) != 0) {
    writeStatus(statusFile, "failed\ncannot listen at " + ep + ": " + strerror(errno));
    return 1;
  }
  chmod(ep.c_str(), 0600);
  fcntl(ls, F_SETFD, FD_CLOEXEC);
  fcntl(ls, F_SETFL, fcntl(ls, F_GETFL) | O_NONBLOCK);
#endif

  ssh_set_blocking(s, 0);
  writeStatus(statusFile, "ok");

  std::list<Client> clients;
  int nextId = 1;
  Clock::time_point lastBusy = Clock::now(), lastPing = Clock::now();
  bool sessionLost = false;
  char buf[32768];

  for (;;) {
    // New clients.
#ifdef _WIN32
    for (;;) {
      BOOL c = ConnectNamedPipe(listening, 0);
      DWORD e = c ? ERROR_PIPE_CONNECTED : GetLastError();
      if (e == ERROR_PIPE_CONNECTED) {
        Client cl;
        cl.h = listening;
        clients.push_back(cl);
        listening = instance(false);
        if (listening == INVALID_HANDLE_VALUE) break;
        continue;
      }
      if (e == ERROR_NO_DATA) DisconnectNamedPipe(listening);
      break;
    }
#else
    for (;;) {
      int fd = accept(ls, 0, 0);
      if (fd < 0) break;
      fcntl(fd, F_SETFD, FD_CLOEXEC);
      noSigpipe(fd);
      if (!peerIsUs(fd)) { close(fd); continue; }
      Client cl;
      cl.fd = fd;
      clients.push_back(cl);
    }
#endif

    if (!sessionLost && !ssh_is_connected(s)) sessionLost = true;
    bool busy = false;

    for (std::list<Client>::iterator it = clients.begin(); it != clients.end();) {
      Client& c = *it;
      // From the client.
      while (!c.clientEof && c.in.size() < kMaxQueued) {
        int n = clientRead(c, buf, sizeof buf);
        if (n < 0) { c.clientEof = true; break; }
        if (n == 0) break;
        c.in.append(buf, n);
        busy = true;
      }

      const State before = c.st;
      if (c.st == REQ) {
        size_t nl = c.in.find('\n');
        if (nl == std::string::npos) {
          if (c.clientEof || c.in.size() > 4096) c.st = DONE;
        } else {
          std::istringstream req(c.in.substr(0, nl));
          std::string verb;
          req >> verb;
          if (verb == "HELLO") {
            c.in.erase(0, nl + 1);
            c.out += "ECCE-SSH-MASTER 1\n";
            c.st = CONTROL;
          } else if (verb == "SIGNAL") {
            int id = 0;
            std::string name;
            req >> id >> name;
            for (std::list<Client>::iterator o = clients.begin(); o != clients.end(); ++o)
              if (o->id == id && o->ch) ssh_channel_request_send_signal(o->ch, name.c_str());
            c.out += "OK\n";
            c.st = DONE;
            c.finished = true;
          } else if (verb == "RUN" || verb == "STREAM") {
            size_t cmdLen = 0, inLen = 0;
            req >> cmdLen;
            if (verb == "RUN") req >> inLen;
            if (c.in.size() >= nl + 1 + cmdLen) {
              std::string cmd = c.in.substr(nl + 1, cmdLen);
              c.in.erase(0, nl + 1 + cmdLen);
              std::string err;
              if (sessionLost) {
                c.out += "ERR the connection to " + t.host() + " has ended\n";
                c.st = DONE;
                c.finished = true;
              } else if (!(c.ch = openExec(s, t.execLineForMaster(cmd), err))) {
                c.out += "ERR cannot start command: " + err + "\n";
                c.st = DONE;
                c.finished = true;
              } else {
                c.id = nextId++;
                c.out += "OK " + std::to_string(c.id) + "\n";
                c.st = verb == "RUN" ? RUN : STREAM;
                c.stdinLeft = inLen;
              }
            } else if (c.clientEof) {
              c.st = DONE;
            }
          } else {
            c.st = DONE;
          }
        }
        if (c.st != before) busy = true;
      }

      if ((c.st == RUN || c.st == STREAM) && c.ch) {
        bool fail = false;
        // To the channel.
        size_t want = c.st == RUN ? std::min(c.stdinLeft, c.in.size()) : c.in.size();
        while (want > 0) {
          int n = ssh_channel_write(c.ch, c.in.data(), (uint32_t)std::min<size_t>(want, 16384));
          if (n == SSH_ERROR) { fail = true; break; }
          if (n <= 0) break;
          c.in.erase(0, n);
          want -= n;
          if (c.st == RUN) c.stdinLeft -= n;
          busy = true;
        }
        const bool inputDone = c.st == RUN ? c.stdinLeft == 0
                                           : (c.clientEof && c.in.empty());
        if (!fail && inputDone && !c.eofSent) {
          int rc = ssh_channel_send_eof(c.ch);
          if (rc == SSH_OK) {
            c.eofSent = true;
            if (c.st == STREAM) c.graceUntil = Clock::now() + std::chrono::seconds(2);
          } else if (rc != SSH_AGAIN) fail = true;
        }
        // From the channel.
        for (int isErr = 0; !fail && isErr < 2 && c.out.size() < kMaxQueued; isErr++) {
          bool& eof = isErr ? c.errEof : c.outEof;
          while (!eof && c.out.size() < kMaxQueued) {
            int n = ssh_channel_read_nonblocking(c.ch, buf, sizeof buf, isErr);
            if (n == SSH_EOF) { eof = true; break; }
            if (n == SSH_ERROR) { fail = true; break; }
            if (n <= 0) break;
            if (c.st == RUN) frame(c.out, isErr ? 'E' : 'O', std::string(buf, n));
            else c.out.append(buf, n);
            busy = true;
          }
        }
        if (!fail && !c.finished &&
            ((c.outEof && c.errEof) || ssh_channel_is_closed(c.ch) ||
             (ssh_channel_is_eof(c.ch) && ssh_channel_poll(c.ch, 0) <= 0 &&
              ssh_channel_poll(c.ch, 1) <= 0))) {
          c.finished = true;
          c.finishedAt = Clock::now();
        }
        if (fail || sessionLost) {
          if (c.st == RUN) frame(c.out, 'F', "connection error: " +
                                 std::string(ssh_get_error(s)));
          closeChannel(c);
          c.st = DONE;
          c.finished = true;
        } else if (c.finished && c.st == RUN) {
          // The exit status arrives just before the channel closes.
          std::string st;
          if (exitStatus(c.ch, st) || ssh_channel_is_closed(c.ch) ||
              Clock::now() - c.finishedAt > std::chrono::seconds(5)) {
            frame(c.out, 'X', st.empty() ? "-1" : st);
            closeChannel(c);
            c.st = DONE;
          }
        } else if (c.finished && c.st == STREAM) {
          closeChannel(c);
          c.st = DONE;
        } else if (c.st == RUN && c.clientEof) {
          // The client gave up (its timeout): the command goes with it.
          closeChannel(c);
          c.st = DONE;
          c.out.clear();
        } else if (c.st == STREAM && c.eofSent && c.clientEof &&
                   Clock::now() >= c.graceUntil) {
          closeChannel(c);
          c.st = DONE;
          c.out.clear();
        }
      }

      // To the client.
      while (!c.out.empty()) {
        int w = clientWrite(c, c.out.data(), c.out.size());
        if (w < 0) {
          c.out.clear();
          closeChannel(c);
          c.st = DONE;
          c.clientEof = true;
          break;
        }
        if (w == 0) break;
        c.out.erase(0, w);
        busy = true;
      }

      const bool gone = (c.st == DONE && c.out.empty()) ||
                        (c.st == CONTROL && c.clientEof) ||
                        (c.st == REQ && c.clientEof && c.in.empty());
      if (gone) {
        closeChannel(c);
        clientClose(c);
        it = clients.erase(it);
        busy = true;
      } else {
        ++it;
      }
    }

    Clock::time_point now = Clock::now();
    if (!clients.empty()) lastBusy = now;
    if (sessionLost && clients.empty()) {
      fprintf(stderr, "eccesshmaster: the connection to %s ended\n", t.host().c_str());
      break;
    }
    if (clients.empty() && now - lastBusy >= std::chrono::seconds(idleSec)) break;
    if (!sessionLost && now - lastPing >= std::chrono::seconds(30)) {
      ssh_send_keepalive(s);
      lastPing = now;
    }

#ifdef _WIN32
    Sleep(busy ? 1 : 10);
#else
    if (!busy) {
      std::vector<struct pollfd> p;
      p.push_back({ ls, POLLIN, 0 });
      if (!sessionLost) p.push_back({ ssh_get_fd(s), POLLIN, 0 });
      for (std::list<Client>::iterator it = clients.begin(); it != clients.end(); ++it)
        if (!it->clientEof)
          p.push_back({ it->fd, (short)(POLLIN | (it->out.empty() ? 0 : POLLOUT)), 0 });
      poll(p.data(), p.size(), 20);
    }
#endif
  }

  for (std::list<Client>::iterator it = clients.begin(); it != clients.end(); ++it) {
    closeChannel(*it);
    clientClose(*it);
  }
#ifdef _WIN32
  if (listening != INVALID_HANDLE_VALUE) CloseHandle(listening);
  LocalFree(psd);
#else
  close(ls);
  unlink(ep.c_str());
#endif
  return sessionLost ? 3 : 0;
}

}  // namespace SshMaster
