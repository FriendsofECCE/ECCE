// See include/util/MqttBroker.H.  Packet layouts: MQTT 5.0 section 3.
#include "util/MqttBroker.H"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>
#include <vector>

#include "util/Ecce.H"
#include "util/MqttLink.H"

#ifdef _WIN32
#include <winsock2.h>
#include <direct.h>
#include <ws2tcpip.h>
typedef SOCKET sock_t;
static const sock_t BAD_SOCK = INVALID_SOCKET;
static void closeSock(sock_t s) { closesocket(s); }
static void setNonBlocking(sock_t s) { u_long on = 1; ioctlsocket(s, FIONBIO, &on); }
static bool wouldBlock() { int e = WSAGetLastError(); return e == WSAEWOULDBLOCK || e == WSAEINTR; }
#define SEND_FLAGS 0
#define POLL WSAPoll
// Not inheritable: a program started with inherited handles would keep
// the broker's sockets open after the Organizer exits.
static sock_t keepPrivate(sock_t s)
{
  if (s != BAD_SOCK) SetHandleInformation((HANDLE)s, HANDLE_FLAG_INHERIT, 0);
  return s;
}
static sock_t newSocket(int domain) { return keepPrivate(socket(domain, SOCK_STREAM, 0)); }
static sock_t acceptSocket(sock_t l, sockaddr* a, socklen_t* n) { return keepPrivate(accept(l, a, n)); }
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
typedef int sock_t;
static const sock_t BAD_SOCK = -1;
static void closeSock(sock_t s) { ::close(s); }
static void setNonBlocking(sock_t s) { fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK); }
static bool wouldBlock() { return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR; }
#ifdef MSG_NOSIGNAL
#define SEND_FLAGS MSG_NOSIGNAL
#else
#define SEND_FLAGS 0
#endif
#define POLL ::poll
// Close-on-exec: system() and execv() pass every other descriptor on, and
// eccejobmaster or a terminal holding the broker's end keeps its clients
// from seeing EOF when the Organizer exits (#1025).
static sock_t keepPrivate(sock_t s)
{
  if (s != BAD_SOCK) fcntl(s, F_SETFD, fcntl(s, F_GETFD) | FD_CLOEXEC);
  return s;
}
static sock_t newSocket(int domain)
{
#ifdef SOCK_CLOEXEC
  return socket(domain, SOCK_STREAM | SOCK_CLOEXEC, 0);
#else
  return keepPrivate(socket(domain, SOCK_STREAM, 0));   // macOS
#endif
}
static sock_t acceptSocket(sock_t l, sockaddr* a, socklen_t* n)
{
#if defined(__linux__) && defined(SOCK_CLOEXEC)
  return accept4(l, a, n, SOCK_CLOEXEC);
#else
  return keepPrivate(accept(l, a, n));
#endif
}
#endif

using std::string;

namespace {

const size_t MAX_PACKET = 64u << 20;
const size_t MAX_OUTBUF = 64u << 20;
const int CONNECT_WAIT_S = 10;

struct ProtocolError : std::runtime_error {
  explicit ProtocolError(const char* w) : std::runtime_error(w) {}
};

// A bounds-checked view of one packet body.
struct Reader {
  const uint8_t* p; size_t n;
  Reader(const uint8_t* b, size_t len) : p(b), n(len) {}
  uint8_t u8() { if (!n) throw ProtocolError("short"); n--; return *p++; }
  uint16_t u16() { uint16_t h = u8(); return (uint16_t)((h << 8) | u8()); }
  uint32_t varint() {
    uint32_t v = 0; int sh = 0;
    for (int i = 0; i < 4; i++) {
      uint8_t b = u8(); v |= (uint32_t)(b & 0x7f) << sh;
      if (!(b & 0x80)) return v;
      sh += 7;
    }
    throw ProtocolError("varint");
  }
  string bytes(size_t k) {
    if (k > n) throw ProtocolError("short");
    string s((const char*)p, k); p += k; n -= k; return s;
  }
  string str() { return bytes(u16()); }
  void skip(size_t k) { bytes(k); }
};

void putU16(string& s, unsigned v) { s += (char)(v >> 8); s += (char)(v & 0xff); }
void putVarint(string& s, size_t v) {
  do { uint8_t b = v & 0x7f; v >>= 7; if (v) b |= 0x80; s += (char)b; } while (v);
}
string packet(uint8_t first, const string& body) {
  string s(1, (char)first);
  putVarint(s, body.size());
  return s + body;
}

bool validName(const string& u) {
  return !u.empty() && u.find_first_of("/+#") == string::npos;
}

std::vector<string> levels(const string& t) {
  std::vector<string> v; size_t a = 0;
  for (;;) {
    size_t b = t.find('/', a);
    if (b == string::npos) { v.push_back(t.substr(a)); return v; }
    v.push_back(t.substr(a, b - a)); a = b + 1;
  }
}

bool validFilter(const string& f) {
  if (f.empty() || f.compare(0, 6, "$share") == 0) return false;
  std::vector<string> l = levels(f);
  for (size_t i = 0; i < l.size(); i++) {
    if (l[i].find('+') != string::npos && l[i] != "+") return false;
    if (l[i].find('#') != string::npos && (l[i] != "#" || i + 1 != l.size())) return false;
  }
  return true;
}

bool constEq(const string& a, const string& b) {
  unsigned d = (unsigned)(a.size() ^ b.size());
  for (size_t i = 0; i < a.size() && i < b.size(); i++) d |= (uint8_t)(a[i] ^ b[i]);
  return d == 0;
}

struct Client {
  sock_t fd = BAD_SOCK;
  string in, out, user, id;
  bool connected = false, dead = false;
  int keepalive = 0;
  std::chrono::steady_clock::time_point last, born;
  std::map<string, uint8_t> subs;     // filter -> options
  uint16_t nextId = 0;
};

} // namespace

