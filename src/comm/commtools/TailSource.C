#include "comm/TailSource.H"

#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "comm/RCommand.H"
#include "tdat/RefMachine.H"

using std::string;

TailSource::TailSource() : p_rcmd(0), p_ended(false) {}

TailSource::~TailSource() { close(); }

string TailSource::shPath(const string& p)
{
  string word = p, prefix;
  if (p == "~") return "\"$HOME\"";
  if (p.compare(0, 2, "~/") == 0) { prefix = "\"$HOME\"/"; word = p.substr(2); }
  string q = "'";
  for (size_t i = 0; i < word.size(); i++) {
    if (word[i] == '\'') q += "'\\''";
    else q += word[i];
  }
  return prefix + q + "'";
}

string TailSource::script(const string& path, int lines)
{
  if (lines < 1) lines = 1;
  // tail -F also waits for a file that is not there yet (a queued job).
  // stdin is the link back to ECCE: its end means stop.
  return "exec 2>&1\n"
         "tail -n " + std::to_string(lines) + " -F -- " + shPath(path) +
         " </dev/null &\n"
         "t=$!\n"
         "cat >/dev/null\n"
         "kill $t 2>/dev/null\n";
}

bool TailSource::open(const string& machineName, const string& shell,
                      const string& user, const string& path, int lines,
                      string& error, bool* missing)
{
  close();
  RefMachine* m = RefMachine::refLookup(machineName);
  if (!m) {
    error = "Machine \"" + machineName + "\" is not currently registered.";
    return false;
  }
  // The same login every other operation on this machine makes; an empty
  // password lets RCommand take it from AuthCache or ask through passdialog.
  RCommand* rc = new RCommand(m->fullname(), shell, m->shell(), user, "",
                              m->frontendMachine(), m->frontendBypass(),
                              m->shellPath(), m->libPath(), m->sourceFile());
  return start(rc, path, lines, error, missing);
}

bool TailSource::start(RCommand* rcmd, const string& path, int lines,
                       string& error, bool* missing)
{
  close();
  p_rcmd = rcmd;
  p_ended = false;
  if (!p_rcmd->isOpen()) {
    error = p_rcmd->commError();
    close();
    return false;
  }
  // Runs before the stream, on the same login: it also opens a shared
  // OpenSSH connection when one has to be logged in to first.
  const bool there = p_rcmd->exists(path);
  if (missing) *missing = !there;
  if (!p_rcmd->isOpen()) {
    error = p_rcmd->commError();
    close();
    return false;
  }
  if (!p_rcmd->startStream(script(path, lines), true)) {
    error = p_rcmd->commError();
    close();
    return false;
  }
  const string b = p_rcmd->sshBackend();
  p_backend = b.empty() ? "local" : b;
  int fd = p_rcmd->streamFd();
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
  return true;
}

int TailSource::fd() const
{
  return p_rcmd && !p_ended ? p_rcmd->streamFd() : -1;
}

bool TailSource::read(string& out)
{
  const int f = fd();
  if (f < 0) return false;
  char buf[16384];
  for (;;) {
    ssize_t n = ::read(f, buf, sizeof buf);
    if (n > 0) { out.append(buf, n); continue; }
    if (n < 0 && errno == EINTR) continue;
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return true;
    p_ended = true;
    return false;
  }
}

void TailSource::close()
{
  if (p_rcmd) {
    // EOF on the script's stdin ends tail; the grace covers a slow link.
    p_rcmd->stopStream(3000);
    delete p_rcmd;
    p_rcmd = 0;
  }
  p_backend.clear();
  p_ended = false;
}
