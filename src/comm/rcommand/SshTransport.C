#include "comm/SshTransport.H"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <atomic>
#include <thread>

#include <dirent.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
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
  p_session = newSession(error);
  return p_session != 0;
}

ssh_session SshTransport::newSession(std::string& error)
{
  ssh_session s = ssh_new();
  if (!s) { error = "ssh_new failed"; return 0; }

  ssh_options_set(s, SSH_OPTIONS_HOST, p_host.c_str());
  if (p_port > 0) ssh_options_set(s, SSH_OPTIONS_PORT, &p_port);
  if (!p_user.empty()) ssh_options_set(s, SSH_OPTIONS_USER, p_user.c_str());
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
  if (!p_knownHosts.empty())
    ssh_options_set(s, SSH_OPTIONS_KNOWNHOSTS, p_knownHosts.c_str());
  else if (const char* home = getenv("HOME"))
    mkdir((std::string(home) + "/.ssh").c_str(), 0700);
  for (size_t i = 0; i < p_identities.size(); i++)
    ssh_options_set(s, SSH_OPTIONS_ADD_IDENTITY, p_identities[i].c_str());

  if (ssh_connect(s) != SSH_OK) {
    error = std::string("cannot connect to ") + p_host + ": " + ssh_get_error(s);
    ssh_free(s);
    return 0;
  }
  if (!checkHostKey(s, error) || !authenticate(s, error)) {
    ssh_disconnect(s);
    ssh_free(s);
    return 0;
  }
  return s;
}

bool SshTransport::checkHostKey(ssh_session s, std::string& error)
{
  enum ssh_known_hosts_e st = ssh_session_is_known_server(s);
  if (st == SSH_KNOWN_HOSTS_OK) return true;

  if (st == SSH_KNOWN_HOSTS_CHANGED || st == SSH_KNOWN_HOSTS_OTHER) {
    error = "the host key of " + p_host + " has CHANGED since it was recorded "
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
  if (ssh_session_update_known_hosts(s) != SSH_OK) {
    error = std::string("cannot record the host key: ") + ssh_get_error(s);
    return false;
  }
  return true;
}

// Sets rc to the libssh result; false means the user aborted.
bool SshTransport::authKeyboardInteractive(ssh_session s, int& rc, bool& asked, std::string& error)
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
      if (!p_prompt || !p_prompt(prompt, echo != 0, answer)) {
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

bool SshTransport::authenticate(ssh_session s, std::string& error)
{
  bool kbdintTried = false;
  int passwordsLeft = p_passwordAttempts;
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
      if (!authKeyboardInteractive(s, rc, asked, error)) return false;
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
        rc = ssh_userauth_password(s, 0, pw.c_str());
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
  if (!sftp(error)) return false;
  std::string target = homeRelative(remotePath);
  if (remoteKind(remotePath) == 1) target = joinPath(target, baseName(localPath));
  return putInto(this, p_sftp, p_session, localPath, target, error,
                 &SshTransport::put);
}

bool SshTransport::getTree(const std::string& remotePath,
                           const std::string& localPath, std::string& error)
{
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

struct SshStream {
  SshStream() : session(0), channel(0), appFd(-1), pumpFd(-1),
                graceMs(2000), intr(false), stop(false)
  { ctl[0] = ctl[1] = -1; }
  ssh_session session;
  ssh_channel channel;
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

  while (!fail) {
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

  ssh_channel_close(ch);
  ssh_channel_free(ch);
  ssh_disconnect(s);
  ssh_free(s);
  st->channel = 0;
  st->session = 0;
  shutdown(st->pumpFd, SHUT_RDWR);
  close(st->pumpFd);
  st->pumpFd = -1;
}

}  // namespace

SshStream* SshTransport::openStream(const std::string& script, int& fd,
                                    std::string& error)
{
  std::string err;
  std::string envp = envPrefix(err);
  if (!err.empty()) { error = err; return 0; }

  ssh_session s = newSession(error);
  if (!s) return 0;

  ssh_channel ch = ssh_channel_new(s);
  if (!ch || ssh_channel_open_session(ch) != SSH_OK ||
      ssh_channel_request_exec(ch, kStreamExec) != SSH_OK) {
    error = std::string("cannot start command: ") + ssh_get_error(s);
    if (ch) ssh_channel_free(ch);
    ssh_disconnect(s);
    ssh_free(s);
    return 0;
  }

  int sp[2], ctl[2];
  if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sp) != 0) {
    error = strerror(errno);
  } else if (pipe2(ctl, O_CLOEXEC) != 0) {
    error = strerror(errno);
    close(sp[0]); close(sp[1]);
  } else {
    SshStream* st = new SshStream;
    st->session = s;
    st->channel = ch;
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
  return 0;
}

void SshTransport::interruptStream(SshStream* st)
{
  if (!st) return;
  st->intr = true;
  if (write(st->ctl[1], "i", 1) < 0) {}
}

void SshTransport::closeStream(SshStream* st, int graceMs)
{
  if (!st) return;
  st->graceMs = graceMs;
  st->stop = true;
  if (write(st->ctl[1], "s", 1) < 0) {}
  st->thread.join();
  close(st->appFd);
  close(st->ctl[0]);
  close(st->ctl[1]);
  delete st;
}