struct MqttBroker::Impl {
  std::map<string, string> users;
  // A Unix-socket listener takes anyone and applies no topic rules, like the
  // per-user mosquitto's socket: the account's 0700 directory keeps others out.
  bool open = false;
  sock_t lfd = BAD_SOCK;
  std::thread thread;
  std::atomic<bool> stopping{false};
  std::atomic<int> nclients{0};
  std::vector<std::unique_ptr<Client>> clients;
  uint64_t anon = 0;

  void queue(Client& c, const string& pkt) {
    if (c.dead) return;
    c.out += pkt;
    if (c.out.size() > MAX_OUTBUF) c.dead = true;
  }

  void flush(Client& c) {
    while (!c.out.empty()) {
      int k = (int)send(c.fd, c.out.data(), (int)std::min<size_t>(c.out.size(), 1 << 16), SEND_FLAGS);
      if (k > 0) c.out.erase(0, k);
      else if (k < 0 && wouldBlock()) return;
      else { c.dead = true; return; }
    }
  }

  void connack(Client& c, int rc) {
    string b; b += (char)0; b += (char)rc;
    // Maximum QoS 1, retain unavailable: what this broker does.
    string props; props += (char)0x24; props += (char)1; props += (char)0x25; props += (char)0;
    if (rc == 0) putVarint(b, props.size()), b += props; else b += (char)0;
    queue(c, packet(0x20, b));
  }

  void onConnect(Client& c, Reader r) {
    if (c.connected) throw ProtocolError("second CONNECT");
    string name = r.str();
    uint8_t level = r.u8();
    if (name != "MQTT") throw ProtocolError("protocol");
    if (level != 5) {   // 3.1.1 clients get the 3.1.1 refusal and are closed
      string b; b += (char)0; b += (char)1;
      queue(c, packet(0x20, b)); c.dead = true; return;
    }
    uint8_t flags = r.u8();
    c.keepalive = r.u16();
    r.skip(r.varint());
    c.id = r.str();
    if (flags & 0x04) {   // will: not used by ECCE, read past it
      r.skip(r.varint()); r.str(); r.skip(r.u16());
    }
    string user, pw;
    bool hasUser = flags & 0x80, hasPw = flags & 0x40;
    if (hasUser) user = r.str();
    if (hasPw) pw = r.str();
    std::map<string, string>::iterator u = users.find(user);
    if (!open && (!hasUser || !validName(user) || u == users.end() ||
                  !constEq(u->second, pw))) {
      connack(c, hasUser ? 0x86 : 0x87); c.dead = true; return;
    }
    if (c.id.empty()) c.id = "ecce-anon-" + std::to_string(++anon);
    for (auto& o : clients)   // a repeated client id takes over
      if (o.get() != &c && o->connected && o->id == c.id) o->dead = true;
    c.user = user; c.connected = true;
    connack(c, 0);
  }

  bool readOk(const Client& s, const string& topic) {
    return open || mayRead(s.user, topic);
  }

