#include "comm/SshTransport.H"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libssh/libssh.h>
#include <libssh/sftp.h>

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

SshTransport::SshTransport(const std::string& host, int port,
                           const std::string& user)
  : p_host(host), p_user(user), p_port(port), p_useConfig(true),
    p_connectTimeout(15), p_passwordAttempts(1), p_session(0), p_sftp(0)
{
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
}

bool SshTransport::connect(std::string& error)
{
  disconnect();
  ssh_session s = ssh_new();
  if (!s) { error = "ssh_new failed"; return false; }

  ssh_options_set(s, SSH_OPTIONS_HOST, p_host.c_str());
  if (p_port > 0) ssh_options_set(s, SSH_OPTIONS_PORT, &p_port);
  if (!p_user.empty()) ssh_options_set(s, SSH_OPTIONS_USER, p_user.c_str());
  ssh_options_set(s, SSH_OPTIONS_TIMEOUT, &p_connectTimeout);
  if (p_useConfig) {
    const char* cf = p_configFile.empty() ? 0 : p_configFile.c_str();
    if (ssh_options_parse_config(s, cf) != 0) {
      error = "cannot read ssh configuration";
      ssh_free(s);
      return false;
    }
  } else {
    int off = 0;
    ssh_options_set(s, SSH_OPTIONS_PROCESS_CONFIG, &off);
  }
  if (!p_knownHosts.empty())
    ssh_options_set(s, SSH_OPTIONS_KNOWNHOSTS, p_knownHosts.c_str());
  else if (const char* home = getenv("HOME"))
    mkdir((std::string(home) + "/.ssh").c_str(), 0700);
  for (size_t i = 0; i < p_identities.size(); i++)
    ssh_options_set(s, SSH_OPTIONS_ADD_IDENTITY, p_identities[i].c_str());

  if (ssh_connect(s) != SSH_OK) {
    error = std::string("cannot connect to ") + p_host + ": " + ssh_get_error(s);
    ssh_free(s);
    return false;
  }
  p_session = s;
  if (!checkHostKey(error) || !authenticate(error)) {
    disconnect();
    return false;
  }
  return true;
}

bool SshTransport::checkHostKey(std::string& error)
{
  enum ssh_known_hosts_e st = ssh_session_is_known_server(p_session);
  if (st == SSH_KNOWN_HOSTS_OK) return true;

  if (st == SSH_KNOWN_HOSTS_CHANGED || st == SSH_KNOWN_HOSTS_OTHER) {
    error = "the host key of " + p_host + " has CHANGED since it was recorded "
            "in known_hosts (or differs in type); refusing to connect. "
            "If the change is expected, remove the old entry and retry.";
    return false;
  }
  if (st == SSH_KNOWN_HOSTS_ERROR) {
    error = std::string("host key check failed: ") + ssh_get_error(p_session);
    return false;
  }

  ssh_key key = 0;
  if (ssh_get_server_publickey(p_session, &key) != SSH_OK) {
    error = "cannot read the server's host key";
    return false;
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

  if (!p_hostKey || !p_hostKey(p_host, fingerprint)) {
    error = "host key of " + p_host + " (" + fingerprint + ") was not accepted";
    return false;
  }
  if (ssh_session_update_known_hosts(p_session) != SSH_OK) {
    error = std::string("cannot record the host key: ") + ssh_get_error(p_session);
    return false;
  }
  return true;
}

// Sets rc to the libssh result; false means the user aborted.
bool SshTransport::authKeyboardInteractive(int& rc, bool& asked, std::string& error)
{
  rc = ssh_userauth_kbdint(p_session, 0, 0);
  while (rc == SSH_AUTH_INFO) {
    int n = ssh_userauth_kbdint_getnprompts(p_session);
    const char* ins = ssh_userauth_kbdint_getinstruction(p_session);
    std::string instr = ins ? ins : "";
    for (int i = 0; i < n; i++) {
      char echo = 0;
      const char* pr = ssh_userauth_kbdint_getprompt(p_session, i, &echo);
      std::string prompt = pr ? pr : "";
      if (i == 0 && !instr.empty()) prompt = instr + "\n" + prompt;
      std::string answer;
      asked = true;
      if (!p_prompt || !p_prompt(prompt, echo != 0, answer)) {
        error = "authentication cancelled";
        wipe(answer);
        return false;
      }
      ssh_userauth_kbdint_setanswer(p_session, i, answer.c_str());
      wipe(answer);
    }
    rc = ssh_userauth_kbdint(p_session, 0, 0);
  }
  return true;
}

bool SshTransport::authenticate(std::string& error)
{
  bool kbdintTried = false;
  int passwordsLeft = p_passwordAttempts;
  // A partial success (key, then a second factor) needs another round.
  for (int round = 0; round < 4; round++) {
    int rc = ssh_userauth_none(p_session, 0);
    if (rc == SSH_AUTH_SUCCESS) return true;
    int methods = ssh_userauth_list(p_session, 0);

    if (methods & SSH_AUTH_METHOD_PUBLICKEY) {
      rc = ssh_userauth_publickey_auto(p_session, 0, 0);
      if (rc == SSH_AUTH_SUCCESS) return true;
    }
    int partial = (rc == SSH_AUTH_PARTIAL);

    if (!partial && (methods & SSH_AUTH_METHOD_INTERACTIVE) && !kbdintTried) {
      kbdintTried = true;
      bool asked = false;
      if (!authKeyboardInteractive(rc, asked, error)) return false;
      if (rc == SSH_AUTH_SUCCESS) return true;
      partial = (rc == SSH_AUTH_PARTIAL);
      // A failed answered prompt is a failed login; asking the same
      // question again as a password would only repeat a wrong answer.
      if (!partial && asked) {
        error = "authentication failed for " + p_user + "@" + p_host;
        return false;
      }
    }
    if (partial) continue;

    if ((methods & SSH_AUTH_METHOD_PASSWORD) && p_prompt) {
      while (passwordsLeft-- > 0) {
        std::string pw;
        if (!p_prompt("Password for " + (p_user.empty() ? "" : p_user + "@") +
                      p_host + ": ", false, pw)) {
          wipe(pw);
          error = "authentication cancelled";
          return false;
        }
        rc = ssh_userauth_password(p_session, 0, pw.c_str());
        wipe(pw);
        if (rc == SSH_AUTH_SUCCESS) return true;
        if (rc == SSH_AUTH_PARTIAL) break;
      }
      if (rc == SSH_AUTH_PARTIAL) continue;
    }
    error = "authentication failed for " + p_user + "@" + p_host;
    return false;
  }
  error = "authentication failed for " + p_user + "@" + p_host;
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
  TransportResult res;
  if (!p_session) { res.error = "not connected"; return res; }
  std::string err;
  std::string envp = envPrefix(err);
  if (!err.empty()) { res.error = err; return res; }

  // fd 3 carries the script, stdin is /dev/null, as in DirectTransport.
  std::string full = "exec 3<&-\n" + envp + withDir(script);

  ssh_channel ch = ssh_channel_new(p_session);
  if (!ch) { res.error = "cannot create channel"; return res; }
  if (ssh_channel_open_session(ch) != SSH_OK ||
      ssh_channel_request_exec(ch, "sh -c 'exec 3<&0 </dev/null; . /dev/fd/3'") != SSH_OK) {
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
