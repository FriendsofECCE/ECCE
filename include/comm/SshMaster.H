#ifndef SSHMASTER_HH
#define SSHMASTER_HH

#include <string>

class SshTransport;

// ECCE's connection master for the built-in ssh client (#1284).
//
// eccesshmaster holds one authenticated libssh session and opens exec
// channels on it for local ECCE programs, so a single-connection or
// front-end machine costs one login (one two-factor code) however many
// processes use it, without fork(), which Windows lacks.  The endpoint is
// reachable only by the same user: a socket in a 0700 directory on POSIX, a
// named pipe whose DACL names only the user's SID on Windows.  Only channel
// traffic crosses it, never a password.
//
// Requests (one line, then bytes):
//   HELLO                      -> "ECCE-SSH-MASTER 1"; held open as a client
//   RUN <cmdlen> <stdinlen>    + cmd + stdin -> "OK <id>" | "ERR <msg>", then
//                              frames 'O'/'E' (output), 'X' (exit status
//                              text), 'F' (failure text): type, 4-byte BE length
//   STREAM <cmdlen>            + cmd -> "OK <id>" | "ERR <msg>", then raw both
//                              ways (stdout and stderr merged)
//   SIGNAL <id> <NAME>         -> "OK"
namespace SshMaster {

// The endpoint for one route (user, host, port, front end) of this user's
// state directory.  "" when no private place for it can be had.
std::string endpoint(const std::string& host, int port,
                     const std::string& user, const std::string& jump);

// ECCE_SSH_MASTER: "off" never uses a master, "serve" is the master itself.
std::string mode();

// True when a master answers HELLO at ep.
bool live(const std::string& ep);

// Seconds a master stays up with no client (ECCE_SSH_MASTER_IDLE, 300).
int idleSeconds();

// The file eccesshmaster reports its outcome in: a line "started", then
// "ok" or "cancelled|refused|unreachable|failed" and a message.
std::string statusPath(const std::string& ep);
void writeStatus(const std::string& path, const std::string& text);
std::string readStatus(const std::string& path);

// One connection to a master.
class Conn {
public:
  Conn();
  ~Conn();
  bool open(const std::string& ep, std::string& error);
  bool isOpen() const;
  bool writeAll(const std::string& s);
  // Up to n bytes within timeoutMs (< 0: no limit): > 0 bytes, 0 nothing
  // yet, -1 end of stream or error.
  int readSome(char* buf, size_t n, int timeoutMs);
  // One line without its newline, read a byte at a time so nothing after it
  // is taken from a stream that follows.
  bool readLine(std::string& line, int timeoutMs);
  // The descriptor for a stream reader; the Conn no longer owns it.  On
  // Windows a CRT descriptor on the pipe (PeekNamedPipe works on it).
  int releaseFd();
  void close();
private:
  Conn(const Conn&);
  Conn& operator=(const Conn&);
#ifdef _WIN32
  void* p_h;
#else
  int p_fd;
#endif
};

// Takes the route's lock (one login per route): false when another
// eccesshmaster holds it.  Held until the process ends.
bool lockRoute(const std::string& ep);

// Serves channels on t's session at ep until it has had no client for
// idleSec or the session ends.  Writes "ok" to statusFile once listening.
int serve(SshTransport& t, const std::string& ep, int idleSec,
          const std::string& statusFile);

}  // namespace SshMaster

#endif