  void onPublish(Client& c, uint8_t first, Reader r) {
    int qos = (first >> 1) & 3;
    if (qos == 2) { disconnectWith(c, 0x9b); return; }
    string topic = r.str();
    uint16_t pid = qos ? r.u16() : 0;
    uint32_t plen = r.varint();
    string props = r.bytes(plen);
    string payload = r.bytes(r.n);
    if (topic.empty() || topic.find_first_of("+#") != string::npos)
      throw ProtocolError("topic");
    if (!open && !mayWrite(c.user, topic)) {
      if (qos) { string b; putU16(b, pid); b += (char)0x87; queue(c, packet(0x40, b)); }
      return;
    }
    for (auto& sp : clients) {
      Client& s = *sp;
      if (!s.connected || s.dead || !readOk(s, topic)) continue;
      int best = -1;
      for (auto& f : s.subs) {
        if (!topicMatches(f.first, topic)) continue;
        if ((f.second & 0x04) && &s == &c) continue;   // no local
        best = std::max(best, (int)(f.second & 3));
      }
      if (best < 0) continue;
      int dq = std::min(best, qos);
      string b;
      putU16(b, (unsigned)topic.size()); b += topic;
      if (dq) { if (++s.nextId == 0) s.nextId = 1; putU16(b, s.nextId); }
      putVarint(b, props.size()); b += props;
      b += payload;
      queue(s, packet((uint8_t)(0x30 | (dq << 1)), b));
    }
    if (qos) { string b; putU16(b, pid); queue(c, packet(0x40, b)); }
  }

  void onSubscribe(Client& c, Reader r) {
    uint16_t pid = r.u16();
    r.skip(r.varint());
    string codes;
    if (!r.n) throw ProtocolError("empty SUBSCRIBE");
    while (r.n) {
      string f = r.str(); uint8_t o = r.u8();
      if (!validFilter(f) || (o & 3) == 3) { codes += (char)0x8f; continue; }
      int q = std::min(o & 3, 1);
      c.subs[f] = (uint8_t)((o & ~3) | q);   // QoS 2 is granted as 1
      codes += (char)q;
    }
    string b; putU16(b, pid); b += (char)0; b += codes;
    queue(c, packet(0x90, b));
  }

  void onUnsubscribe(Client& c, Reader r) {
    uint16_t pid = r.u16();
    r.skip(r.varint());
    string codes;
    if (!r.n) throw ProtocolError("empty UNSUBSCRIBE");
    while (r.n) codes += (char)(c.subs.erase(r.str()) ? 0x00 : 0x11);
    string b; putU16(b, pid); b += (char)0; b += codes;
    queue(c, packet(0xb0, b));
  }

  void disconnectWith(Client& c, int rc) {
    string b; b += (char)rc; b += (char)0;
    queue(c, packet(0xe0, b)); c.dead = true;
  }

  // Returns false when the buffer holds no whole packet.
  bool step(Client& c) {
    if (c.in.size() < 2) return false;
    size_t len = 0, mul = 1, i = 1;
    for (;; i++) {
      if (i >= c.in.size()) return false;
      if (i > 4) throw ProtocolError("length");
      uint8_t b = (uint8_t)c.in[i];
      len += (b & 0x7f) * mul; mul <<= 7;
      if (!(b & 0x80)) { i++; break; }
    }
    if (len > MAX_PACKET) throw ProtocolError("too large");
    if (c.in.size() < i + len) return false;
    uint8_t first = (uint8_t)c.in[0];
    string body = c.in.substr(i, len);
    c.in.erase(0, i + len);
    c.last = std::chrono::steady_clock::now();
    Reader r((const uint8_t*)body.data(), body.size());
    int type = first >> 4;
    if (!c.connected && type != 1) throw ProtocolError("not connected");
    switch (type) {
    case 1: onConnect(c, r); break;
    case 3: onPublish(c, first, r); break;
    case 4: break;   // PUBACK for what was sent: nothing is kept to resend
    case 8: onSubscribe(c, r); break;
    case 10: onUnsubscribe(c, r); break;
    case 12: queue(c, packet(0xd0, "")); break;
    case 14: c.dead = true; break;
    default: throw ProtocolError("packet type");
    }
    return true;
  }

  void acceptOne() {
    sockaddr_storage a; socklen_t l = sizeof a;
    sock_t s = acceptSocket(lfd, (sockaddr*)&a, &l);
    if (s == BAD_SOCK) return;
    setNonBlocking(s);
    int one = 1;
    if (!open) setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char*)&one, sizeof one);
#ifdef SO_NOSIGPIPE
    // macOS has no MSG_NOSIGNAL: a client gone mid-send must not kill us.
    setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, (const char*)&one, sizeof one);
#endif
    std::unique_ptr<Client> c(new Client);
    c->fd = s;
    c->last = c->born = std::chrono::steady_clock::now();
    clients.push_back(std::move(c));
  }

  void run() {
    while (!stopping) {
      std::vector<pollfd> fds;
      pollfd lp; lp.fd = lfd; lp.events = POLLIN; lp.revents = 0;
      fds.push_back(lp);
      for (auto& c : clients) {
        pollfd p; p.fd = c->fd; p.events = POLLIN | (c->out.empty() ? 0 : POLLOUT);
        p.revents = 0; fds.push_back(p);
      }
      if (POLL(fds.data(), (unsigned)fds.size(), 100) < 0) {
        if (wouldBlock()) continue;
        break;
      }
      size_t n = clients.size();
      for (size_t k = 0; k < n; k++) {
        Client& c = *clients[k];
        short ev = fds[k + 1].revents;
        if (ev & (POLLERR | POLLNVAL)) c.dead = true;
        if ((ev & (POLLIN | POLLHUP)) && !c.dead) {
          char buf[1 << 16];
          int got = (int)recv(c.fd, buf, sizeof buf, 0);
          if (got > 0) {
            c.in.append(buf, got);
            try { while (!c.dead && step(c)) {} }
            catch (const std::exception&) { c.dead = true; }
          } else if (got == 0 || !wouldBlock()) c.dead = true;
        }
      }
      if (fds[0].revents & POLLIN) acceptOne();
      auto now = std::chrono::steady_clock::now();
      for (auto& cp : clients) {
        Client& c = *cp;
        flush(c);
        double idle = std::chrono::duration<double>(now - c.last).count();
        if (c.connected && c.keepalive && idle > c.keepalive * 1.5) c.dead = true;
        if (!c.connected && idle > CONNECT_WAIT_S) c.dead = true;
      }
      int live = 0;
      for (auto& cp : clients) if (cp->connected && !cp->dead) live++;
      nclients = live;
      for (size_t k = 0; k < clients.size();) {
        if (clients[k]->dead) {
          flush(*clients[k]);   // a CONNACK refusal still goes out
          closeSock(clients[k]->fd);
          clients.erase(clients.begin() + k);
        } else k++;
      }
    }
    for (auto& c : clients) closeSock(c->fd);
    clients.clear();
  }
};

// ---------------------------------------------------------------- rules

bool MqttBroker::mayWrite(const string& user, const string& topic)
{
  if (!validName(user)) return false;
  string base = "ecce/" + user;
  return topic == base || topic.compare(0, base.size() + 1, base + "/") == 0;
}

bool MqttBroker::mayRead(const string& user, const string& topic)
{
  if (mayWrite(user, topic)) return true;
  std::vector<string> l = levels(topic);
  return l.size() == 3 && l[0] == "ecce" && l[2] == "ecce_machreg_changed";
}

bool MqttBroker::topicMatches(const string& filter, const string& topic)
{
  std::vector<string> f = levels(filter), t = levels(topic);
  if (!t[0].empty() && t[0][0] == '$' && (f[0] == "#" || f[0] == "+")) return false;
  for (size_t i = 0; i < f.size(); i++) {
    if (f[i] == "#") return true;
    if (i >= t.size()) return false;
    if (f[i] != "+" && f[i] != t[i]) return false;
  }
  return f.size() == t.size();
}

// ---------------------------------------------------------------- public

MqttBroker::MqttBroker() : p_impl(new Impl) {}
MqttBroker::~MqttBroker() { stop(); }

int MqttBroker::clients() const { return p_impl->nclients; }

void MqttBroker::addUser(const string& user, const string& password)
{
  p_impl->users[user] = password;
}

bool MqttBroker::start(int port)
{
  if (p_impl->thread.joinable()) return false;
#ifdef _WIN32
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
#endif
  sock_t s = newSocket(AF_INET);
  if (s == BAD_SOCK) return false;
  sockaddr_in a;
  memset(&a, 0, sizeof a);
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  a.sin_port = htons((unsigned short)port);
  int one = 1;
#ifdef _WIN32
  // SO_REUSEADDR on Windows lets another program take over the port.
  setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&one, sizeof one);
#else
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&one, sizeof one);
#endif
  socklen_t len = sizeof a;
  if (::bind(s, (sockaddr*)&a, sizeof a) != 0 || listen(s, 64) != 0 ||
      getsockname(s, (sockaddr*)&a, &len) != 0) {
    closeSock(s);
    return false;
  }
  setNonBlocking(s);
  p_port = ntohs(a.sin_port);
  p_impl->lfd = s;
  p_impl->stopping = false;
  p_impl->thread = std::thread([this] { p_impl->run(); });
  return true;
}

bool MqttBroker::start(const string& path)
{
#ifdef _WIN32
  (void)path;
  return false;
#else
  if (p_impl->thread.joinable()) return false;
  sockaddr_un a;
  memset(&a, 0, sizeof a);
  if (path.empty() || path.size() >= sizeof a.sun_path) return false;
  a.sun_family = AF_UNIX;
  memcpy(a.sun_path, path.c_str(), path.size());
  sock_t s = newSocket(AF_UNIX);
  if (s == BAD_SOCK) return false;
  ::unlink(path.c_str());
  if (::bind(s, (sockaddr*)&a, sizeof a) != 0) { closeSock(s); return false; }
  // The directory is what keeps other accounts out; 0600 says the same.
  chmod(path.c_str(), 0600);
  if (listen(s, 64) != 0) { closeSock(s); ::unlink(path.c_str()); return false; }
  setNonBlocking(s);
  p_path = path;
  p_impl->open = true;
  p_impl->lfd = s;
  p_impl->stopping = false;
  p_impl->thread = std::thread([this] { p_impl->run(); });
  return true;
#endif
}

void MqttBroker::stop()
{
  if (!p_impl->thread.joinable()) return;
  p_impl->stopping = true;
  p_impl->thread.join();
  closeSock(p_impl->lfd);
  p_impl->lfd = BAD_SOCK;
#ifndef _WIN32
  if (!p_path.empty()) ::unlink(p_path.c_str());
#endif
}

// ---------------------------------------------------------- the session

namespace {
std::unique_ptr<MqttBroker> g_session;
string g_brokerFile;

string randomHex(int bytes)
{
  std::random_device rd;
  static const char hex[] = "0123456789abcdef";
  string s;
  for (int i = 0; i < bytes; i++) {
    unsigned v = rd() & 0xff;
    s += hex[v >> 4]; s += hex[v & 15];
  }
  return s;
}

// Atomic, so a process starting now never reads half of it.
bool writeAtomic(const string& path, const string& text)
{
  string tmp = path + ".tmp." + randomHex(4);
  { std::ofstream f(tmp.c_str(), std::ios::binary); f << text; if (!f) return false; }
  std::remove(path.c_str());   // Windows rename does not replace
  return std::rename(tmp.c_str(), path.c_str()) == 0;
}
} // namespace

#ifndef _WIN32
namespace {
string g_sockDir;   // a directory made for the socket, removed with it
std::atomic<bool> g_stopRequested{false};

string fileValue(const string& file, const string& key)
{
  std::ifstream f(file.c_str());
  string line;
  while (std::getline(f, line))
    if (line.compare(0, key.size() + 1, key + "=") == 0) return line.substr(key.size() + 1);
  return "";
}

bool socketAnswers(const string& path)
{
  sockaddr_un a;
  memset(&a, 0, sizeof a);
  if (path.empty() || path.size() >= sizeof a.sun_path) return false;
  a.sun_family = AF_UNIX;
  memcpy(a.sun_path, path.c_str(), path.size());
  int s = newSocket(AF_UNIX);
  if (s < 0) return false;
  bool ok = ::connect(s, (sockaddr*)&a, sizeof a) == 0;
  ::close(s);
  return ok;
}

// In the private state directory, as the per-user mosquitto's was; a deep
// one does not fit sun_path (104 bytes on macOS) and gets a private
// directory of its own under the runtime directory instead.
string sessionSocketPath(const string& dir)
{
  string name = "mqtt-" + randomHex(4) + ".sock";
  if (dir.size() + 1 + name.size() < 100) return dir + "/" + name;
  const char* rt = getenv("XDG_RUNTIME_DIR");
  string d = string(rt && *rt ? rt : "/tmp") + "/ecce-" +
             std::to_string((long)getuid()) + "-" + randomHex(4);
  if (mkdir(d.c_str(), 0700) != 0) return "";
  g_sockDir = d;
  return d + "/" + name;
}
} // namespace
#endif

string MqttBroker::startForSession()
{
  if (g_session) return "";
  string id = Ecce::sessionId();
  string pointer = Ecce::sessionPointerFile();
  const char* home = getenv("ECCE_REALUSERHOME");
  if (id.empty() || pointer.empty() || !home)
    return "no valid ECCE_SESSION_ID or no ECCE_REALUSERHOME";
  string file = string(home) + "/.ECCE/broker_" + Ecce::sessionKeyFor(id);
  // Plain mkdir: std::filesystem needs macOS 10.15, and the 10.13 build uses this file.
  string dir = string(home) + "/.ECCE";
#ifdef _WIN32
  _mkdir(dir.c_str());
#else
  mkdir(dir.c_str(), 0700);
#endif

  std::unique_ptr<MqttBroker> b(new MqttBroker);
#ifdef _WIN32
  string user = "s" + randomHex(4), pw = randomHex(16);
  b->addUser(user, pw);
  if (!b->start(0)) return "cannot listen on 127.0.0.1";
  string text = "user=" + user + "\npassword=" + pw +
                "\nhost=127.0.0.1\nport=" + std::to_string(b->port()) + "\n";
#else
  // Linux, macOS: a Unix socket in the 0700 state directory, no login (the
  // socket is private to the account), as the per-user mosquitto was.
  chmod(dir.c_str(), 0700);
  string old = fileValue(file, "socket");
  if (socketAnswers(old)) return "";   // another program of the session hosts it
  struct stat st;
  if (!old.empty() && lstat(old.c_str(), &st) == 0 && S_ISSOCK(st.st_mode))
    ::unlink(old.c_str());
  string sock = sessionSocketPath(dir);
  if (sock.empty() || !b->start(sock)) {
    if (!g_sockDir.empty()) { rmdir(g_sockDir.c_str()); g_sockDir.clear(); }
    return "cannot listen on a Unix socket in " + dir;
  }
  // "local" as the topic user, as the mosquitto route writes it.
  string text = "user=local\nsocket=" + sock + "\n";
#endif
  if (!writeAtomic(file, text)) return "cannot write " + file;
  writeAtomic(pointer, id + "\n");
  g_brokerFile = file;
  g_session = std::move(b);
  return "";
}

bool MqttBroker::hostsSession() { return g_session != nullptr; }

void MqttBroker::stopForSession()
{
  if (!g_session) return;
#ifndef _WIN32
  // Another program may have taken the session over (ecce-gateway-stop
  // removed the file, a new start wrote its own): leave its file alone.
  bool ours = fileValue(g_brokerFile, "socket") == g_session->socketPath();
#else
  bool ours = true;
#endif
  g_session->stop();
  g_session.reset();
  if (ours) std::remove(g_brokerFile.c_str());
#ifndef _WIN32
  if (!g_sockDir.empty()) { rmdir(g_sockDir.c_str()); g_sockDir.clear(); }
#endif
}

void MqttBroker::requestStop()
{
#ifndef _WIN32
  g_stopRequested = true;
#endif
}

void MqttBroker::serveForSession(int graceSeconds, int startSeconds)
{
#ifndef _WIN32
  if (!g_session) return;
  using clk = std::chrono::steady_clock;
  auto idleSince = clk::now();
  bool seen = false;
  while (!g_stopRequested &&
         fileValue(g_brokerFile, "socket") == g_session->socketPath()) {
    if (g_session->clients() > 0) { seen = true; idleSince = clk::now(); }
    else if (std::chrono::duration<double>(clk::now() - idleSince).count() >=
             (seen ? graceSeconds : std::max(graceSeconds, startSeconds))) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  stopForSession();
#else
  (void)graceSeconds; (void)startSeconds;
#endif
}

void MqttBroker::lingerForSession(int graceSeconds)
{
  if (!g_session) return;
  using clk = std::chrono::steady_clock;
  auto sleepMs = [](int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); };
  int before = g_session->clients();
  bool own = MqttLink::instance().connected();
  MqttLink::instance().shutdown();
  // Wait until the broker thread has seen our own link leave.
  for (int i = 0; own && i < 20 && g_session->clients() >= before; i++) sleepMs(100);
  if (g_session->clients() == 0) { stopForSession(); return; }
  auto idleSince = clk::now();
  for (;;) {
    if (g_session->clients() > 0) idleSince = clk::now();
    else if (std::chrono::duration<double>(clk::now() - idleSince).count() >= graceSeconds) break;
    sleepMs(100);
  }
  stopForSession();
}
