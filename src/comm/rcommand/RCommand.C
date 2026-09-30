//////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: RCommand.C
//
// DESIGN:
//
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
  using std::cout;
  using std::endl;
  using std::cerr;
#include <fstream>
  using std::ifstream;
  using std::ofstream;
#include <map>
#include <memory>
#include <set>
  using std::map;

#include <stdlib.h> // getenv
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>

#ifndef __APPLE__
#include <wait.h> // wait
#else
#include <sys/wait.h>
#endif

#include <sys/utsname.h> // uname
#include <sys/stat.h> // stat
#include <glob.h> // glob (on POSIX systems)
#include <signal.h> // SIGTERM
#include <netdb.h> // gethostbyname

#include "util/StringTokenizer.H"
#include "util/Ecce.H"

#include "tdat/AuthCache.H"

#include "comm/RCommand.H"

#include "comm/expect.h"
#include "comm/DirectTransport.H"
#ifdef ECCE_HAVE_LIBSSH
#include "comm/SshTransport.H"
#endif

#define MAXARGS 32
/*#define MAXLINE 256*/
#define MAXLINE 16384

static const char* sshpass_opts[] = {"-o", "PasswordAuthentication=yes",
                                     "-o", "StrictHostKeyChecking=no",
                                     "-o", "FallBackToRsh=no",
                                     "-o", "UseRsh=no",
                                     "-o", "RhostsAuthentication=no",
                                     "-o", "RhostsRSAAuthentication=no",
                                     "-o", "RSAAuthentication=no",
                                     "-o", "TISAuthentication=no",
                                     0};

// A machine CONFIG's locShell can be a bare name or a full path
// ("/usr/bin/bash", "/bin/csh", ...). Classify by basename, not by an
// exact match against the full string, so "/usr/bin/bash" is
// recognised the same as "bash". ECCE's LOCAL shell only ever needs to
// be csh, tcsh or bash (Andy, 2026-09-28) -- zsh/mksh/ksh/dash/sh are
// only ever relevant as a REMOTE login shell during a hop, which is a
// different code path (waitShellReady() below) and never runs this
// classification. Anything else here is refused outright rather than
// silently guessing csh syntax, which used to fail the login with a
// generic, misleading "(incorrect password?)" (#143/#69).
enum ShellDialect { SHELL_CSH, SHELL_BASH, SHELL_UNSUPPORTED };

static ShellDialect classifyShell(const string& locShell)
{
  string base = locShell;
  string::size_type slash = base.find_last_of('/');
  if (slash != string::npos)
    base = base.substr(slash + 1);

  if (base == "csh" || base == "tcsh")
    return SHELL_CSH;
  if (base == "bash")
    return SHELL_BASH;

  return SHELL_UNSUPPORTED;
}

// A login shell (tcsh, zsh, ksh93) freshly spawned by "locShell -i"
// can still be mid-setup -- reading its startup file, enabling its own
// line editor -- when we start writing to it, and a raw-mode editor's
// terminal setup can flush already-typed-ahead input, discarding it.
// A fixed sleep is exactly the hang-shaped risk this file already had
// (#143/#69): poll with a real probe instead, so the wait is only ever
// as long as it needs to be, and never longer than the retry budget.
static bool waitShellReady(int fid)
{
  static const char* probe = "echo ECCE_READY_''PROBE";
  // A bare substring, not "\r\n...\r\n"-anchored: bash's bracketed-paste
  // escapes and a shell's own prompt/echo quirks (bsd-csh prints its
  // prompt directly against a command's output with no newline between)
  // can land other bytes at that exact boundary. All that matters here
  // is "did this shell just run our command" -- unlike the item-1
  // sentinel, nothing downstream parses what follows this match.
  static const char* mark = "ECCE_READY_PROBE";
  size_t problen = strlen(probe);

  int savedTimeout = exp_timeout;
  exp_timeout = 1;

  bool ready = false;
  for (int tries = 0; !ready && tries < 10; tries++) {
    if (write(fid, probe, problen) != (ssize_t)problen ||
        write(fid, "\n", 1) != 1)
      break;

    if (exp_expectl(fid, exp_glob, mark, 1, exp_end) == 1) {
      ready = true;
    } else {
      // ksh93 can be left at a "> " continuation prompt if a flush cut
      // the probe mid-word; Ctrl-C plus a newline gets back to a plain
      // prompt before the next attempt instead of compounding garbage.
      write(fid, "\x03", 1);
      write(fid, "\n", 1);
    }
  }

  exp_timeout = savedTimeout;
  return ready;
}

RCommand::HostKeyHook RCommand::hostKeyHook = 0;

static string shQuote(const string& s)
{
  string q = "'";
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '\'') q += "'\\''";
    else q += s[i];
  }
  return q + "'";
}

// What the machine's sourceFile adds to the environment, found by running it
// once in the shell it was written for and diffing `env -0` before and after.
// Aliases and shell functions the file defines cannot be carried over.
// On success, `changed` names the variables the file set or removed.
bool RCommand::importSourceFile(const string& sourceFile, const string& locShell,
                                bool loginShell, std::set<std::string>& changed)
{
  string shell = locShell;
  if (loginShell) {
    TransportResult sr = p_transport->run("printf '%s' \"$SHELL\"\n", 60);
    if (sr.status == 0 && classifyShell(sr.out) != SHELL_UNSUPPORTED)
      shell = sr.out;
  }
  ShellDialect dialect = classifyShell(shell);
  if (dialect == SHELL_UNSUPPORTED) {
    p_errMessage = "Unsupported local shell '" + shell + "' for " +
                   p_machine + " -- ECCE needs csh, tcsh or bash";
    return false;
  }
  bool csh = (dialect == SHELL_CSH);
  if (csh && sourceFile.find_first_of("'!") != string::npos) {
    p_errMessage = "Unsuccessful remote shell login--source " + sourceFile +
                   " failed: quote or ! in the file name";
    return false;
  }

  // The test-and-source line is the one the pty login sends; -f and
  // --norc --noprofile keep everything but the file itself out of the diff.
  const string srcLine = csh ?
    "if (-e " + sourceFile + ") source " + sourceFile :
    "[ -e " + sourceFile + " ] && source " + sourceFile;
  const string inner = string("env -0; printf 'ECCE_SRC_A\\n'; ") + srcLine +
    "; printf 'ECCE_SRC_B\\n'; env -0; printf 'ECCE_SRC_E\\n'";
  string script = p_scriptPrefix + shQuote(shell) +
    (csh ? " -f -c " : " --norc --noprofile -c ") + shQuote(inner) + "\n";

  TransportResult r = p_transport->run(script, 60);
  const string mb = "ECCE_SRC_A\n", mc = "ECCE_SRC_B\n", me = "ECCE_SRC_E\n";
  string::size_type pa = r.out.find(mb), pb = r.out.find(mc);
  bool complete = r.status == 0 && pa != string::npos && pb != string::npos &&
                  pb > pa && r.out.size() >= me.size() &&
                  r.out.compare(r.out.size() - me.size(), me.size(), me) == 0;
  if (!complete) {
    string why = r.error != "" ? r.error : r.err;
    while (!why.empty() && (why[why.size()-1] == '\n' || why[why.size()-1] == '\r'))
      why.erase(why.size() - 1);
    p_errMessage = "Unsuccessful remote shell login--source " + sourceFile +
                   " failed" + (why != "" ? ": " + why : "");
    return false;
  }

  map<string, string> before, after;
  for (int pass = 0; pass < 2; pass++) {
    string block = pass == 0 ? r.out.substr(0, pa) :
      r.out.substr(pb + mc.size(), r.out.size() - me.size() - pb - mc.size());
    map<string, string>& dst = pass == 0 ? before : after;
    string::size_type at = 0;
    while (at < block.size()) {
      string::size_type nul = block.find('\0', at);
      if (nul == string::npos) nul = block.size();
      string::size_type eq = block.find('=', at);
      if (eq != string::npos && eq < nul)
        dst[block.substr(at, eq - at)] = block.substr(eq + 1, nul - eq - 1);
      at = nul + 1;
    }
  }

  // Shell bookkeeping, not environment; function exports (BASH_FUNC_x%%)
  // are not valid names and go with the functions.
  static const char* volatileNames[] = { "PWD", "OLDPWD", "SHLVL", "_", "PS1",
    "PS2", "PS4", "prompt", "COLUMNS", "LINES", "SHELL", 0 };
  struct Skip {
    static bool name(const string& n, const char** v) {
      if (n.empty() || isdigit((unsigned char)n[0])) return true;
      for (size_t i = 0; i < n.size(); i++)
        if (!isalnum((unsigned char)n[i]) && n[i] != '_') return true;
      for (int i = 0; v[i]; i++) if (n == v[i]) return true;
      return false;
    }
  };

  for (map<string, string>::const_iterator i = after.begin(); i != after.end(); ++i) {
    if (Skip::name(i->first, volatileNames)) continue;
    map<string, string>::const_iterator b = before.find(i->first);
    if (b == before.end() || b->second != i->second) {
      p_transport->setEnv(i->first, i->second);
      changed.insert(i->first);
    }
  }
  for (map<string, string>::const_iterator b = before.begin(); b != before.end(); ++b) {
    if (Skip::name(b->first, volatileNames) || after.count(b->first)) continue;
    p_transport->unsetEnv(b->first);
    changed.insert(b->first);
  }

  // The pty session stays in the directory the file cd'd to.
  map<string, string>::const_iterator wb = before.find("PWD"),
                                      wa = after.find("PWD");
  if (wb != before.end() && wa != after.end() && wa->second != wb->second)
    p_transport->setDir(wa->second);

  if (getenv("ECCE_RCOM_LOGMODE"))
    cout << "source file " << sourceFile << " (" << shell << "): "
         << changed.size() << " variable(s) imported" << endl;
  return true;
}

#ifdef ECCE_HAVE_LIBSSH
// One line from passdialog; false if it was cancelled or could not run.
static bool askPassdialog(const char* type, const string& machine,
                          const string& user, string& answer)
{
  string cmd = Ecce::ecceBinCommand("passdialog") + " " + type + " " +
               machine + " " + user;
  FILE* p = popen(cmd.c_str(), "r");
  if (!p) return false;
  char buf[MAXLINE];
  bool ok = fgets(buf, sizeof(buf), p) != NULL;
  pclose(p);
  if (!ok) return false;
  answer = buf;
  while (!answer.empty() && (answer[answer.size()-1]=='\n' ||
                             answer[answer.size()-1]=='\r'))
    answer.erase(answer.size()-1);
  return !answer.empty();
}

// Runs hostkeydialog.  ran=false when it could not start (no display, no
// program), so the caller can fall back to "run ssh once by hand".
static bool askHostKeyDialog(const string& host, const string& fp,
                             const string& keyType, bool& ran)
{
  ran = false;
  const char* d1 = getenv("DISPLAY");
  const char* d2 = getenv("WAYLAND_DISPLAY");
  if ((!d1 || !*d1) && (!d2 || !*d2)) return false;
  string cmd = Ecce::ecceBinCommand("hostkeydialog") + " " + shQuote(host) +
               " " + shQuote(fp) + " " + shQuote(keyType);
  FILE* p = popen(cmd.c_str(), "r");
  if (!p) return false;
  char buf[MAXLINE];
  string out;
  while (fgets(buf, sizeof(buf), p)) out += buf;
  int st = pclose(p);
  // 126/127: the shell could not run the program at all.
  if (WIFEXITED(st) && (WEXITSTATUS(st) == 126 || WEXITSTATUS(st) == 127))
    return false;
  ran = true;
  return WIFEXITED(st) && WEXITSTATUS(st) == 0 && out.find("accept") == 0;
}

static bool looksLikeCode(const string& prompt)
{
  string l;
  for (size_t i = 0; i < prompt.size(); i++)
    l += (char)tolower((unsigned char)prompt[i]);
  return l.find("passcode")!=string::npos || l.find("verification")!=string::npos ||
         l.find("token")!=string::npos || l.find("otp")!=string::npos ||
         l.find("duo")!=string::npos || l.find("code")!=string::npos;
}

// Logs in over libssh with the credentials the pty login loop would use:
// the password given, then AuthCache, then passdialog; passdialog's
// "passcode" for what looks like a second factor.
bool RCommand::sshConnect(const string& machine, const string& userName,
                          const string& password, const string& jumpHost)
{
  const string theUser = userName == "" ? string(Ecce::realUser()) : userName;

  // The callbacks outlive this call: the monitor stream logs in again on
  // a session of its own, reusing the password that just worked.  Each host
  // keeps its own password state, since a front end and the machine behind
  // it need not share one.
  struct Creds { string pass; bool passTried; };
  struct HostKey { string msg; };
  std::shared_ptr<HostKey> hk(new HostKey);
  const string shell = p_shell;

  auto newCreds = [&password]() {
    std::shared_ptr<Creds> c(new Creds);
    c->pass = password;
    c->passTried = false;
    return c;
  };
  auto promptFor = [shell, theUser](std::shared_ptr<Creds> c, const string& host) {
    return SshTransport::PromptFn([c, shell, host, theUser](const string& prompt,
                                                            bool echo, string& answer) {
      if (echo || looksLikeCode(prompt))
        return askPassdialog("passcode", host, theUser, answer);
      if (!c->passTried &&
          (c->pass != "" || RCommand::getPassCache(shell, host, theUser, c->pass))) {
        c->passTried = true;
        answer = c->pass;
        return true;
      }
      c->passTried = true;
      if (!askPassdialog("password", host, theUser, c->pass)) return false;
      answer = c->pass;
      return true;
    });
  };
  std::shared_ptr<Creds> c = newCreds(), cj = newCreds();

  // Port 0 leaves the port to ~/.ssh/config, as for the ssh command.
  SshTransport* t = new SshTransport(machine, 0, userName);
  t->setConnectTimeout(RC_CONNECT_TIMEOUT);
  t->setPasswordAttempts(3);
  t->setPromptCallback(promptFor(c, machine));
  if (jumpHost != "") {
    t->setJumpHost(jumpHost, 0, userName);
    t->setJumpPromptCallback(promptFor(cj, jumpHost));
  }
  t->setHostKeyCallback([hk](const string& host, const string& fingerprint,
                             const string& keyType) {
    if (RCommand::hostKeyHook) {
      if (RCommand::hostKeyHook(host, fingerprint)) return true;
    } else {
      bool ran;
      if (askHostKeyDialog(host, fingerprint, keyType, ran)) return true;
      if (ran) {
        hk->msg = "The host key of " + host + " (" + fingerprint +
                  ") was not accepted.";
        return false;
      }
    }
    hk->msg = "The host key of " + host + " (" + fingerprint +
              ") is not known.  Run \"ssh " + host + "\" once in a "
              "terminal to accept it, then try again.";
    return false;
  });

  string error;
  bool ok = t->connect(error);

  if (!ok) {
    p_errMessage = hk->msg != "" ? hk->msg :
                   "Unable to open ssh connection to " + machine + ": " + error;
    delete t;
    return false;
  }
  // A later session starts from the password that was accepted.
  c->passTried = false;
  cj->passTried = false;
  const string thePass = c->pass, theJumpPass = cj->pass;

  if (p_transport) {
    stopStream();
    delete p_transport;
  }
  p_transport = t;
  p_direct = true;
  p_ssh = true;
  p_remoteBash = true;
  exp_timeout = RC_EXEC_TIMEOUT;
  p_connected = true;

  if (thePass != "")
    RCommand::setPassCache(p_shell, machine, theUser, thePass);
  if (jumpHost != "" && theJumpPass != "")
    RCommand::setPassCache(p_shell, jumpHost, theUser, theJumpPass);

  if (getenv("ECCE_RCOM_LOGMODE"))
    cout << "ssh transport: commands run over libssh on " << machine
         << (jumpHost == "" ? "" : t->nested() ?
             " through " + jumpHost + " (nested ssh)" :
             " through " + jumpHost + " (forwarded connection)") << endl;
  return true;
}

// hop() over libssh: a new connection to hopMachine, through the same
// front end the current one used, or through the current machine when there
// was none.  The pty path types ssh into its shell; that would nest one
// hop deeper each time here, and compute nodes are reached from the front end.
bool RCommand::sshHop(const string& hopMachine, const string& locShell,
                      const string& userName, const string& password,
                      const string& shellPath, const string& libPath,
                      const string& sourceFile)
{
  SshTransport* cur = static_cast<SshTransport*>(p_transport);
  const string jump = cur->jumpHost() != "" ? cur->jumpHost() : cur->host();
  const string user = userName != "" ? userName : cur->user();
  const string pathLine = shellPath == "" ? "" :
    "PATH=" + shQuote(shellPath) + ":$PATH; export PATH\n";
  const string libLine = libPath == "" ? "" :
    "LD_LIBRARY_PATH=" + shQuote(libPath) + ":$LD_LIBRARY_PATH; "
    "export LD_LIBRARY_PATH\n";
  p_scriptPrefix = pathLine + libLine;
  if (!sshConnect(hopMachine, user, password, jump)) return false;
  if (sourceFile != "") {
    std::set<std::string> changed;
    if (!importSourceFile(sourceFile, locShell, true, changed)) {
      p_connected = false;
      return false;
    }
    p_scriptPrefix = (changed.count("PATH") ? "" : pathLine) +
                     (changed.count("LD_LIBRARY_PATH") ? "" : libLine);
  }
  return true;
}
#else
bool RCommand::sshConnect(const string&, const string&, const string&,
                          const string&)
{
  return false;
}

bool RCommand::sshHop(const string&, const string&, const string&,
                      const string&, const string&, const string&,
                      const string&)
{
  return false;
}
#endif

string RCommand::frontEndMode() const
{
#ifdef ECCE_HAVE_LIBSSH
  if (p_ssh && p_transport) {
    const SshTransport* t = static_cast<const SshTransport*>(p_transport);
    if (t->jumpHost() != "") return t->nested() ? "nested" : "forward";
  }
#endif
  return "";
}

bool RCommand::directUnsupported(const char* what)
{
  p_errMessage = string(what) + " is not available with ECCE_TRANSPORT=" +
                 (p_ssh ? "ssh" : "direct");
  return false;
}

int RCommand::expect1(const char* pat)
{
  if (p_direct) { directUnsupported("expect1"); return -1; }
  int ixp = exp_expectl(p_fid, exp_glob, pat, 1, exp_end);

  if (ixp < 1) {
    p_connected = false;
    p_errMessage =
      "Lost remote shell connection attempting to read command output";
  }

  return ixp;
}

int RCommand::expect2(const char* pat1, const char* pat2)
{
  if (p_direct) { directUnsupported("expect2"); return -1; }
  int ixp = exp_expectl(p_fid, exp_glob, pat1, 1,
                               exp_glob, pat2, 2, exp_end);

  if (ixp < 1) {
    p_connected = false;
    p_errMessage =
      "Lost remote shell connection attempting to read command output";
  }

  return ixp;
}

void RCommand::patalloc(int numPatterns, ...)
{
  if (p_direct) { directUnsupported("patalloc"); return; }
  int it;
  va_list ap;
  va_start(ap, numPatterns);

  p_pats = (struct exp_case*)malloc((numPatterns+1) * sizeof(struct exp_case));

  for (it=0; it<numPatterns; it++) {
    p_pats[it].pattern = strdup((char*)va_arg(ap, char*));
    p_pats[it].type = exp_glob;
    p_pats[it].value = it+1;
  }
  p_pats[numPatterns].type = exp_end;

  va_end(ap);
}

void RCommand::patfree(void)
{
  if (p_direct) return;
  int it;

  for (it=0; p_pats[it].type != exp_end; it++)
    free(p_pats[it].pattern);

  free((char*)p_pats);
}

int RCommand::patexpect(void)
{
  if (p_direct) { directUnsupported("patexpect"); return -1; }
  int ixp = exp_expectv(p_fid, p_pats);

  if (ixp < 1) {
    p_connected = false;
    p_errMessage =
      "Lost remote shell connection attempting to read command output";
  }

  return ixp;
}

int RCommand::expect(int numPatterns, ...)
{
  if (p_direct) { directUnsupported("expect"); return -1; }
  int it;
  va_list ap;
  va_start(ap, numPatterns);

  struct exp_case* pats = (struct exp_case*)malloc((numPatterns+1) *
                                                   sizeof(struct exp_case));
  for (it=0; it<numPatterns; it++) {
    pats[it].pattern = (char*)va_arg(ap, char*);
    pats[it].type = exp_glob;
    pats[it].value = it+1;
  }
  pats[numPatterns].type = exp_end;

  va_end(ap);

  int ixp = exp_expectv(p_fid, pats);

  free((char*)pats);

  if (ixp < 1) {
    p_connected = false;
    p_errMessage =
      "Lost remote shell connection attempting to read command output";
  }

  return ixp;
}

// A dead reader must give EPIPE, not SIGPIPE.
static bool sendAll(int fd, const string& data)
{
  size_t done = 0;
  while (done < data.size()) {
    ssize_t w = send(fd, data.data() + done, data.size() - done, MSG_NOSIGNAL);
    if (w > 0) done += w;
    else if (w < 0 && errno == EINTR) continue;
    else return false;
  }
  return true;
}

int RCommand::expfid(void)
{
  if (p_direct) {
    if (p_stream.rfd >= 0) return p_stream.rfd;
    directUnsupported("expfid");
    return -1;
  }
  return p_fid;
}

bool RCommand::expwrite(const string& command)
{
  if (p_direct) {
    if (p_stream.wfd < 0) return directUnsupported("expwrite");
    if (p_ssh ? sendAll(p_stream.wfd, command + "\n")
              : static_cast<DirectTransport*>(p_transport)->writeStream(
                  p_stream, command + "\n"))
      return true;
    p_errMessage = "Lost connection to the job monitor attempting to send command";
    return false;
  }
  int comlen = command.length();

  if (comlen >= MAXLINE) {
    p_errMessage = "Exceeds maximum C shell command length of 16384 characters";
    return false;
  }

  if (write(p_fid, command.c_str(), comlen)==comlen && write(p_fid, "\n", 1)==1)
    return true;

  p_connected = false;
  p_errMessage = "Lost remote shell connection attempting to send command";
  return false;
}

bool RCommand::expwrite(const char* command)
{
  if (p_direct) return expwrite(string(command));
  int comlen = strlen(command);

  if (comlen >= MAXLINE) {
    p_errMessage = "Exceeds maximum C shell command length of 16384 characters";
    return false;
  }

  if (write(p_fid, command, comlen)==comlen && write(p_fid, "\n", 1)==1)
    return true;

  p_connected = false;
  p_errMessage = "Lost remote shell connection attempting to send command";
  return false;
}

bool RCommand::expwritefull(const string& command)
{
  if (p_direct) return directUnsupported("expwritefull");
  int comlen = command.length();

  if (write(p_fid, command.c_str(), comlen)==comlen && write(p_fid, "\n", 1)==1)
    return true;

  p_connected = false;
  p_errMessage = "Lost remote shell connection attempting to send data";
  return false;
}

void RCommand::exptimeout(const int& timeout)
{
  if (timeout == 0)
    exp_timeout = RC_EXEC_TIMEOUT;
  else
    exp_timeout = timeout;
}

char* RCommand::expout(void)
{
  if (p_direct) {
    directUnsupported("expout");
    static char empty[1] = "";
    return empty;
  }
  *exp_match = '\0';

  // strip /r characters
  char* expptr = exp_buffer;
  int offset = 0;

  for (; *expptr != '\0'; expptr++) {
    if (*(expptr+offset) == '\r')
      offset++;

    *expptr = *(expptr+offset);
  }

  return exp_buffer;
}

bool RCommand::fidwrite(const int& fid, const string& command,
                        string& errMessage)
{
  int comlen = command.length();

  if (comlen >= MAXLINE) {
    errMessage = "Exceeds maximum C shell command length of 16384 characters";
    return false;
  }

  if (write(fid, command.c_str(), comlen)==comlen && write(fid, "\n", 1)==1)
    return true;

  errMessage = "Lost remote shell connection attempting to send command";
  return false;
}

string RCommand::whereami(void)
{
  string whereami = "";

  struct utsname _uname;
  if (uname(&_uname) != -1)
    whereami = _uname.nodename;

  return whereami;
}

bool RCommand::isRemote(const string& machine, const string& remShell,
                        const string& userName)
{
  // Check if it's a local user/machine connection because that's more
  // straightforward conditional logic-wise than checking for a remote
  // connection

  bool localFlag = false;

  {
    string whereami = RCommand::whereami();

    // "localhost" and the loopback address name this machine as surely as
    // its own hostname does, but were not recognised -- so a machine
    // registered as "localhost" was treated as remote and ssh'd to, which
    // needs working key-based authentication to yourself for something
    // that should never leave the process. siteconfig/Machines ships such
    // an entry, since a shipped file cannot know the hostname it lands on.
    bool loopback = (machine=="localhost" ||
                     machine=="localhost.localdomain" ||
                     machine=="127.0.0.1" || machine=="::1");

    if (whereami != "" || loopback)
      localFlag = (loopback || machine=="" || machine=="-f" ||
                   machine=="system" || machine==whereami ||
                   (whereami != "" &&
                    machine.length()>whereami.length() &&
                    machine.substr(0,whereami.length())==whereami &&
                    machine[whereami.length()]=='.'));

    if (localFlag)
      localFlag = userName=="" || strcmp(userName.c_str(), Ecce::realUser())==0;
  }

  return !localFlag;
}

/**
 * Say where a job for this machine will actually run (#144).
 *
 * A machine registered under a loopback name (or this host's own name) is
 * run LOCALLY when the login name is empty or your own, and over the
 * remote shell to that name when it is anyone else's -- the deliberate
 * escape hatch a port-forwarded cluster login relies on. Nothing used to
 * say which branch was taken, and when the code is installed here too, a
 * job meant for the cluster succeeds on the workstation instead.
 *
 * Deliberately built from isRemote() itself rather than restating its
 * rules, so this can never describe a different decision than the one the
 * launch makes.
 */
string RCommand::localityNote(const string& machine, const string& remShell,
                              const string& userName)
{
  if (machine == "" || RCommand::isRemote(machine, "ssh", "")) {
    return "";
  }

  if (!RCommand::isRemote(machine, remShell, userName)) {
    return string("runs locally, as ") + Ecce::realUser();
  }

  string shell = remShell == "" ? string("the remote shell") : remShell;
  string note = "via " + shell + " to " + machine;
  if (userName != "") note += " as " + userName;
  return note;
}


bool RCommand::isSameDomain(const string& machine)
{
  bool sameDomain = false;
  string whereami = RCommand::whereami();

  struct hostent* host = gethostbyname(whereami.c_str());
  if (host != NULL) {
    string mymachine = host->h_name;

    char* mymachinestr = strdup((char*)mymachine.c_str());
    char* machinestr = strdup((char*)machine.c_str());

    // strange little bit of logic to compare the last two dot-separated
    // parts of machine and mymachine
    char* domstr = strrchr(machinestr, '.');
    if (domstr != NULL) {
      domstr--;
      while (domstr!=NULL && *domstr!='.' && domstr!=machinestr)
        domstr--;
      if (domstr != NULL) {
        char* mydomstr = strrchr(mymachinestr, '.');
        if (mydomstr != NULL) {
          mydomstr--;
          while (mydomstr!=NULL && *mydomstr!='.' && mydomstr!=mymachinestr)
            mydomstr--;
          if (mydomstr != NULL)
            sameDomain = strcmp(domstr, mydomstr)==0;
        }
      }
    }

    free(mymachinestr);
    free(machinestr);
  }

  return sameDomain;
}

string RCommand::removedShellMessage(const string& remShell)
{
  string name = remShell.substr(0, remShell.find('/'));
  if (name=="telnet" || name=="Globus" || name=="Globus-ssh")
    return "The remote shell '" + name + "' is no longer supported by ECCE; "
           "edit this machine in Machine Registration and choose ssh.";
  return "";
}

string RCommand::shellCommand(const string& remShell, const string& machine,
                              const string& locShell, const string& userName,
                              const bool& hopFlag, string& proxyAuth,
                              char** argv)
{
  string theShell;

  static const char* minfc  =  "-fc";
  static const char* minc  =  "-c";

  // REVERTED 2026-09-03: "--norc --noprofile" (instead of csh-style "-f")
  // for bash here was well-intentioned (see git history) but turned out
  // to be actively harmful for at least one real setup: it skips
  // ~/.bashrc entirely, and for a user whose .bashrc is what actually
  // sets up a compute code's environment (e.g. sourcing a vendor
  // profile script that exports GAUSS_ARCHDIR/GAUSS_BSDDIR/G16BASIS/etc
  // -- more than the ECCE-generated submit script itself sets), that
  // silently broke job launches that depended on it, coinciding exactly
  // with the fix landing. Reverted pending a fix that doesn't assume
  // anything about what a user's dotfiles do or don't need to provide --
  // e.g. having ECCE's own generated submit scripts source the same
  // vendor profile explicitly, so job correctness never depends on
  // ~/.bashrc content one way or the other. The original bracketed-
  // paste-mode issue this was investigating is independent and still
  // handled further down (the "unalias -a...bind...enable-bracketed-
  // paste off" block).
  string echoshell = "echo +hi+ && " + locShell + " -f";

  static const char* minl  =  "-l";
  // bash spawned this way (as the remote command of a real ssh session,
  // as opposed to the same-domain "local shell" exp_spawnv() shortcut
  // built from echoshell above) has a confirmed, reproducible bug:
  // readline duplicates a trailing fragment of a long command line's
  // echo a second time after the real, correct echo -- and since a
  // caller's command text can legitimately contain its own "did this
  // die" marker as a literal substring (e.g. job monitoring's "echo
  // eccejobmonitor_went_bye_bye"), that duplicate reads as a false
  // "command already finished" signal. Confirmed live, directly: the
  // identical test over the "local shell" shortcut (which also runs
  // bash, just not through ssh) never reproduces this -- so it's
  // specific to interactive bash under a real pty-forwarded ssh
  // session, not bash in general. --noediting disables readline
  // entirely while keeping -i (interactive: reads ~/.bashrc, sets a
  // default prompt, etc.) intact.
  // bash requires GNU long options before short options in the same
  // invocation ("bash -i --noediting" errors with "--: invalid option";
  // "bash --noediting -i" is the form that actually works) -- confirmed
  // directly.
  static const char* cmdBash[] = {"echo", "+hi+", "&&", "", "--noediting", "-i", 0};
  static const char* cmdOther[] = {"echo", "+hi+", "&&", "", "-i", 0};
  const char** cmd = (locShell == "bash") ? cmdBash : cmdOther;
  cmd[3] = strdup(locShell.c_str());

  // ssh verbose flag for recognizing authentication success/failure
  static const char* minv  =  "-v";

  // for forwarding ssh X11 connections
  static const char* mino  =  "-o";
  static const char* minx  =  "ForwardX11=yes";

  int argc = 1, it;

  string theMachine = (machine=="" || machine=="-f" || machine=="system")?
                       RCommand::whereami(): machine;

  if (RCommand::isRemote(machine, remShell, userName)) {
    if (remShell=="" || remShell=="ssh" || remShell=="sshpass" ||
        remShell.find("ssh/")==0) {
      theShell = "ssh";

      // enable ssh verbose mode
      argv[argc++] = (char*)minv;

      // enable ssh X11 port forwarding
      argv[argc++] = (char*)mino;
      argv[argc++] = (char*)minx;

      // All the ssh "-o" options that attempt to force password authentication.
      // They seem to have some effect although I'm sure the server side sshd
      // daemon ultimately decides what authentication it will accept.
      // Only apply these if the shell is sshpass to potentially allow other
      // types of ssh authentication as long as it doesn't break the expect
      // pattern matching.
      if (remShell == "sshpass")
        for (it=0; sshpass_opts[it]!=(char*)0; it++)
          argv[argc++] = (char*)sshpass_opts[it];

    } else if (remShell=="rsh" || remShell.find("rsh/")==0)
      theShell = "rsh";

    else {
      theShell = RCommand::userShellCommandArgs(remShell, proxyAuth, argc,argv);
      if (theShell=="ssh" || (theShell.find("/ssh")!=string::npos &&
                              theShell.find("/ssh")==theShell.length()-4)) {
        // enable ssh verbose mode
        argv[argc++] = (char*)minv;

        // enable ssh X11 port forwarding
        argv[argc++] = (char*)mino;
        argv[argc++] = (char*)minx;
      }
    }

    if (userName!="") {
      argv[argc++] = (char*)minl;
      argv[argc++] = strdup((char*)userName.c_str());
    }

    argv[argc++] = strdup((char*)theMachine.c_str());

    // for ssh, recognize authentication success/failure from verbose
    // mode logging and request a local "csh" shell after the connection
    // is established if there are no hops being done
    // Otherwise there are problems with authentication when making hops
    // to other machines from the one initially logged in on
    if (theShell!="ssh" || !hopFlag) {
      for (it=0; cmd[it]!=(char*)0; it++)
        argv[argc++] = (char*)cmd[it];
    }

  } else {
    theShell = locShell;

    // The "-f" or "system" value for the machine indicates a local launch
    // that is a fast shell (doesn't read .cshrc) and thus picks up the
    // environment of the calling process.  This is suitable for using an
    // RCommand instance to replace the usual system() calls.
    if (machine=="-f" || machine=="system")
      argv[argc++] = (char*)minfc;
    else
      argv[argc++] = (char*)minc;

    argv[argc++] = strdup((char*)echoshell.c_str());
  }

  argv[0] = strdup((char*)theShell.c_str());
  argv[argc] = (char*)0;

  if (exp_loguser == 1) {
    cout << "Remote shell command:" << endl;
    for (it=0; it<argc; it++)
      cout << "arg " << it << ": " << argv[it] << endl;
    cout << "End remote shell command" << endl; 
  }

  return theShell;
}

string RCommand::userCommand(const string& command,
                             const string& fullShell, const string& machine,
                             const string& locShell, const string& userName,
                             string& proxyAuth, char** argv)
{
  static const char* minfc  =  "-fc";
  static const char* minc  =  "-c";
  static const char* minl  =  "-l";

  // for forwarding ssh X11 connections
  static const char* mino  =  "-o";
  static const char* minx  =  "ForwardX11=yes";

  int argc = 1, it;
  string theShell;
  string theMachine = (machine=="" || machine=="-f" || machine=="system")?
                       RCommand::whereami(): machine;

  char* tokenize;
  string remShell = "";

  // strtok hangs with an empty string
  if (fullShell != "") {
    tokenize = strdup((char*)fullShell.c_str());
    remShell = strtok(tokenize, " ");
  }

  if (RCommand::isRemote(machine, remShell, userName)) {
    if (remShell=="" || remShell=="ssh" || remShell=="sshpass" ||
        remShell.find("ssh/")==0) {
      theShell = "ssh";

      // enable ssh X11 port forwarding
      argv[argc++] = (char*)mino;
      argv[argc++] = (char*)minx;

      // All the ssh "-o" options that attempt to force password authentication.
      // They seem to have some effect although I'm sure the server side sshd
      // daemon ultimately decides what authentication it will accept.
      // Only apply these if the shell is sshpass to potentially allow other
      // types of ssh authentication as long as it doesn't break the expect
      // pattern matching.
      if (remShell == "sshpass")
        for (it=0; sshpass_opts[it]!=(char*)0; it++)
          argv[argc++] = (char*)sshpass_opts[it];

    } else if (remShell=="rsh" || remShell.find("rsh/")==0) {
      theShell = "rsh";

    } else {
      theShell = RCommand::userShellCommandArgs(remShell, proxyAuth, argc,argv);
      if (theShell=="ssh" || (theShell.find("/ssh")!=string::npos &&
                              theShell.find("/ssh")==theShell.length()-4)) {
        // enable ssh X11 port forwarding
        argv[argc++] = (char*)mino;
        argv[argc++] = (char*)minx;
      }
    }

    if (fullShell.find(" -l ") != string::npos) {
      string subme = fullShell;
      int idx = subme.find("##user##");
      if (idx != string::npos)
        subme.replace(idx, 8, userName);

      idx = subme.find("##machine##");
      if (idx != string::npos)
        subme.replace(idx, 11, theMachine);

      // tokenize with ##command## within the string so we can substitute
      // for this in argv directly w/o tokenizing the command itself
      char* tokenify = strdup((char*)subme.c_str());
      string tossShell = strtok(tokenify, " ");

      while ((argv[argc++] = strtok(NULL, " ")) != NULL);
      argc--;

      // find ##command## and then throw in the value for command in the
      // same place within argv
      for (idx=1; idx<argc && strcmp(argv[idx], "##command##")!=0 &&
                  strcmp(argv[idx], "command##")!=0; idx++);

      if (idx < argc) {
        if (command != "") {
          // prepend "csh/tcsh -c" for standard remote shells so we know the
          // environment the command will be executed under
          if (theShell=="ssh" || theShell=="rsh") {
            string cshCommand = locShell + " -c '";
            cshCommand += command + "'";
            argv[idx] = strdup((char*)cshCommand.c_str());
          } else
            argv[idx] = strdup((char*)command.c_str());

          // check if the preceeding argument started with a ## which
          // indicates that the command was actually a compound structure
          // such as ##-e command##
          // In this case we get rid of the leading ## and combine them
          // as a single argument
          if (idx>0 && strncmp(argv[idx-1], "##", 2)==0) {
            char* cptr = argv[idx-1];
            cptr += 2;
            argv[idx-1] = (char*)malloc(strlen(cptr) + strlen(argv[idx]) + 2);
            strcpy(argv[idx-1], cptr);
            strcat(argv[idx-1], " ");
            strcat(argv[idx-1], argv[idx]);

            argc--;
            for (it=idx; it<argc; it++)
              argv[it] = argv[it+1];
          }

        } else {
          // no command--get rid of ##command## placeholder from argv
          argc--;
          for (it=idx; it<argc; it++)
            argv[it] = argv[it+1];

          // if it is a compound command then get rid of that argument too
          if (idx>0 && strncmp(argv[idx-1], "##", 2)==0) {
            argc--;
            for (it=idx-1; it<argc; it++)
              argv[it] = argv[it+1];
          }
        }

      } else if (command != "") {
        // append command as a new last arg
        // prepend "csh/tcsh -c" for standard remote shells so we know the
        // environment the command will be executed under
        if (theShell=="ssh" || theShell=="rsh") {
          string cshCommand = locShell + " -c '" + command + "'";
          argv[argc++] = strdup((char*)cshCommand.c_str());
        } else
          argv[argc++] = strdup((char*)command.c_str());
      }

    } else {
      if (userName!="") {
        argv[argc++] = (char*)minl;
        argv[argc++] = strdup((char*)userName.c_str());
      }

      argv[argc++] = strdup((char*)theMachine.c_str());

      // append on any extra args given with fullShell
      // this completes the strtok up at the top of the method
      while ((argv[argc++] = strtok(NULL, " ")) != NULL);
      argc--;

#if 000
      // seems like passing the whole command as a single argument works
      // just fine.  But if it doesn't then it can simply be tokenized
      // which for some reason also behaves correctly with quotes around
      // tokens to indicate grouping
      tokenize = strdup((char*)command.c_str());
      argv[argc++] = strtok(tokenize, " ");
      while ((argv[argc++] = strtok(NULL, " ")) != NULL);
      argc--;
#else
      if (command != "") {
        // prepend "csh/tcsh -c" for standard remote shells so we know the
        // environment the command will be executed under
        if (theShell=="ssh" || theShell=="rsh") {
          string cshCommand = locShell + " -c '" + command + "'";
          argv[argc++] = strdup((char*)cshCommand.c_str());
        } else
          argv[argc++] = strdup((char*)command.c_str());
      }
#endif
    }

  } else {
    theShell = locShell;

    // The "-f" or "system" value for the machine indicates a local launch
    // that is a fast shell (doesn't read .cshrc) and thus picks up the
    // environment of the calling process.  This is suitable for using an
    // RCommand instance to replace the usual system() calls.
    if (machine=="-f" || machine=="system")
      argv[argc++] = (char*)minfc;
    else
      argv[argc++] = (char*)minc;

    if (command != "")
      argv[argc++] = strdup((char*)command.c_str());
  }

  argv[0] = strdup((char*)theShell.c_str());
  argv[argc] = (char*)0;

  if (exp_loguser == 1) {
    cout << "Remote shell command:" << endl;
    for (it=0; it<argc; it++)
      cout << "arg " << it << ": " << argv[it] << endl;
    cout << "End remote shell command" << endl; 
  }

  return theShell;
}

string RCommand::userShellCommandArgs(const string& remShell, string& proxyAuth,
                                      int& argc, char** argv)
{
  proxyAuth = "";
  string ret = remShell;
  string shellMatch = remShell + ":";

  string siteShellFile = Ecce::ecceHome();
  siteShellFile += "/siteconfig/remote_shells.site";

  if (access(siteShellFile.c_str(), F_OK) == 0) {
    ifstream is(siteShellFile.c_str());
    char buf[MAXLINE];
    char* tok;
    while (!is.eof()) {
      is.getline(buf, MAXLINE);
      if (buf[0]!='\0' && buf[0]!='#') {
        if (strncmp(buf, shellMatch.c_str(), shellMatch.length()) == 0) {
          tok = &buf[shellMatch.length()];
          char* afterptr = NULL;
          char* slashptr = strchr(tok, '|');
          if (slashptr != NULL) {
            afterptr = slashptr+1;
            *slashptr = '\0';
	  }

          tok = strtok(tok, " ");
          ret = tok==NULL? remShell.c_str(): tok;

          while ((tok = strtok(NULL, " ")) != NULL)
            argv[argc++] = strdup(tok);

          if (afterptr != NULL) {
            afterptr = strchr(afterptr, '|');
            if (afterptr != NULL) {
              afterptr = strchr(afterptr+1, '|');
              if (afterptr!=NULL && afterptr+1!=NULL)
                // this is the 4th item
                proxyAuth = afterptr+1; 
            }
          }
          break;
        }
      }
    }
    is.close();
  }

  string::size_type slash = ret.find('\\');
  if (slash != string::npos)
    ret.resize(slash);

  return ret;
}

string RCommand::userCopyCommandArgs(const string& remCopy, string& proxyAuth,
                                     int& argc, char** argv)
{
  string ret = remCopy;
  string copyMatch = remCopy + ":";

  string siteShellFile = Ecce::ecceHome();
  siteShellFile += "/siteconfig/remote_shells.site";

  if (access(siteShellFile.c_str(), F_OK) == 0) {
    ifstream is(siteShellFile.c_str());
    char buf[MAXLINE];
    char* tok;
    char* tokend;
    while (!is.eof()) {
      is.getline(buf, MAXLINE);
      if (buf[0]!='\0' && buf[0]!='#') {
        if (strncmp(buf, copyMatch.c_str(), copyMatch.length()) == 0) {
          tok = strchr(&buf[copyMatch.length()], '|');
          char* afterptr = NULL;
          if (tok != NULL) {
            tok++;
            tokend = strchr(tok, '|');
            if (tokend != NULL) {
              afterptr = tokend+1;
              *tokend = '\0';
	    }
          } else
            tok = &buf[copyMatch.length()];

          tok = strtok(tok, " ");
          ret = tok==NULL? remCopy.c_str(): tok;

          while ((tok = strtok(NULL, " ")) != NULL)
            argv[argc++] = strdup(tok);

          if (afterptr != NULL) {
            afterptr = strchr(afterptr+1, '|');
            if (afterptr!=NULL && afterptr+1!=NULL)
              // this is the 4th item
              proxyAuth = afterptr+1; 
          }
          break;
        }
      }
    }
    is.close();
  }

  string::size_type slash = ret.find('\\');
  if (slash != string::npos) {
    ret.replace(0, slash+1, "");

    slash = ret.find('\\');
    if (slash != string::npos)
      ret.replace(slash, ret.length()-slash, "");

  } else if (ret.rfind("sh")!=string::npos && ret.rfind("sh")==ret.length()-2)
    ret.replace(ret.length()-2, 2, "cp");

  return ret;
}

string RCommand::copyCommand(const string& remShell, const bool& isRemote,
                             const string& machine, const string& userName,
                             string& proxyAuth, int& argc, char** argv)
{
  string theCopy;

  static const char* minr = "-r";

  static const char* mini = "-i";

  argc = 1;
  int it;

  if (isRemote) {
    if (remShell=="" || remShell=="scp" ||
        (remShell.find("/scp")!=string::npos &&
         remShell.find("/scp")==remShell.length()-4) ||
        remShell=="ssh" || remShell=="sshpass") {
      theCopy = "scp";

      if (remShell == "sshpass")
        for (it=0; sshpass_opts[it]!=(char*)0; it++)
          argv[argc++] = (char*)sshpass_opts[it];

      argv[argc++] = (char*)minr;

    } else if (remShell=="rcp" ||
               (remShell.find("/rcp")!=string::npos &&
                remShell.find("/rcp")==remShell.length()-4) ||
               remShell=="rsh") {
      theCopy = "rcp";
      argv[argc++] = (char*)minr;

    } else if (remShell=="ftp" ||
               (remShell.find("/ftp")!=string::npos &&
                remShell.find("/ftp")==remShell.length()-4)) {
      theCopy = "ftp";
      argv[argc++] = (char*)mini;
      argv[argc++] = strdup((char*)machine.c_str());

    } else if (remShell=="sftp" ||
               (remShell.find("/sftp")!=string::npos &&
                remShell.find("/sftp")==remShell.length()-5)) {
      theCopy = "sftp";
      string useratmach = userName + "@" + machine;
      argv[argc++] = strdup((char*)useratmach.c_str());

    } else
      theCopy = RCommand::userCopyCommandArgs(remShell, proxyAuth, argc, argv);

  } else {
    theCopy = "cp";
    argv[argc++] = (char*)minr;
  }

  argv[0] = strdup((char*)theCopy.c_str());
  argv[argc] = (char*)0;

  return theCopy;
}

string RCommand::copyToShell(const string& copyCmd)
{
  string shellCmd = copyCmd;

  if (copyCmd=="scp" || copyCmd=="sftp")
    shellCmd = "ssh";
  else if (copyCmd == "rcp")
    shellCmd = "rsh";

  return shellCmd;
}

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Create Context for Executing Shell Commands.
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
bool RCommand::usesSsh(const string& machine, const string& remShell,
                       const string& userName)
{
  const char* mode = getenv("ECCE_TRANSPORT");
  return mode && !strcmp(mode, "ssh") &&
         RCommand::isRemote(machine, remShell, userName) &&
         (remShell=="" || remShell=="ssh" || remShell=="sshpass" ||
          remShell.find("ssh/")==0);
}

bool RCommand::usesLibssh(const string& machine, const string& remShell,
                          const string& userName)
{
#ifdef ECCE_HAVE_LIBSSH
  return usesSsh(machine, remShell, userName);
#else
  return false;
#endif
}

RCommand::RCommand(const string& machine, const string& remShell,
                   const string& locShell, const string& userName,
                   const string& password, const string& frontendMachine,
                   const string& frontendBypass, const string& shellPath,
                   const string& libPath, const string& sourceFile,
                   bool allowDirect, bool allowSsh)
{
  p_connected = false;
  p_background = false;
  p_hopCount = 0;
  p_remoteBash = false;
  p_transport = 0;
  p_direct = false;
  p_ssh = false;
  p_sshStream = 0;
  p_fid = -1;
  p_pid = 0;
  p_pats = 0;

  if (getenv("ECCE_RCOM_DEBUGGING"))
    exp_is_debugging = 1;
  else
    exp_is_debugging = 0;

  if (getenv("ECCE_RCOM_LOGMODE")) {
    exp_loguser = 1;
    cout << endl;
    cout << "Creating remote shell:" << endl;
    cout << "machine (" << machine << ")" << endl;
    cout << "remote shell (" << remShell << ")" << endl;
    cout << "local shell (" << locShell << ")" << endl;
    cout << "user name (" << userName << ")" << endl;
    cout << "password is " << password.length() << " characters" << endl;
    if (frontendMachine != "") {
      cout << "frontend machine (" << frontendMachine << ")" << endl;
      if (frontendBypass != "")
        cout << "frontend bypass domain (" << frontendBypass << ")" << endl;
    }
  } else
    exp_loguser = 0;

  // Catch exp_buffer overflows and bump up the size of the internal
  // expect buffers to something reasonable instead of the default 2000 chars
  exp_match_max = 50000;
  // Found that exp_full_buffer was resulting in some bizarre behavior with
  // mpp2 "shellput" file transfer freezing up.  So, disable it and hopefully
  // this won't cause any issues with other aspects of remote communication
  //exp_full_buffer = 1;

  // Bump up timeout because connection failures can take a long time
  exp_timeout = RC_CONNECT_TIMEOUT;

  // Set the machine and user name variables for the benefit of error
  // messages (the values passed in are const)
  p_machine = (machine=="" || machine=="-f" || machine=="system")?
               RCommand::whereami(): machine;

  if (RCommand::isRemote(machine, remShell, userName)) {
    p_errMessage = RCommand::removedShellMessage(remShell);
    if (p_errMessage != "")
      return;
  }

  const char* transportEnv = getenv("ECCE_TRANSPORT");
  const string transportMode = transportEnv ? transportEnv : "";
  if (allowDirect && (transportMode=="direct" || transportMode=="ssh") &&
      !RCommand::isRemote(machine, remShell, userName) &&
      frontendMachine=="") {
    p_direct = true;
    p_transport = new DirectTransport;
    p_remoteBash = true;
    exp_timeout = RC_EXEC_TIMEOUT;

    if (shellPath != "") {
      const char* cur = getenv("PATH");
      p_transport->setEnv("PATH", shellPath + ":" + (cur ? cur : ""));
    }
    if (libPath != "") {
      const char* cur = getenv("LD_LIBRARY_PATH");
      p_transport->setEnv("LD_LIBRARY_PATH",
                          libPath + ":" + (cur ? cur : ""));
    }

    if (sourceFile != "") {
      std::set<std::string> changed;
      if (!importSourceFile(sourceFile, locShell, false, changed))
        return;
    }

    if (getenv("ECCE_RCOM_LOGMODE"))
      cout << "Direct mode: commands run without a shell session" << endl;

    p_connected = true;
    return;
  }

  if (allowDirect && allowSsh && usesSsh(machine, remShell, userName)) {
#ifdef ECCE_HAVE_LIBSSH
    p_shell = "ssh";
    const string pathLine = shellPath == "" ? "" :
      "PATH=" + shQuote(shellPath) + ":$PATH; export PATH\n";
    const string libLine = libPath == "" ? "" :
      "LD_LIBRARY_PATH=" + shQuote(libPath) + ":$LD_LIBRARY_PATH; "
      "export LD_LIBRARY_PATH\n";
    p_scriptPrefix = pathLine + libLine;
    // A refused or failed login is final: falling back to the pty would
    // only prompt the user a second time for the same thing.
    // The pty path's own test: a machine inside the front end's domain is
    // reached directly.
    const string jump = frontendMachine != "" &&
      (frontendBypass=="" || !RCommand::isSameDomain(frontendBypass)) ?
      frontendMachine : string("");
    if (sshConnect(p_machine, userName, password, jump) && sourceFile != "") {
      std::set<std::string> changed;
      if (!importSourceFile(sourceFile, locShell, true, changed)) {
        p_connected = false;
        return;
      }
      // A variable the file set already carries the prefix it saw.
      p_scriptPrefix = (changed.count("PATH") ? "" : pathLine) +
                       (changed.count("LD_LIBRARY_PATH") ? "" : libLine);
    }
    return;
#else
    static bool warned = false;
    if (!warned) {
      cerr << "ECCE_TRANSPORT=ssh: this build has no libssh; using the "
              "pty ssh path" << endl;
      warned = true;
    }
#endif
  }

  string theMachine, shellMachine;

  // for machines that use a front-end (like mpp2), check if the machine
  // running ECCE is within the domain given by frontendBypass.  If so, the
  // there is no need to connect to the front-end first.
  if (frontendMachine!="" &&
      (frontendBypass=="" || !RCommand::isSameDomain(frontendBypass))) {
    theMachine = shellMachine = frontendMachine;
    p_hopCount++;
  } else {
    theMachine = p_machine;
    shellMachine = machine;
  }

  // Need updatable copies
  string theUser;
  if (userName == "")
    theUser = Ecce::realUser();
  else
    theUser = userName;

  string thePass = password;

  char *argv[MAXARGS];

  string proxyAuth;
  p_shell = RCommand::shellCommand(remShell, shellMachine, locShell, userName,
                                   p_hopCount>0, proxyAuth, argv);

  if (proxyAuth != "") {
    if (!RCommand::userproxy(proxyAuth, theMachine, userName,
                             password, p_errMessage))
      return;
  }

  if ((p_fid = exp_spawnv((char*)p_shell.c_str(), argv)) <= 0) {
    p_errMessage = "Unable to run remote shell " + p_shell +
                   " (not in the path?)";
    return;
  }

  bool done;
  string output;
  bool login_prompt = false;

  // Save away spawned process id in order to use waitpid in destructor
  // which guarantees it will be waiting on the right process
  p_pid = exp_pid;

  string notFoundStr = p_shell + ": Command not found";

  // passcode prompting variables
  string passCmd;
  FILE* passPtr;
  char passBuf[MAXLINE];
  passBuf[0] = '\0';
  char codeBuf[MAXLINE];

  bool iHop = false;

hopToIt:

  do {
    done = true;  // be optimistic

    switch (exp_expectl(p_fid, exp_glob, notFoundStr.c_str(), 1,
                               exp_glob, "execvp(", 1,
                               exp_glob, "denied", 2,
                               exp_glob, "failed", 2,
                               exp_glob, " closed", 2,
                               exp_glob, "Connection refused", 3,
                               exp_glob, "Bad host name", 4,
                               exp_glob, "Unknown host", 4,
                               exp_glob, "incorrect", 5,
                               exp_glob, "Connection timed out", 6,
                               exp_glob, "^Usage:", 7,
                               exp_glob, "\r\nUsage:", 7,
                               exp_glob, "(yes/no)? $", 8,
                               exp_glob, "password: $", 9,
                               exp_glob, "Password: $", 9,
                               exp_glob, "Password:$", 9,
                               exp_glob, "passphrase*: $", 9,
                               exp_glob, "PASSCODE:$", 10,
                               exp_glob, "PASSCODE: $", 10,
                               exp_glob, "login: $", 12,
                               exp_glob, "Authentication succeeded", 14,
                               // Modern OpenSSH's actual -v output for a
                               // successful key-based (no password prompt)
                               // login is "Authenticated to <host> ...
                               // using \"publickey\"." -- not the literal
                               // "Authentication succeeded" text above,
                               // which this decades-old pattern list has
                               // apparently always expected. Confirmed via
                               // a direct `ssh -v` run against a real
                               // key-trusted host: this is genuinely what
                               // current OpenSSH prints, not a fluke.
                               // Without this, a key-authenticated
                               // connection is never recognized as
                               // successful and the loop times out --
                               // reported live as "Failed to open remote
                               // shell ... (incorrect password?)" against
                               // a machine that never even prompted for
                               // one. Password-based logins were already
                               // fine (they complete via the "+hi+\r\n"
                               // echo marker below, once the shell after a
                               // successful password entry is reached).
                               exp_glob, "Authenticated to*", 14,
                               exp_glob, "+hi+\r\n", 14,
                               exp_end)) {

      case 1:
        p_errMessage = "Unable to find remote shell " + p_shell +
                       " (not in the path?)";
        return;

      case 2:
      case EXP_EOF:
        p_errMessage = "Permission to run remote shell " + p_shell +
                       " denied for " + theMachine;
        if (p_shell == "rsh")
          p_errMessage +=
                       " (do you have a .rhosts entry on " + theMachine + "?)";
        else if (p_shell == "ssh")
          p_errMessage += " (incorrect password?)";
      return;

      case 3:
        p_errMessage = "Shell authentication server for " + p_shell +
                       " not running or installed on " + theMachine;
        return;

      case 4:
        p_errMessage = "Unknown or unavailable host " + theMachine;
        return;

      case 5:
        p_errMessage = "Invalid username " + theUser +
                       " for host " + theMachine;
        return;

      case 6:
        p_errMessage = "Timeout trying to connect to " + theMachine +
                       " with remote shell " + p_shell;
        return;

      case 7:
        p_errMessage = "Invalid syntax for remote shell command";
        return;

      case 8:
        // Allows yes/no questions of any type and just says "yes".
        // Should only see this for the man-in-the-middle attack warning
        if (!expwrite("yes")) return;
        done = false;
        break;
 
      case 9:
        if (thePass=="" &&
            !RCommand::getPassCache(p_shell, theMachine, theUser, thePass)) {
          //  Resolved against $ECCE_HOME/bin: the apps no longer run with
          //  their working directory set to bin, so the bare "./passdialog"
          //  this used to be found nothing (#134).
          passCmd = Ecce::ecceBinCommand("passdialog") + " password " +
                    theMachine + " " + theUser;
          if ((passPtr = popen(passCmd.c_str(), "r")) != NULL) {
            if (fgets(passBuf, sizeof(passBuf), passPtr) != NULL) {
              // strip off the trailing newline
              passBuf[strlen(passBuf)-1] = '\0';
              // handle password dialog cancel button
              if (strcmp(passBuf, "") == 0) {
                // close the pipe
                pclose(passPtr);
                return;
              }

              thePass = passBuf;
            } else {
              // close the pipe
              pclose(passPtr);
              return;
            }

            // close the pipe
            pclose(passPtr);
          } else
            return;
        }

        exp_elide(thePass.c_str());
        if (!expwrite(thePass)) return;
        done = false;
        break;

      case 10:
        passCmd = Ecce::ecceBinCommand("passdialog") + " passcode " +
                  theMachine + " " + theUser;
        if ((passPtr = popen(passCmd.c_str(), "r")) != NULL) {
          if (fgets(codeBuf, sizeof(codeBuf), passPtr) != NULL) {
            // strip off the trailing newline
            codeBuf[strlen(codeBuf)-1] = '\0';
            // handle password dialog cancel button
            if (strcmp(codeBuf, "") == 0) {
              // close the pipe
              pclose(passPtr);
              return;
            }

            exp_elide(codeBuf);
            if (!expwrite(codeBuf)) {
              // close the pipe
              pclose(passPtr);
              return;
            }
            done = false;
          } else {
            // close the pipe
            pclose(passPtr);
            return;
          }

          // close the pipe
          pclose(passPtr);
        } else
          return;
        break;

      case 12:
        if (login_prompt) {
          p_errMessage = "Invalid username " + theUser +
                         ", or password for host " + theMachine;
          return;
        }

        if (!expwrite(userName)) return;
        login_prompt = true;
        done = false;
        break;

      case 14:
        // Successful login recognized
        break;

      case EXP_TIMEOUT:
        p_errMessage = "Timeout running remote shell " + p_shell +
                       " for " + theMachine;
        return;

      default:
        p_errMessage =
          "Unrecognized authentication failure running remote shell " +
          p_shell + " for " + theMachine;
        return;
    }
  } while (!done);

  exp_elide(NULL);

  // Request a csh shell for ssh logins
  string cmd;
  if (p_shell=="ssh" && p_hopCount>0) {
    cmd = locShell + " -i";
    if (!expwrite(cmd)) return;
  }

  // Which dialect is actually listening on the other end of this
  // connection? Originally this whole login sequence assumed csh/tcsh
  // unconditionally -- fine as long as the remote account's login shell
  // actually is tcsh, broken (silently: setenv/if ($?VAR) is invalid bash
  // syntax) if it's bash instead, which is entirely outside ECCE's
  // control on a shared cluster account.
  //
  // An earlier version of this fix ran an active probe here (checking
  // specifically for a `tcsh` binary) -- wrong, and it caused exactly
  // this failure mode live: this box has plain `csh` (which the SSH
  // remote command above already launched successfully, confirmed by
  // reaching this point at all -- see shellCommand()'s "echo +hi+ && "
  // + locShell construction, matched via the "+hi+\r\n" pattern in the
  // login loop above) but no separate `tcsh` binary, so the probe
  // concluded "no tcsh, use bash" and sent bash syntax (PS1=...) to an
  // actual csh session, which doesn't understand it -- no "+go+" prompt
  // ever appeared, hanging expect1() below indefinitely.
  //
  // The actual fix needs no probe at all: reaching this point already
  // proves locShell (whatever shell RefMachine::shell() configured for
  // this machine -- "csh" by default) is genuinely present and working,
  // since shellCommand() already used that exact value to build the SSH
  // remote command, and we just matched its "+hi+" echo. So just check
  // locShell's own value directly -- it's already the single source of
  // truth for what's actually running, no separate detection needed.
  // Classify by basename (classifyShell), not an exact match against
  // the literal string "bash" -- a CONFIG naming "/usr/bin/bash" used
  // to fall through to csh syntax and fail the login outright. Anything
  // that isn't csh/tcsh/bash is refused rather than guessed at (see
  // classifyShell's comment).
  ShellDialect dialect = classifyShell(locShell);
  if (dialect == SHELL_UNSUPPORTED) {
    p_errMessage = "Unsupported local shell '" + locShell + "' for " +
                   theMachine + " -- ECCE needs csh, tcsh or bash";
    return;
  }
  bool useBash = (dialect == SHELL_BASH);
  p_remoteBash = useBash;

  // Login failure is caught by trying to set the prompt.
  // Can't parse for a successful login without the expwrite because I don't
  // know what the prompt might be if the user overrides the default "%" in
  // their .cshrc.
  // A login failure will be recognized after expect sees an EOF meaning
  // the shell has closed.
  // Buffer isn't flushed from previous write so the prompt may show up
  // on a line with other output instead of by itself as it should elsewhere.
  // By echoing out $prompt we should be able to work around this and
  // get reliable checks for good logins.
  if (useBash) {
    // bash's bracketed-paste mode (readline emitting \e[?2004h before
    // and \e[?2004l after every prompt) breaks every "\r\n+go+"-style
    // pattern match downstream: it inserts the escape sequence *between*
    // the \r\n and the prompt text, so the literal "\r\n+go+" adjacency
    // every match in this file assumes never actually appears in the
    // raw stream. Confirmed live, directly: a real connection with a
    // real password succeeded completely (confirmed via an
    // ECCE_RCOM_LOGMODE trace showing a correct "date" command result),
    // but exec("date") inside isOpen() still reported failure, and every
    // report of this looked identical to a wrong password from the
    // outside (RCommand's own generic "(incorrect password?)" fallback
    // message) -- unrelated to auth. Only reachable via RCommand's
    // "shellCommand()"-selected local-shell path (same-domain targets
    // get spawned as a plain "bash -f", not through ssh -v, which
    // doesn't hit this), so a purely-ssh-based repro never surfaced it.
    // Disabling it once, right after setting the prompt, keeps it off
    // for the rest of the session -- readline re-emits the escape
    // sequence around every future prompt otherwise, since it's a
    // per-prompt readline behavior, not a one-time startup message.
    //  "set +o emacs; set +o vi" turns readline off.  Otherwise a long
    //  command's echo comes back redrawn (wrapped with "\r", or
    //  horizontally scrolled with a leading "<"), so the exact-echo match
    //  eccejobstore waits on never arrives and monitoring hangs forever
    //  (#69 Bug 2; the bash side of #143).  The local "bash -f" spawn
    //  has readline on; --noediting only covers the direct ssh path.
    //  PROMPT_COMMAND: RHEL's /etc/bashrc prints an xterm title escape
    //  before every prompt, so "\r\n+go+" never matches (#200).
    if (!expwrite("unalias -a 2>/dev/null; PS1='+go+'; unset PROMPT_COMMAND; "
                  "bind 'set enable-bracketed-paste off' 2>/dev/null; "
                  "set +o emacs; set +o vi"))
      return;
  } else {
    //  "unset edit": where csh is tcsh (Ubuntu), its line editor wraps a
    //  long command's echo at 80 columns with " \b", so the exact-echo
    //  match that eccejobstore waits on never arrives and monitoring
    //  hangs forever (#143).  bsd-csh has no editor and ignores it.
    if (!expwrite("unalias precmd; set prompt=+go+; unset echo; unset edit"))
      return;
  }
  if (expect1("+go+$") != 1) {
    p_errMessage =
      "Unsuccessful remote shell login--invalid username or password";
    return;
  }

  // Set timeout back to normal
  exp_timeout = RC_EXEC_TIMEOUT;

  if (!useBash) {
    if (!expwrite("unalias *")) return;
    if (expect1("\r\n+go+$") != 1) {
      p_errMessage = "Unsuccessful remote shell login--unalias * failed";
      return;
    }
  }

  // set $PATH
  if (shellPath != "") {
    if (useBash) {
      // Safe even if $PATH happens to be unset (prefix + trailing colon,
      // harmless) -- no need for tcsh's two-branch $?PATH existence check.
      cmd = "export PATH=\"" + shellPath + ":${PATH}\"";
      if (!expwrite(cmd)) return;
      if (expect1("\r\n+go+$") != 1) {
        p_errMessage = "Unsuccessful remote shell login--export PATH " +
                       shellPath + ":${PATH} failed";
        return;
      }
    } else {
      cmd = "if ($?PATH) setenv PATH \"" + shellPath + ":${PATH}\"";
      if (!expwrite(cmd)) return;
      if (expect1("\r\n+go+$") != 1) {
        p_errMessage = "Unsuccessful remote shell login--setenv PATH " +
                       shellPath + ":${PATH} failed";
        return;
      }
      cmd = "if ($?PATH == 0) setenv PATH \"" + shellPath + "\"";
      if (!expwrite(cmd)) return;
      if (expect1("\r\n+go+$") != 1) {
        p_errMessage = "Unsuccessful remote shell login--setenv PATH " +
                       shellPath + " failed";
        return;
      }
    }
  }

  // set $LD_LIBRARY_PATH
  if (libPath != "") {
    if (useBash) {
      cmd = "export LD_LIBRARY_PATH=\"" + libPath + ":${LD_LIBRARY_PATH}\"";
      if (!expwrite(cmd)) return;
      if (expect1("\r\n+go+$") != 1) {
        p_errMessage =
          "Unsuccessful remote shell login--export LD_LIBRARY_PATH "+
          libPath + ":${LD_LIBRARY_PATH} failed";
        return;
      }
    } else {
      cmd = "if ($?LD_LIBRARY_PATH) setenv LD_LIBRARY_PATH \"" +
            libPath + ":${LD_LIBRARY_PATH}\"";
      if (!expwrite(cmd)) return;
      if (expect1("\r\n+go+$") != 1) {
        p_errMessage = "Unsuccessful remote shell login--setenv LD_LIBRARY_PATH "+
                       libPath + ":${LD_LIBRARY_PATH} failed";
        return;
      }
      cmd = "if ($?LD_LIBRARY_PATH == 0) setenv LD_LIBRARY_PATH \"" +
                       libPath + "\"";
      if (!expwrite(cmd)) return;
      if (expect1("\r\n+go+$") != 1) {
        p_errMessage = "Unsuccessful remote shell login--setenv LD_LIBRARY_PATH "+
                       libPath + " failed";
        return;
      }
    }
  }

  // source file, if specified
  if (sourceFile != "") {
    cmd = useBash ?
      ("[ -e " + sourceFile + " ] && source " + sourceFile) :
      ("if (-e " + sourceFile + ") source " + sourceFile);
    if (!expwrite(cmd)) return;
    if (expect1("\r\n+go+$") != 1) {
      p_errMessage = "Unsuccessful remote shell login--source " +
                     sourceFile + " failed";
      return;
    }
  }

  // it appears this was a successful connection so cache the user entered
  // password if applicable
  if (thePass != "")
    RCommand::setPassCache(p_shell, theMachine, theUser, thePass);

  if (!iHop && p_hopCount>0) {
    iHop = true;

    // now connect to the actual destination from the front-end
    theMachine = p_machine;

    // assume that the final machine uses the same remote shell as the
    // front-end machine and that the user is also the same (not specified)
    cmd = p_shell;

    // ssh only needs the -v flag to check authentication and -X tries to
    // ensure X11 port forwarding will work.  All the other should be
    // ignored as they have caused issues on mpp2 at least
    if (p_shell == "ssh")
      cmd += " -X -v";
    cmd += " " + theMachine;

    if (!expwrite(cmd)) return;
    goto hopToIt;
  }

  p_connected = true;
}


bool RCommand::hop(const string& hopMachine, const string& locShell,
                   const string& userName, const string& password,
                   const string& shellPath, const string& libPath,
                   const string& sourceFile)
{
  if (p_direct) {
    if (p_ssh) return sshHop(hopMachine, locShell, userName, password,
                             shellPath, libPath, sourceFile);
    return directUnsupported("hop");
  }

  p_hopCount++;

  bool done;
  string output;
  bool login_prompt = false;

  string theUser;
  if (userName == "")
    theUser = Ecce::realUser();
  else
    theUser = userName;

  string thePass = password;

  string notFoundStr = p_shell + ": Command not found";

  // assume that the hopMachine uses the same remote shell as the original
  // remote shell and that the user is also the same (not specified)
  string cmd = p_shell;
  // ssh only needs the -v flag to check authentication and all the other
  // options such as forwarding X11 should be ignored
  // (they cause issues for mpp2 at least)
  if (p_shell == "ssh")
    cmd += " -v";
  cmd += " " +  hopMachine;

  // passcode prompting variables
  string passCmd;
  FILE* passPtr;
  char passBuf[MAXLINE];
  passBuf[0] = '\0';
  char codeBuf[MAXLINE];

  if (!expwrite(cmd)) return false;

  do {
    done = true;  // be optimistic

    switch (exp_expectl(p_fid, exp_glob, notFoundStr.c_str(), 1,
                               exp_glob, "execvp(", 1,
                               exp_glob, "denied", 2,
                               exp_glob, "failed", 2,
                               exp_glob, " closed", 2,
                               exp_glob, "Connection refused", 3,
                               exp_glob, "Bad host name", 4,
                               exp_glob, "Unknown host", 4,
                               exp_glob, "incorrect", 5,
                               exp_glob, "Connection timed out", 6,
                               exp_glob, "^Usage:", 7,
                               exp_glob, "\r\nUsage:", 7,
                               exp_glob, "(yes/no)? $", 8,
                               exp_glob, "password: $", 9,
                               exp_glob, "Password: $", 9,
                               exp_glob, "Password:$", 9,
                               exp_glob, "passphrase*: $", 9,
                               exp_glob, "PASSCODE:$", 10,
                               exp_glob, "PASSCODE: $", 10,
                               exp_glob, "login: $", 12,
                               exp_glob, "Authentication succeeded", 14,
                               // Modern OpenSSH's actual -v output for a
                               // successful key-based (no password prompt)
                               // login is "Authenticated to <host> ...
                               // using \"publickey\"." -- not the literal
                               // "Authentication succeeded" text above,
                               // which this decades-old pattern list has
                               // apparently always expected. Confirmed via
                               // a direct `ssh -v` run against a real
                               // key-trusted host: this is genuinely what
                               // current OpenSSH prints, not a fluke.
                               // Without this, a key-authenticated
                               // connection is never recognized as
                               // successful and the loop times out --
                               // reported live as "Failed to open remote
                               // shell ... (incorrect password?)" against
                               // a machine that never even prompted for
                               // one. Password-based logins were already
                               // fine (they complete via the "+hi+\r\n"
                               // echo marker below, once the shell after a
                               // successful password entry is reached).
                               exp_glob, "Authenticated to*", 14,
                               exp_glob, "+hi+\r\n", 14,
                               exp_end)) {

      case 1:
        p_errMessage = "Unable to find remote shell " + p_shell +
                       " (not in the path?)";
        return false;

      case 2:
      case EXP_EOF:
        p_errMessage = "Permission to run remote shell " + p_shell +
                       " denied for " + hopMachine;
        if (p_shell == "rsh")
          p_errMessage +=
                       " (do you have a .rhosts entry on " + hopMachine + "?)";
        else if (p_shell=="ssh" && password=="")
          p_errMessage = "No password configured for " + hopMachine +
                       " (did you set a new passphrase without reconfiguring?)";
        else if (p_shell == "ssh")
          p_errMessage += " (incorrect password?)";
      return false;

      case 3:
        p_errMessage = "Shell authentication server for " + p_shell +
                       " not running or installed on " + hopMachine;
        return false;

      case 4:
        p_errMessage = "Unknown or unavailable host " + hopMachine;
        return false;

      case 5:
        p_errMessage = "Invalid username " + theUser +
                       " for host " + hopMachine;
        return false;

      case 6:
        p_errMessage = "Timeout trying to connect to " + hopMachine +
                       " with remote shell " + p_shell;
        return false;

      case 7:
        p_errMessage = "Invalid syntax for remote shell command";
        return false;

      case 8:
        // Allows yes/no questions of any type and just says "yes".
        // Should only see this for the man-in-the-middle attack warning
        if (!expwrite("yes")) return false;
        done = false;
        break;
 
      case 9:
        if (thePass=="" &&
            !RCommand::getPassCache(p_shell, hopMachine, theUser, thePass)) {
          passCmd = Ecce::ecceBinCommand("passdialog") + " password " +
                    hopMachine + " " + theUser;
          if ((passPtr = popen(passCmd.c_str(), "r")) != NULL) {
            if (fgets(passBuf, sizeof(passBuf), passPtr) != NULL) {
              // strip off the trailing newline
              passBuf[strlen(passBuf)-1] = '\0';
              // handle password dialog cancel button
              if (strcmp(passBuf, "") == 0) {
                // close the pipe
                pclose(passPtr);
                return false;
              }

              thePass = passBuf;
            } else {
              // close the pipe
              pclose(passPtr);
              return false;
            }

            // close the pipe
            pclose(passPtr);
          } else
            return false;
        }

        exp_elide(thePass.c_str());
        if (!expwrite(thePass)) return false;
        done = false;
        break;

      case 10:
        passCmd = Ecce::ecceBinCommand("passdialog") + " passcode " +
                  hopMachine + " " + theUser;
        if ((passPtr = popen(passCmd.c_str(), "r")) != NULL) {
          if (fgets(codeBuf, sizeof(codeBuf), passPtr) != NULL) {
            // strip off the trailing newline
            codeBuf[strlen(codeBuf)-1] = '\0';
            // handle password dialog cancel button
            if (strcmp(codeBuf, "") == 0) {
              // close the pipe
              pclose(passPtr);
              return false;
            }

            exp_elide(codeBuf);
            if (!expwrite(codeBuf)) {
              // close the pipe
              pclose(passPtr);
              return false;
            }
            done = false;
          } else {
            // close the pipe
            pclose(passPtr);
            return false;
          }

          // close the pipe
          pclose(passPtr);
        } else
          return false;
        break;

      case 12:
        if (login_prompt) {
          p_errMessage = "Invalid username " + theUser +
                         ", or password for host " + p_machine;
          return false;
        }

        if (!expwrite(userName)) return false;
        login_prompt = true;
        done = false;
        break;

      case 14:
        // Successful login recognized
        break;

      case EXP_TIMEOUT:
        p_errMessage = "Timeout running remote shell " + p_shell +
                       " for " + p_machine;
        return false;

      default:
        p_errMessage =
          "Unrecognized authentication failure running remote shell " +
          p_shell + " for " + p_machine;
        return false;
    }
  } while (!done);

  exp_elide(NULL);

  // Request a csh shell for ssh logins
  if (p_shell == "ssh") {
    cmd = locShell + " -i";
    if (!expwrite(cmd)) return false;

    // locShell -i is a fresh interactive login shell on the hop
    // machine (tcsh, zsh, ksh93, ...) -- it can still be sourcing its
    // startup file and enabling its own raw-mode editor when we write
    // the init line next, and that editor's terminal setup can flush
    // (discard) whatever we already typed. Wait for it to actually be
    // reading before sending anything else, rather than racing it.
    if (!waitShellReady(p_fid)) {
      p_errMessage = "Timeout waiting for " + locShell +
                     " to start on " + hopMachine;
      return false;
    }
  }

  // Same dialect handling as the main constructor above (see its
  // comments for the full story of why this exists): reaching this
  // point already proves locShell is genuinely present and working on
  // the hop machine, since shellCommand()/the cmd above already used
  // that exact value and we're about to match its output. No separate
  // detection needed, just check locShell's own value directly.
  // Classify by basename (classifyShell), not an exact match against
  // the literal string "bash" -- see the main constructor above. This
  // is ECCE's own local-shell config for the hop machine (the shell it
  // will send further commands in), not the login shell "locShell -i"
  // above ran into -- that one can legitimately be zsh/mksh/ksh93 and
  // waitShellReady() above already handles it, but locShell itself is
  // still restricted to csh/tcsh/bash.
  ShellDialect dialect = classifyShell(locShell);
  if (dialect == SHELL_UNSUPPORTED) {
    p_errMessage = "Unsupported local shell '" + locShell + "' for " +
                   hopMachine + " -- ECCE needs csh, tcsh or bash";
    return false;
  }
  bool useBash = (dialect == SHELL_BASH);
  p_remoteBash = useBash;

  // Login failure is caught by trying to set the prompt.
  // Can't parse for a successful login without the expwrite because I don't
  // know what the prompt might be if the user overrides the default "%" in
  // their .cshrc.
  // A login failure will be recognized after expect sees an EOF meaning
  // the shell has closed.
  // Buffer isn't flushed from previous write so the prompt may show up
  // on a line with other output instead of by itself as it should elsewhere.
  // By echoing out $prompt we should be able to work around this and
  // get reliable checks for good logins.
  if (useBash) {
    // See the main constructor's identical setup line above for the
    // full story on why bracketed-paste mode needs disabling here too.
    //  See the matching "set +o emacs" note above (#69, #143): a hop's
    //  "bash -i" runs on the remote pty ssh allocates, readline on.
    if (!expwrite("unalias -a 2>/dev/null; PS1='+go+'; unset PROMPT_COMMAND; "
                  "bind 'set enable-bracketed-paste off' 2>/dev/null; "
                  "set +o emacs; set +o vi"))
      return false;
  } else {
    //  See the matching "unset edit" note above (#143).
    if (!expwrite("unalias precmd; set prompt=+go+; unset echo; unset edit"))
      return false;
  }
  if (expect1("+go+$") != 1) {
    p_errMessage =
      "Unsuccessful remote shell login--invalid username or password";
    return false;
  }

  // Set timeout back to normal
  exp_timeout = RC_EXEC_TIMEOUT;

  if (!useBash) {
    if (!expwrite("unalias *")) return false;
    if (expect1("\r\n+go+$") != 1) {
      p_errMessage = "Unsuccessful remote shell login--unalias * failed";
      return false;
    }
  }

  // set $PATH
  if (shellPath != "") {
    if (useBash) {
      cmd = "export PATH=\"" + shellPath + ":${PATH}\"";
      if (!expwrite(cmd)) return false;
      if (expect1("\r\n+go+$") != 1) {
        p_errMessage = "Unsuccessful remote shell login--export PATH " +
                       shellPath + ":${PATH} failed";
        return false;
      }
    } else {
      cmd = "if ($?PATH) setenv PATH \"" + shellPath + ":${PATH}\"";
      if (!expwrite(cmd)) return false;
      if (expect1("\r\n+go+$") != 1) {
        p_errMessage = "Unsuccessful remote shell login--setenv PATH " +
                       shellPath + ":${PATH} failed";
        return false;
      }
      cmd = "if ($?PATH == 0) setenv PATH \"" + shellPath + "\"";
      if (!expwrite(cmd)) return false;
      if (expect1("\r\n+go+$") != 1) {
        p_errMessage = "Unsuccessful remote shell login--setenv PATH " +
                       shellPath + " failed";
        return false;
      }
    }
  }

  // set $LD_LIBRARY_PATH
  if (libPath != "") {
    if (useBash) {
      cmd = "export LD_LIBRARY_PATH=\"" + libPath + ":${LD_LIBRARY_PATH}\"";
      if (!expwrite(cmd)) return false;
      if (expect1("\r\n+go+$") != 1) {
        p_errMessage =
          "Unsuccessful remote shell login--export LD_LIBRARY_PATH "+
          libPath + ":${LD_LIBRARY_PATH} failed";
        return false;
      }
    } else {
      cmd = "if ($?LD_LIBRARY_PATH) setenv LD_LIBRARY_PATH \"" +
            libPath + ":${LD_LIBRARY_PATH}\"";
      if (!expwrite(cmd)) return false;
      if (expect1("\r\n+go+$") != 1) {
        p_errMessage = "Unsuccessful remote shell login--setenv LD_LIBRARY_PATH "+
                       libPath + ":${LD_LIBRARY_PATH} failed";
        return false;
      }
      cmd = "if ($?LD_LIBRARY_PATH == 0) setenv LD_LIBRARY_PATH \"" +
                       libPath + "\"";
      if (!expwrite(cmd)) return false;
      if (expect1("\r\n+go+$") != 1) {
        p_errMessage = "Unsuccessful remote shell login--setenv LD_LIBRARY_PATH "+
                       libPath + " failed";
        return false;
      }
    }
  }

  // source file, if specified
  if (sourceFile != "") {
    cmd = useBash ?
      ("[ -e " + sourceFile + " ] && source " + sourceFile) :
      ("if (-e " + sourceFile + ") source " + sourceFile);
    if (!expwrite(cmd)) return false;
    if (expect1("\r\n+go+$") != 1) {
      p_errMessage = "Unsuccessful remote shell login--source " +
                     sourceFile + " failed";
      return false;
    }
  }

  // it appears this was a successful connection so cache the user entered
  // password if applicable
  if (thePass != "")
    RCommand::setPassCache(p_shell, hopMachine, theUser, thePass);

  return true;
}


// ---------- Destructors ------------
RCommand::~RCommand(void)
{ 
  if (p_direct) {
    stopStream();
    delete p_transport;
    return;
  }

  if (p_connected) {
hopToExit:
    // send the exit to the shell.  If it fails I really don't know what an
    // appropriate reaction would be so I ignore the return value and just
    // hope for the GOODBYE acknowledgement.  Note that if there are
    // running jobs when the destructor is called then we've only expressed
    // our intent to exit as soon as those jobs are done.
    (void)expwrite("exit; echo GOODBYE");
    (void)expect1("GOODBYE\r\n");

    if (p_shell=="ssh") {
      // Let the first exit finish cleanly before the one that logs out.
      sleep(1);
      (void)expwrite("exit");
      // If background commands were issued in this shell, then waiting for
      // the regular ssh "connection closed" message will result in the
      // exit hanging.  Use the ssh verbose mesage about "exit-status"
      // because this happens before it waits on background jobs.
      // (void)expect1("Connection to * closed.");
      if (p_background)
        (void)expect1("exit-status reply");
      else
        (void)expect1("Connection to * closed.");
    }

    // It is possible to be all hopped up!!  Actually, two hops should
    // be the maximum:  one from a front-end to the actual compute machine and
    // then from the compute machine to a compute node.  Regardless, for
    // each hop, an exit is necessary to close connection.
    if (p_hopCount > 0) {
      p_hopCount--;
      goto hopToExit;
    }

    // this first wait is for the remote connection to close in response to
    // the "exit" command above

    // rsh will not exit until background jobs (eg, xterm&) exit so this is
    // a hack doing a "WNOHANG" wait on an rsh with a background job.  It will
    // leave defunct processes hanging around but I didn't see a good solution.
    if (p_shell == "rsh")
      (void)wait3(NULL, WNOHANG, NULL);
    else if (p_background)
      (void)waitpid(p_pid, NULL, WNOHANG);
    else if (kill(p_pid, SIGTERM) == 0)
      // go ahead and kill the process because otherwise waitpid sometimes
      // does not return for 30+ seconds and there really is no reason to
      // be nice to the process once we know the remote connection is exitted.
      (void)waitpid(p_pid, NULL, 0);
  }

  // now close the file descriptor associated with the expect pty
  close(p_fid);
  (void)wait3(NULL, WNOHANG, NULL);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Execute a single command instead of creating a connection
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
bool RCommand::command(const string& command, string& output,
                       string& errMessage, const string& machine,
                       const string& remShell, const string& locShell,
                       const string& userName, const string& password,
                       const string& frontendMachine,
                       const string& frontendBypass)
{
  bool status = false;

  if (getenv("ECCE_RCOM_DEBUGGING"))
    exp_is_debugging = 1;
  else
    exp_is_debugging = 0;

  if (getenv("ECCE_RCOM_LOGMODE")) {
    exp_loguser = 1;
    cout << endl;
    cout << "Running remote command:" << endl;
    cout << "command (" << command << ")" << endl;
    cout << "machine (" << machine << ")" << endl;
    cout << "remote shell (" << remShell << ")" << endl;
    cout << "local shell (" << locShell << ")" << endl;
    cout << "user name (" << userName << ")" << endl;
    cout << "password is " << password.length() << " characters" << endl;
    if (frontendMachine != "") {
      cout << "frontend machine (" << frontendMachine << ")" << endl;
      if (frontendBypass != "")
        cout << "frontend bypass domain (" << frontendBypass << ")" << endl;
    }
  } else
    exp_loguser = 0;

  RCommand rcmd(machine, remShell, locShell, userName, password,
                frontendMachine, frontendBypass);
  if (rcmd.isOpen()) {
    // turn off the timeout because these may be xterms and the like that
    // the user doesn't want disappearing on them even after ECCE has been
    // closed
    exp_timeout = -1;

    status = rcmd.execout(command, output);
  }

  errMessage = rcmd.commError();

  return status;
}

bool RCommand::bgcommand(const string& command, string& errMessage,
                         const string& machine, const string& remShell,
                         const string& userName, const string& password)
{
  bool status = false;

  if (getenv("ECCE_RCOM_LOGMODE")) {
    cout << endl;
    cout << "Running remote background command:" << endl;
    cout << "command (" << command << ")" << endl;
    cout << "machine (" << machine << ")" << endl;
    cout << "remote shell (" << remShell << ")" << endl;
    cout << "user name (" << userName << ")" << endl;
    cout << "password is " << password.length() << " characters" << endl;
  }

  pid_t pid = fork();
  if (pid == -1) {
    errMessage = "Unable to fork remote command " + command +
                 " as a background job.";
    return false;
  } else if (pid > 0) {
    // parent side -- assume success and continue processing
    (void)waitpid(pid, NULL, 0);
    return true;
  }

  string authPipeName = AuthCache::pipeName();

  // this is the background command child process which is going
  // to fork again which dissociates the original parent from the
  // new grandchild.  This eliminates any defunct zombie processes
  // due to the parent not ever waiting on the child w/o this double
  // fork design.
  pid = fork();
  if (pid == -1) {
    errMessage = "Unable to double fork remote command " + command +
                 " as a background job.";
    return false;
  } else if (pid > 0) {
    // parent side -- send authentication cache bow out gracefully
    AuthCache::getCache().pipeOut(authPipeName);
    _exit(0);
  }

  // this is the background command grandchild process
  // setup and do the execvp which is a simple app that reinvokes
  // RCommand::command
  string app = "ecmd";
  //  Resolved against $ECCE_HOME/bin: the apps no longer run from there,
  //  and execvp does not search PATH for a name containing a slash, so
  //  "./ecmd" could only ever have worked from the bin directory (#134).
  string path = Ecce::ecceBinCommand(app);

  static const char* minbg = "-bg";
  static const char* minpipe = "-pipe";

  char* argv[9];
  argv[0] = strdup((char*)app.c_str());
  argv[1] = (char*)minpipe;
  argv[2] = strdup((char*)authPipeName.c_str());
  argv[3] = strdup((char*)remShell.c_str());
  argv[4] = strdup((char*)userName.c_str());
  argv[5] = strdup((char*)machine.c_str());
  argv[6] = strdup((char*)command.c_str());
  argv[7] = (char*)minbg;
  argv[8] = NULL;

  status = execvp(path.c_str(), argv) >= 0;

  // should never reach this code except if execvp fails
  if (!status)
    errMessage = "Unable to execute application " + path;

  return status;
}

string RCommand::commandShell(const string& machine, const string& remShell,
                              const string& userName)
{
  string theShell = remShell;

  if (!RCommand::isRemote(machine, remShell, userName) ||
      remShell=="" || remShell=="ssh" || remShell=="sshpass" ||
      remShell.find("ssh/")==0 || (remShell.find("/ssh")!=string::npos &&
       remShell.find("/ssh")==remShell.length()-4) ||
      remShell=="rsh" || remShell.find("rsh/")==0 ||
      (remShell.find("/rsh")!=string::npos &&
       remShell.find("/rsh")==remShell.length()-4)) {
    // empty -- done this way for speed of evaluation
  } else {
    string shellMatch = remShell + ":";

    string siteShellFile = Ecce::ecceHome();
    siteShellFile += "/siteconfig/remote_shells.site";

    if (access(siteShellFile.c_str(), F_OK) == 0) {
      ifstream is(siteShellFile.c_str());
      char buf[MAXLINE];
      char* tok;
      char* tokend;
      while (!is.eof()) {
        is.getline(buf, MAXLINE);
        if (buf[0]!='\0' && buf[0]!='#') {
          if (strncmp(buf, shellMatch.c_str(), shellMatch.length()) == 0) {
            tok = strchr(&buf[shellMatch.length()], '|');
            if (tok != NULL) {
              tok++;
              tok = strchr(tok, '|');
              if (tok != NULL)
                tok++;
            }
            if (tok == NULL) {
              tok = &buf[shellMatch.length()];
              tokend = strchr(tok, '|');
              if (tokend != NULL)
                *tokend = '\0';
            }

            tok = strtok(tok, " ");
            if (tok != NULL)
              theShell = tok;

            break;
          }
        }
      }
      is.close();
    }
  }

  string origShell = theShell;
  string::size_type slash = theShell.find('\\');
  if (slash != string::npos) {
    theShell.replace(0, slash+1, "");

    slash = theShell.find('\\');
    if (slash != string::npos)
      theShell.replace(0, slash+1, "");
    else {
      // go back to the first one if there is no third
      slash = origShell.find('\\');
      origShell.replace(slash, origShell.length()-slash, "");
      theShell = origShell;
    }
  }

  return theShell;
}

string RCommand::argsToCommand(const string& command, const string& args,
                               const string& remShell, const bool& isRemote,
                               string& commandWithArgs)
{
  string theShell = remShell;
  string shellArgs = "";

  commandWithArgs = "";

  if (!isRemote ||
      remShell=="" || remShell=="ssh" || remShell=="sshpass" ||
      remShell.find("ssh/")==0 ||
      remShell=="rsh" || remShell.find("rsh/")==0) {
    // empty -- done this way for speed of evaluation
  } else {
    string shellMatch = remShell + ":";

    string siteShellFile = Ecce::ecceHome();
    siteShellFile += "/siteconfig/remote_shells.site";

    if (access(siteShellFile.c_str(), F_OK) == 0) {
      ifstream is(siteShellFile.c_str());
      char buf[MAXLINE];
      char* tok;
      char* tokend;
      while (!is.eof()) {
        is.getline(buf, MAXLINE);
        if (buf[0]!='\0' && buf[0]!='#') {
          if (strncmp(buf, shellMatch.c_str(), shellMatch.length()) == 0) {
            tok = strchr(&buf[shellMatch.length()], '|');
            if (tok != NULL) {
              tok++;
              tok = strchr(tok, '|');
              if (tok != NULL)
                tok++;
            }
            if (tok == NULL) {
              tok = &buf[shellMatch.length()];
              tokend = strchr(tok, '|');
              if (tokend != NULL)
                *tokend = '\0';
            }

            tok = strtok(tok, " ");
            if (tok != NULL)
              theShell = tok;

            tok = strtok(NULL, "\0");
            if (tok != NULL) {
              shellArgs = " ";
              shellArgs += tok;
            }

            break;
          }
        }
      }
      is.close();
    }
  }

  string origShell = theShell;
  string::size_type slash = theShell.find('\\');
  if (slash != string::npos) {
    theShell.replace(0, slash+1, "");

    slash = theShell.find('\\');
    if (slash != string::npos)
      theShell.replace(0, slash+1, "");
    else {
      // go back to the first one if there is no third
      slash = origShell.find('\\');
      origShell.replace(slash, origShell.length()-slash, "");
      theShell = origShell;
    }
  }

  if (!isRemote ||
      theShell=="ssh" || theShell=="sshpass" ||
      theShell.find("ssh/")==0 || (theShell.find("/ssh")!=string::npos &&
       theShell.find("/ssh")==theShell.length()-4) ||
      theShell=="rsh" || theShell.find("rsh/")==0 ||
      (theShell.find("/rsh")!=string::npos &&
       theShell.find("/rsh")==theShell.length()-4))
    commandWithArgs = command;

  if (args != "") {
    if (commandWithArgs == "")
      commandWithArgs = args;
    else
      commandWithArgs += " " + args;
  }

  theShell += shellArgs;

  return theShell;
}

bool RCommand::command(const string& command, const string& args,
                       string& output, string& errMessage,
                       const string& machine, const string& remShell,
                       const string& locShell, const string& userName,
                       const string& password, const string& frontendMachine,
                       const string& frontendBypass)
{
  if (getenv("ECCE_RCOM_LOGMODE")) {
    cout << endl;
    cout << "Running remote command with args:" << endl;
    cout << "command (" << command << ")" << endl;
    cout << "args (" << args << ")" << endl;
    cout << "machine (" << machine << ")" << endl;
    cout << "remote shell (" << remShell << ")" << endl;
    cout << "local shell (" << locShell << ")" << endl;
    cout << "user name (" << userName << ")" << endl;
    cout << "password is " << password.length() << " characters" << endl;
    if (frontendMachine != "") {
      cout << "frontend machine (" << frontendMachine << ")" << endl;
      if (frontendBypass != "")
        cout << "frontend bypass domain (" << frontendBypass << ")" << endl;
    }
  }

  string theCommand;
  string theShell = RCommand::argsToCommand(command, args, remShell,
                  RCommand::isRemote(machine, remShell, userName), theCommand);

  return RCommand::command(theCommand, output, errMessage,
                           machine, theShell, locShell, userName, password);
}

bool RCommand::bgcommand(const string& command, const string& args,
                         string& errMessage,
                         const string& machine, const string& remShell,
                         const string& userName, const string& password)
{
  if (getenv("ECCE_RCOM_LOGMODE")) {
    cout << endl;
    cout << "Running remote background command with args:" << endl;
    cout << "command (" << command << ")" << endl;
    cout << "args (" << args << ")" << endl;
    cout << "machine (" << machine << ")" << endl;
    cout << "remote shell (" << remShell << ")" << endl;
    cout << "user name (" << userName << ")" << endl;
    cout << "password is " << password.length() << " characters" << endl;
  }

  string theCommand;
  string theShell = RCommand::argsToCommand(command, args, remShell,
                  RCommand::isRemote(machine, remShell, userName), theCommand);

  return RCommand::bgcommand(theCommand, errMessage, machine, theShell,
                             userName, password);
}

bool RCommand::userproxy(const string& proxyAuth, const string& machine,
               const string& userName, const string& password, string& errMessage)
{
  bool status = false;
  string proxy = proxyAuth;

  int idx = proxy.find("##user##");
  if (idx != string::npos)
    proxy.replace(idx, 8, userName);

  idx = proxy.find("##machine##");
  if (idx != string::npos)
    proxy.replace(idx, 11, machine);

  if (getenv("ECCE_RCOM_DEBUGGING"))
    exp_is_debugging = 1;
  else
    exp_is_debugging = 0;

  if (getenv("ECCE_RCOM_LOGMODE"))
    exp_loguser = 1;
  else
    exp_loguser = 0;

  // This should finish quickly
  exp_timeout = RC_EXEC_TIMEOUT;

  char* tokenify = strdup((char*)proxy.c_str());
  char* argv[MAXARGS];
  int argc = 1;

  argv[0] = strtok(tokenify, " ");
  while ((argv[argc++] = strtok(NULL, " ")) != NULL);
  argv[argc] = (char*)0;
  argc--;

  if (exp_loguser == 1) {
    cout << "proxy authentication command:" << endl;
    for (int it=0; it<argc; it++)
      cout << "arg " << it << ": " << argv[it] << endl;
    cout << "end proxy authentication command" << endl; 
  }

  int theFid;
  if ((theFid = exp_spawnv(argv[0], argv)) <= 0) {
    errMessage = "Unable to spawn command ";
    errMessage += argv[0];
    return false;
  }

  string output;
  bool done;
  do {
    done = true;  // be optimistic

    switch (exp_expectl(theFid, exp_glob, "Command not found", 1,
                                exp_glob, "execvp(", 1,
                                exp_glob, "Bad", 2,
                                exp_glob, "Wrong", 2,
                                exp_glob, "ERROR", 2,
                                exp_glob, "error", 2,
                                exp_glob, "incorrect", 2,
                                exp_glob, "password: $", 3,
                                exp_glob, "passphrase*: $", 3,
                                exp_glob, "Password:$", 3,
                                exp_glob, "Password: $", 3,
                                exp_end)) {
      case 1:
        errMessage = "Unable to find proxy authentication command";
        errMessage += " (not in the path?)";
        break;

      case 2:
        if (strlen(exp_buffer) > 2)
          exp_buffer[strlen(exp_buffer)-2] = '\0';
        output = (exp_buffer != NULL)? exp_buffer: "";
        errMessage = "Unsuccessful proxy authentication";
        if (output != "")
          errMessage += "\nError output: " + output;
        break;

      case 3:
        exp_elide(password.c_str());
        if (!fidwrite(theFid, password, errMessage)) break;

        done = false;
        break;

      case EXP_EOF:
        // Successful login
        status = true;
        break;

      case EXP_TIMEOUT:
        errMessage = "Timeout running proxy authentication";
        break;

      default:
        errMessage = "Unrecognized proxy authentication failure";
        break;
    }
  } while (!done);

  exp_elide(NULL);

  // Must wait for EOF before closing descriptor
  (void)wait(NULL);

  // Shouldn't complain even if spawned process has already been closed by EOF
  close(theFid);

  return status;
}


bool RCommand::fileOp(const string& op, const string& filename)
{
  if (!p_connected) return false;

  const string opone = op.substr(0, 1);

  if (!(opone=="e" || opone=="d" || opone=="w" || opone=="r" ||
        opone=="x" || opone=="o" || opone=="z"))
    return false;

  // Previous approaches here both had real, confirmed-live bugs:
  //   1. csh's `if (-x file) echo TRUE` is not valid bash -- bash parses
  //      `(...)` as a subshell, so `-x` is interpreted as an attempt to
  //      run a command literally named "-x". Needed dialect branching.
  //   2. A hand-rolled "echo TRUE"/"echo FALSE" + expect2() pattern
  //      match (two attempts) was never reliable: an unanchored pattern
  //      matched the command's own terminal echo (it contains the
  //      literal substring "echo TRUE"); anchoring that with "\r\n...$"
  //      fixed the false-positive but introduced a genuine, reproducible
  //      race under fast back-to-back calls (confirmed via 10 repeated
  //      live runs: correct results only when artificial delay --
  //      verbose logging -- was inserted between send and match,
  //      otherwise frequent 30s timeouts/wrong results) -- almost
  //      certainly the vendored 1990s Expect matcher not handling a
  //      "$"-anchored, end-of-buffer pattern correctly across output
  //      that arrives in more than one incremental read.
  //
  // Sidesteps both: `test`/`[` is a real external command
  // (/usr/bin/test, or a builtin with identical POSIX syntax/exit-status
  // semantics in bash, dash, AND csh/tcsh) -- no dialect branching
  // needed at all. And rather than reinvent pattern matching, this just
  // hands the test off to execout(), which already reliably detects
  // success/failure via $?/$status (the exact mechanism proven across
  // this whole session's real end-to-end job launches) -- its own
  // pattern ("CMDSTAT=0*\r\n+go+", no leading anchor) is naturally
  // robust to fragmented reads because "CMDSTAT=" only ever appears in
  // real output, never in the command's own echoed source text, so it
  // doesn't need the strict end-of-buffer anchor that made fileOp()'s
  // own pattern fragile.
  // "o" (csh's "owned by you") has no lowercase equivalent in POSIX
  // test -- bash/POSIX use capital -O for this.
  string bashOpone = (opone == "o") ? "O" : opone;
  string testTarget = (filename == "~") ? (filename + "/") : filename;
  string cmd = "test -" + bashOpone + " " + testTarget;

  string output;
  return execout(cmd, output, "", 0);
}


bool RCommand::exists(const string& filename)
{
  return fileOp("e", filename);
}


bool RCommand::directory(const string& filename)
{
  return fileOp("d", filename);
}


bool RCommand::writable(const string& filename)
{
  return fileOp("w", filename);
}


bool RCommand::executable(const string& filename)
{
  return fileOp("x", filename);
}


bool RCommand::cd(const string& directory)
{
  // Old comment here claimed "cd is interpreted by the C shell [so] it
  // doesn't work to append an echo $status to the end", and instead
  // hand-rolled success detection by literal-text-matching the command's
  // own echo in exp_buffer and checking whether anything followed it.
  // Confirmed live, directly, that this hand-rolled check is simply
  // broken under bash: a `cd /tmp` that provably succeeded (confirmed
  // via a follow-up `pwd` genuinely showing /tmp) was still reported as
  // a failure -- exp_buffer's post-match state after expect1()'s own
  // "\r\n+go+$" truncation doesn't leave the buffer in the shape this
  // code assumed. Same bug class as the old fileOp() -- reinventing
  // success detection instead of using the one mechanism (execout()'s
  // $?/$status check) already proven reliable throughout this whole
  // session's real job launches. `cd` is a shell builtin in bash, dash,
  // AND csh/tcsh alike, and all of them set $?/$status from it just
  // like any other command -- there's no actual C-shell-specific
  // limitation here. Running it via execout() (no subshell involved,
  // since this is one command line sent to the existing persistent
  // remote shell) changes that shell's real working directory exactly
  // as before, it just detects success correctly now.

  if (!p_connected) return false;

  // save a microsecond by calling fileOp directly
  if (!fileOp("d", directory)) {
    p_errMessage = "Directory " + directory + " does not exist";
    return false;
  }

  string output;
  if (p_direct) {
    // Nothing persists between commands, so remember where we are, as an
    // absolute path; the transport prepends the cd to every later command.
    if (!execout("cd -- " + directory + " && pwd", output)) {
      p_errMessage = "Unable to cd to " + directory;
      return false;
    }
    while (!output.empty() && (output[output.size()-1]=='\n' ||
                               output[output.size()-1]=='\r'))
      output.erase(output.size()-1);
    p_transport->setDir(output);
    return true;
  }

  if (!execout("cd " + directory, output)) {
    p_errMessage = "Unable to cd to " + directory;
    return false;
  }

  return true;
}

bool RCommand::which(const string& filename, string& path)
{
  bool ret = true;
  path = filename;

  if (path[0] != '/') {
    string pathvar;
    if (execout("echo $PATH", pathvar)) {
      // get rid of newline stuff expect appends--2 characters instead of 1
      pathvar.resize(pathvar.length()-2);

      char* pathstr = strdup(pathvar.c_str());
      char* tok;
      string trypath;
      ret = false;

      for (tok = strtok(pathstr, ":"); tok!=NULL && !ret;
           tok = strtok(NULL, ":")) {
        trypath = tok;
        trypath += "/" + filename;
        ret = executable(trypath);
      }

      if (ret)
        path = trypath;
      else
        path = "";

      free(pathstr);
    } else if (!executable(path)) {
      ret = false;
      path = "";
    }
  } else if (!executable(path)) {
    ret = false;
    path = "";
  }

  return ret;
}

// What the pty path hands back: every newline as CR LF, and the stray
// backspace and colour sequences expMungedOutputFix() removes.
static string ptyStyleOutput(const string& raw)
{
  string s;
  s.reserve(raw.size() + raw.size()/16);
  for (size_t i = 0; i < raw.size(); i++) {
    if (raw[i] == '\n')
      s += '\r';
    s += raw[i];
  }

  size_t pos;
  while (s.size() > 1 && (pos = s.find('\b', 1)) != string::npos)
    s.erase(pos-1, 2);
  while ((pos = s.find("\033[00m")) != string::npos)
    s.erase(pos, 5);
  while ((pos = s.find("\033[m")) != string::npos)
    s.erase(pos, 3);

  return s;
}

bool RCommand::directExecout(const string& command, string& output,
                             const string& errorMessage, const int& timeout)
{
  output = "";

  if (timeout != 0)
    exp_timeout = timeout;

  TransportResult r = p_transport->run(p_scriptPrefix + "exec 2>&1\n" +
                                       command + "\n",
                                       exp_timeout > 0 ? exp_timeout : -1);

  if (timeout > 0)
    exp_timeout = RC_EXEC_TIMEOUT;

  if (getenv("ECCE_RCOM_LOGMODE"))
    cout << "Direct command (" << command << ") in ("
         << p_transport->dir() << ") status " << r.status
         << (r.error.empty() ? "" : " " + r.error) << endl;

  bool status = false;

  if (r.timedOut) {
    p_errMessage = "Unexpected timeout executing command " + command;
  } else if (r.status < 0) {
    p_errMessage = "Unable to execute command " + command +
                   (r.error.empty() ? "" : ": " + r.error);
    if (p_ssh && r.error.find("connection") != string::npos)
      p_connected = false;
  } else if (r.status == 0) {
    status = true;
  } else {
    // The pty path only recognises a status that begins with 0, 1 or 2
    // (glob patterns on "CMDSTAT=").  Mirror that, including its message
    // for the rest.
    char lead = std::to_string(r.status)[0];
    if (lead=='1' || lead=='2')
      p_errMessage = errorMessage != "" ? errorMessage :
                     "Failed executing command " + command;
    else
      p_errMessage = "No status returned from executing command " + command;
  }

  output = ptyStyleOutput(r.out);
  return status;
}

bool RCommand::startStream(const string& command)
{
  if (!p_direct || !p_connected || p_stream.rfd >= 0) return false;
  string error;
  if (p_ssh) {
#ifdef ECCE_HAVE_LIBSSH
    int fd = -1;
    p_sshStream = static_cast<SshTransport*>(p_transport)->openStream(
                    p_scriptPrefix + command, fd, error);
    if (!p_sshStream) {
      p_errMessage = "Could not start " + command + ": " + error;
      return false;
    }
    p_stream.rfd = p_stream.wfd = fd;
    if (getenv("ECCE_RCOM_LOGMODE"))
      cout << "ssh stream (" << command << ") in (" << p_transport->dir()
           << ") on its own session, no pty" << endl;
    return true;
#else
    return false;
#endif
  }
  if (!static_cast<DirectTransport*>(p_transport)->openStream(
        command, p_stream, error)) {
    p_errMessage = "Could not start " + command + ": " + error;
    return false;
  }
  if (getenv("ECCE_RCOM_LOGMODE"))
    cout << "Direct stream (" << command << ") in (" << p_transport->dir()
         << ") pid " << p_stream.pid << " on pipes, no pty" << endl;
  return true;
}

void RCommand::stopStream(int graceMs)
{
  if (!p_direct) return;
#ifdef ECCE_HAVE_LIBSSH
  if (p_sshStream) {
    static_cast<SshTransport*>(p_transport)->closeStream(p_sshStream, graceMs);
    p_sshStream = 0;
    p_stream.rfd = p_stream.wfd = -1;
    if (getenv("ECCE_RCOM_LOGMODE")) cout << "ssh stream closed" << endl;
    return;
  }
#endif
  if (p_ssh || p_stream.pid <= 0) return;
  int st = static_cast<DirectTransport*>(p_transport)->closeStream(
             p_stream, graceMs);
  if (getenv("ECCE_RCOM_LOGMODE"))
    cout << "Direct stream closed, status " << st << endl;
}

bool RCommand::execout(const string& command, string& output,
                       const string& errorMessage, const int& timeout)
{
  if (!p_connected) return false;

  if (p_direct && command == "\003") {
#ifdef ECCE_HAVE_LIBSSH
    if (p_sshStream) {
      static_cast<SshTransport*>(p_transport)->interruptStream(p_sshStream);
      return true;
    }
#endif
    if (p_ssh || p_stream.pid <= 0) return false;
    static_cast<DirectTransport*>(p_transport)->interruptStream(p_stream);
    return true;
  }

  if (p_direct)
    return directExecout(command, output, errorMessage, timeout);

  bool status = false;
  string cmdstat = command;

  // $status is csh/tcsh's exit-status variable; bash/sh use $? instead --
  // $status is simply unset in bash, so this would silently always
  // produce an empty "CMDSTAT=" with no digit, matching none of the
  // patterns below and failing every single remote command with "No
  // status returned". Confirmed via a live standalone repro against a
  // real bash remote connection.
  cmdstat.append(p_remoteBash ? "; echo CMDSTAT=$?" : "; echo CMDSTAT=$status");

  if (timeout != 0)
    exp_timeout = timeout;

  // temporarily restore the full buffer flag to be able to recognize
  // when the output is more than can be handled
  exp_full_buffer = 1;

  if (!expwrite(cmdstat)) return false;
  switch (exp_expectl(p_fid, exp_glob, "CMDSTAT=0*\r\n+go+", 1,
                             exp_glob, "Command not found*\r\n+go+", 2,
                             exp_glob, "CMDSTAT=1*\r\n+go+", 3,
                             exp_glob, "CMDSTAT=2*\r\n+go+", 3,
                             exp_glob, "\r\n+go+", 4, exp_end)) {

    case -1:
      p_connected = false;
      p_errMessage = "Lost remote shell connection attempting to read output "
                     "of command " + command;
      break;

    case 1:
      status = true;
      *exp_match = '\0';
      break;

    case 2:
      p_errMessage = "Could not find command " + command;
      break;

    case 3:
      if (exp_buffer_end == exp_match_end)
        *exp_match = '\0';
      if (errorMessage != "")
        p_errMessage = errorMessage;
      else
        p_errMessage = "Failed executing command " + command;
      break;

    case 4:
      p_errMessage = "No status returned from executing command " + command;
      break;

    case EXP_FULLBUFFER:
      p_errMessage = "Command output buffer length exceeded "
                     "(partial results returned)";
      break;

    case EXP_EOF:
      p_errMessage = "Unexpected termination of remote shell executing command "
                     + command;
      break;

    case EXP_TIMEOUT:
      p_errMessage = "Unexpected timeout executing command " + command;
      break;

    default:
      p_errMessage = "Unexpected output executing command " + command;
  }

  // Some how, some way, expect introduces some bogus characters in the
  // output.  These include escape sequences and backspace characters.
  // These completely hose ecce processing and thus these characters need
  // to be stripped out.  This seems to happen on mpp2, and possibly only
  // mpp2 so maybe this fix can be conditionalized based on the machine.
  RCommand::expMungedOutputFix();

  // Matches whichever status-variable text was actually echoed back as
  // part of the command's own terminal echo (see cmdstat construction
  // above) -- "$status" (7 chars) for csh/tcsh, "$?" (2 chars) for bash.
  const char* statusVarEcho = p_remoteBash ? "$?" : "$status";
  int statusVarLen = p_remoteBash ? 2 : 7;
  string nlPattern = string(statusVarEcho) + "\r\n";
  string crPattern = string(statusVarEcho) + "\r";
  char* line = strstr(exp_buffer, nlPattern.c_str());

  if (line != NULL)
    output = line + statusVarLen + 2;
  else if ((line = strstr(exp_buffer, crPattern.c_str())) != NULL)
    // this fixes some weird problem that occured on a Dell Linux workstation
    output = line + statusVarLen + 3;
  else if (status)
    output = "";
  else
    output = exp_buffer;

  if (timeout > 0)
    exp_timeout = RC_EXEC_TIMEOUT;

  // clear the full buffer flag again
  exp_full_buffer = 0;

  return status;
}


void RCommand::expMungedOutputFix()
{
  // strip out backspace sequences (which includes the character right
  // before the backspace)
  if (strlen(exp_buffer) > 1) {
    char* bsptr = strchr(exp_buffer+1, 8);
    while (bsptr != NULL) {
      memmove(bsptr-1, bsptr+1, strlen(exp_buffer) - (bsptr - exp_buffer));
      bsptr = strchr(bsptr-1, 8);
    }
  }

  // strip out the longer escape sequence
  char findme[6];
  findme[0] = 27;
  findme[1] = 91;
  findme[2] = 48;
  findme[3] = 48;
  findme[4] = 109;
  findme[5] = '\0';
  char *eptr;
  while ((eptr = strstr(exp_buffer, findme)) != NULL) {
    memmove(eptr, eptr+5, strlen(exp_buffer) - (eptr - exp_buffer) - 4);
  }

  // strip out the shorter escape sequence
  findme[0] = 27;
  findme[1] = 91;
  findme[2] = 109;
  findme[3] = '\0';
  while ((eptr = strstr(exp_buffer, findme)) != NULL) {
    memmove(eptr, eptr+3, strlen(exp_buffer) - (eptr - exp_buffer) - 2);
  }
}


bool RCommand::exec(const string& command,
                    const string& errorMessage, const int& timeout)
{
  string output;
  return execout(command, output, errorMessage, timeout);
}


bool RCommand::execbg(const string& command, string& output,
                      const string& errorMessage)
{
  if (!p_connected) return false;

  if (p_direct) {
    string error;
    long pid = p_transport->spawnDetached(p_scriptPrefix + "nohup " + command,
                                          error);
    if (getenv("ECCE_RCOM_LOGMODE"))
      cout << "Direct background command (" << command << ") in ("
           << p_transport->dir() << ") pid " << pid << endl;
    if (pid < 0) {
      p_errMessage = errorMessage != "" ? errorMessage :
                     "Failed executing background command " + command;
      output = "";
      return false;
    }
    output = std::to_string(pid);
    p_background = true;
    return true;
  }

  bool status = false;

  // Old approach sent "command&; sleep 2" and hoped an asynchronous
  // job-control "Exit N" notification would arrive before the next
  // prompt, on the theory that 2 seconds was "a semi-reliable" window
  // (GDB's own 2001 comment, quoted in git history) -- and extracted
  // `output` by text-slicing everything after the command's own echoed
  // "; sleep 2" text. Confirmed live, directly, that this is broken
  // under bash run without job control -- every RCommand-spawned
  // "bash -i" session here reports "no job control in this shell" (see
  // every login trace this session), so there is no "Exit N" message to
  // ever catch, and the "; sleep 2" text-slicing produced outright
  // corrupted output: a real job submission's `output` came back as the
  // command's own echoed source text, e.g. "./submit__Calculation-5&;
  // sleep 2" -- which Launch.C's bgFlag job-id parsing (expecting a
  // "[1] 12345"-style job-control PID notification, per
  // siteconfig/QueueManagers' Shell manager) then accepted whole-cloth
  // as the "job id" (its own fallback for "no ']' found" is "use the
  // untouched string"), corrupting the eccejobmaster launch command
  // built from it and leaving the calculation stuck with no way to
  // reconnect job monitoring later.
  //
  // $! (PID of the most recently backgrounded job) is a POSIX shell
  // builtin available in bash, dash, AND csh/tcsh alike, with or
  // without job control -- unlike a job-control notification, it's
  // always populated the instant a command is backgrounded, so this
  // sidesteps the whole "wait and hope" heuristic entirely. Echoing it
  // immediately with an unambiguous marker (same reasoning as
  // fileOp()'s TRUE/FALSE redesign) reuses one reliable mechanism
  // instead of two fragile ones. Launch.C needs no changes: its
  // existing "no ']' found -> use the string as-is" fallback already
  // does exactly the right thing with a clean, bare PID.
  // nohup: without it, the backgrounded command is still a member of
  // this connection's own session -- if the connection closes (e.g.
  // RCommand's destructor runs once Launch::doLaunch() returns) while
  // the command is still running, it gets SIGHUP'd along with
  // everything else in that session. Confirmed live: a real, longer-
  // running job submitted this way died mid-computation with the
  // compute code's own crash log reporting "Error: hangup" -- neither
  // this call nor the generated submit script itself
  // (Launch::generateJobSubmissionFile()) had ever protected against
  // this, it just was never exercised long enough to matter until job
  // submission started reliably working today. Explicit redirect avoids
  // nohup's own default behavior of creating a stray nohup.out in the
  // run directory when stdout isn't already redirected -- this command
  // already handles its own output via generated shell-script
  // redirects, nothing here needs to see it.
  //  The redirection has to be written in the dialect of the shell that
  //  will parse it, and that shell is csh unless the machine is
  //  registered as using bash -- the same distinction p_remoteBash
  //  already makes for $status vs $? everywhere else in this file.
  //
  //  "> /dev/null 2>&1" is the sh form.  csh reads it as TWO output
  //  redirections -- "2" is an ordinary word, and ">&1" is csh's
  //  redirect-both operator -- and refuses the whole command with
  //  "Ambiguous output redirect."  Nothing is started, no PID comes
  //  back, and because execbg()'s caller treats a backgrounded launch
  //  as successful, ECCE reports the calculation as started.  That is
  //  the entire failure: files staged, job never runs, no error (#141).
  //
  //  It went unnoticed because the two csh implementations disagree.
  //  Debian's default csh is bsd-csh, which accepts the sh form; Ubuntu
  //  and RHEL ship tcsh as /usr/bin/csh, which rejects it.  So the same
  //  build launches jobs on one machine and silently fails on another,
  //  with "csh" installed and working on both.  Test against tcsh
  //  specifically, not whatever "csh" resolves to locally.
  //
  //  ">& /dev/null" is csh's own form and is what ECCE's generated
  //  submit scripts have always used.
  const string redirect = p_remoteBash ? " > /dev/null 2>&1" : " >& /dev/null";

  if (!expwrite("nohup " + command + redirect + " & echo RC_EXECBG_PID=$!"))
    return false;

  int matchResult = expect1("RC_EXECBG_PID=*\r\n+go+");
  switch (matchResult) {
    case 1:
      status = true;
      p_background = true;
      break;

    case EXP_TIMEOUT:
      p_errMessage = "Unexpected timeout of remote shell executing "
                     "background command " + command;
      break;

    case EXP_EOF:
      p_errMessage = "Unexpected termination of remote shell executing "
                     "background command " + command;
      break;

    default:
      if (errorMessage != "")
        p_errMessage = errorMessage;
      else
        p_errMessage = "Failed executing background command " + command;
  }

  RCommand::expMungedOutputFix();

  // Extract the marker's value BEFORE truncating exp_buffer at the
  // match -- unlike execout() (which searches for text that comes
  // *before* its own match point, so truncating there is harmless),
  // the PID text here is *inside* the matched region itself, so
  // truncating first would wipe out exactly the value being extracted.
  // Confirmed live: this was the actual cause of a first attempt at
  // this fix coming back with an empty output every time.
  //
  // Take the LAST occurrence of the marker, not the first: the buffer
  // contains it twice -- once in the terminal's own echo of the raw,
  // unexpanded command text we typed ("echo RC_EXECBG_PID=$!", literal
  // "$!"), and once in the shell's real, expanded output
  // ("RC_EXECBG_PID=12345"). strstr()'s first hit is always the echo,
  // confirmed live: an earlier version of this fix using the first
  // occurrence captured the literal string "$!" as the "PID" every
  // single run. Same underlying lesson as fileOp()'s original bug
  // (matching a command's own echo instead of its real output), just
  // solved by taking the last hit instead of anchoring on "\r\n".
  const char* marker = "RC_EXECBG_PID=";
  char* line = strstr(exp_buffer, marker);
  char* nextHit;
  while (line != NULL && (nextHit = strstr(line + 1, marker)) != NULL)
    line = nextHit;

  if (line != NULL) {
    output = line + strlen(marker);
    string::size_type pos = output.find_first_of("\r\n");
    if (pos != string::npos)
      output = output.substr(0, pos);
  } else if (status)
    output = "";
  else
    output = exp_buffer;

  return status;
}

bool RCommand::remoteShellIsBash(void) const
{
  return p_remoteBash;
}

bool RCommand::isOpen(void)
{
  if (p_direct)
    return p_connected;

  if (p_connected) {
    p_connected = exec("date");
    if (!p_connected)
      p_errMessage = "Opened remote shell on " + p_machine +
                     ", but unable to process new commands";
  } else if (p_errMessage.empty())
    // Only when open() itself set nothing specific (e.g. "Unsupported
    // local shell ...", #143/08716de) -- otherwise that message is more
    // useful than this generic guess and was getting overwritten.
    p_errMessage = "Failed to open remote shell on " + p_machine +
                   " (incorrect password?)";

  return p_connected;
}

string RCommand::commError(void)
{ return p_errMessage; }

bool RCommand::copy(string& errMessage,
                    const string& machine, const string& copyCmd,
                    const string& userName, const string& password,
                    char** argv)
{
  bool status = false;

  if (getenv("ECCE_RCOM_DEBUGGING"))
    exp_is_debugging = 1;
  else
    exp_is_debugging = 0;

  if (getenv("ECCE_RCOM_LOGMODE")) {
    exp_loguser = 1;
    cout << endl;
    cout << "Performing remote copy:" << endl;
    cout << "machine (" << machine << ")" << endl;
    cout << "copy command (" << copyCmd << ")" << endl;
    cout << "user name (" << userName << ")" << endl;
    cout << "password is " << password.length() << " characters" << endl <<endl;
  } else
    exp_loguser = 0;

  // Bump timeout way up because copies can take seemingly forever
  exp_timeout = RC_COPY_TIMEOUT;

  // Set the machine and user name variables for the benefit of error
  // messages (the values passed in are const)
  string theMachine = (machine=="" || machine=="-f" || machine=="system")?
                      RCommand::whereami(): machine;
  string theUser;
  if (userName == "")
    theUser = Ecce::realUser();
  else
    theUser = userName;

  string thePass = password;

  if (exp_loguser == 1) {
    cout << "remote copy command:" << endl;
    for (int it=0; argv[it]; it++)
      cout << "arg " << it << ": " << argv[it] << endl;
    cout << "end remote copy command" << endl; 
  }

  int theFid;
  if ((theFid = exp_spawnv((char*)copyCmd.c_str(), argv)) <= 0) {
    errMessage = "Unable to spawn copy command " + copyCmd;
    return false;
  }

  string notFoundStr = copyCmd + ": Command not found";

  // passcode prompting variables
  string passCmd;
  FILE* passPtr;
  char passBuf[MAXLINE];
  passBuf[0] = '\0';
  char codeBuf[MAXLINE];

  bool done;
  do {
    done = true;  // be optimistic

    int iexp = exp_expectl(theFid, exp_glob, notFoundStr.c_str(), 1,
                                exp_glob, "execvp(", 1,
                                exp_glob, "scp: warning: *\r\n", 2,
                                exp_glob, "cp: No match", 2,
                                exp_glob, "No match", 2,
                                exp_glob, ": No such file or directory", 2,
                                exp_glob, "cp: warning: * specified more than once",16,
                                exp_glob, "cp: *\r\n", 3,
                                exp_glob, "denied", 4,
                                exp_glob, "failed", 4,
                                exp_glob, " closed", 4,
                                exp_glob, "remote server failed", 4,
                                exp_glob, "rcmd: *\r\n", 4,
                                exp_glob, "ssh1: *\r\n", 4,
                                exp_glob, "ssh2: *\r\n", 4,
                                exp_glob, "Connection refused", 5,
                                exp_glob, "Bad host name", 6,
                                exp_glob, "Unknown host", 6,
                                exp_glob, "incorrect", 7,
                                exp_glob, "failed to store", 8,
                                exp_glob, "No space left", 8,
                                exp_glob, "disk space exceeded", 8,
                                exp_glob, "Connection timed out", 9,
                                exp_glob, "^Usage:", 10,
                                exp_glob, "\r\nUsage:", 10,
                                exp_glob, "failure", 11,
                                exp_glob, "alert", 11,
                                exp_glob, "(yes/no)? $", 12,
                                exp_glob, "password: $", 14,
                                exp_glob, "Password: $", 14,
                                exp_glob, "Password:$", 14,
                                exp_glob, "passphrase*: $", 14,
                                exp_glob, "PASSCODE:$", 15,
                                exp_glob, "PASSCODE: $", 15,
                                exp_end);
    switch (iexp) {
      case 1:
        errMessage = "Unable to find copy command " + copyCmd;
        if (copyCmd == "scp")
          errMessage += " (is scp in the path for " + theUser +
                        " on " + theMachine + "?)";
        break;

      case 2:
        // scp warnings should just be ignored rather than caught as errors
        // which would be done in the next case statement without this one
        // also used to ignore failures in wildcard matches
        done = false;
        break;

      case 3:
        exp_match[strlen(exp_match)-2] = '\0';
        errMessage = "Copy command " + copyCmd + " failed for " + theMachine +
                     ": " + &exp_match[4];
        break;

      case 4:
        errMessage = "Permission to run copy command " + copyCmd +
                     " denied for " + theMachine;
        if (copyCmd == "rcp")
          errMessage+=" (do you have a .rhosts entry on " + theMachine + "?)";
        else if (copyCmd=="scp" && thePass=="")
          errMessage = "No password configured for " + theMachine +
                       " (did you set a new passphrase without reconfiguring?)";
        else if (copyCmd == "scp")
          errMessage += " (incorrect password?)";
        break;

      case 5:
        errMessage = "Copy command authentication server for " + copyCmd +
                     " not running or installed on " + theMachine;
        break;

      case 6:
        errMessage = "Unknown or unavailable host " + theMachine;
        break;

      case 7:
        errMessage = "Invalid username " + theUser + " for host " +theMachine;
        break;

      case 8:
        errMessage = "Copy command " + copyCmd + " failed due to lack of "
                     "disk space under destination directory";
        break;

      case 9:
        errMessage = "Timeout trying to connect to " + theMachine +
                     " with remote copy command " + copyCmd;
        break;

      case 10:
        errMessage = "Invalid syntax for remote copy command";
        break;

      case 11:
        errMessage = "Remote copy authentication failure";
        break;

      case 12:
        // Allows a yes/no question of any type and just says "yes".
        // Should only see this for the man-in-the-middle attack warning on mpp1
        if (!fidwrite(theFid, "yes", errMessage)) break;
        done = false;
        break;

      case 14:
        if (thePass=="" &&
            !RCommand::getPassCache(copyCmd, theMachine, theUser, thePass)) {
          passCmd = Ecce::ecceBinCommand("passdialog") + " password " +
                    theMachine + " " + theUser;
          if ((passPtr = popen(passCmd.c_str(), "r")) != NULL) {
            if (fgets(passBuf, sizeof(passBuf), passPtr) != NULL) {
              // strip off the trailing newline
              passBuf[strlen(passBuf)-1] = '\0';
              // handle password dialog cancel button
              if (strcmp(passBuf, "") == 0) {
                // close the pipe
                pclose(passPtr);
                break;
              }

              thePass = passBuf;
            } else {
              // close the pipe
              pclose(passPtr);
              break;
            }

            // close the pipe
            pclose(passPtr);
          } else
            break;
        }

        exp_elide(thePass.c_str());
        if (!fidwrite(theFid, thePass, errMessage)) break;
        done = false;
        break;

      case 15:
        passCmd = Ecce::ecceBinCommand("passdialog") + " passcode " +
                  theMachine + " " + theUser;
        if ((passPtr = popen(passCmd.c_str(), "r")) != NULL) {
          if (fgets(codeBuf, sizeof(codeBuf), passPtr) != NULL) {
            // strip off the trailing newline
            codeBuf[strlen(codeBuf)-1] = '\0';
            // handle password dialog cancel button
            if (strcmp(codeBuf, "") == 0) {
              // close the pipe
              pclose(passPtr);
              break;
            }

            exp_elide(codeBuf);
            if (!fidwrite(theFid, codeBuf, errMessage)) {
              // close the pipe
              pclose(passPtr);
              break;
            }
            done = false;
          } else {
            // close the pipe
            pclose(passPtr);
            break;
          }

          // close the pipe
          pclose(passPtr);
        }
        break;
 
      case 16:
      case EXP_ABEOF:
        // Abnormal EOF comes across occasionally with copy commands
        // but it seems like it is only with successful copies
      case EXP_EOF:
        // Successful copy without password
        status = true;
        break;

      case EXP_TIMEOUT:
        errMessage = "Timeout running copy command " + copyCmd +
                     " for " + theMachine;
        break;

      default:
        errMessage = "Unrecognized authentication failure running copy "
                     "command " + copyCmd + " for " + theMachine + "\n";
        errMessage += exp_buffer;
        if (exp_loguser == 1)
          cout << "copy hit switch default with iexp " << iexp << endl;
    }
  } while (!done);

  exp_elide(NULL);

  // Set timeout back to normal.
  // Since the copy is a one shot operation this should only be significant
  // when the copy is done while there is also a separate open RCommand shell
  // connection which actually does happen in the launch process because
  // it keeps an ongoing shell connection
  exp_timeout = RC_EXEC_TIMEOUT;

  // Shouldn't complain even if spawned process has already been closed by EOF
  close(theFid);

  // it appears this was a successful connection so cache the user entered
  // password if applicable
  if (thePass != "")
    RCommand::setPassCache(copyCmd, theMachine, theUser, thePass);

  return status;
}


string RCommand::ftpTarget(const string& ftpIn)
{
#if 0000
  string ftpOut = " ";

  string::size_type slash = ftpIn.find_last_of('/');
  if (slash != string::npos)
    ftpOut += ftpIn.substr(slash+1);
  else
    ftpOut += ftpIn;
#else
  // wildcard ftp operations fail when the target has a wildcard, but
  // work fine with just the dot
  string ftpOut = " .";
#endif

  return ftpOut;
}


bool RCommand::ftp(string& errMessage, const string& machine,
                   const string& copyCmd, const string& userName,
                   const string& password, char** argv,
                   const char** fromFiles, const string& toFile,
                   const bool& putFlag)
{
  bool status = false;

  if (getenv("ECCE_RCOM_DEBUGGING"))
    exp_is_debugging = 1;
  else
    exp_is_debugging = 0;

  if (getenv("ECCE_RCOM_LOGMODE"))
    exp_loguser = 1;
  else
    exp_loguser = 0;

  // Bump timeout way up because copies can take seemingly forever
  exp_timeout = RC_COPY_TIMEOUT;

  // Set the machine and user name variables for the benefit of error
  // messages (the values passed in are const)
  string theMachine = (machine=="" || machine=="-f" || machine=="system")?
                      RCommand::whereami(): machine;

  string theUser;
  if (userName == "")
    theUser = Ecce::realUser();
  else
    theUser = userName;

  string thePass = password;

  if (exp_loguser == 1) {
    cout << "remote " << copyCmd << " command:" << endl;
    for (int it=0; argv[it]; it++)
      cout << "arg " << it << ": " << argv[it] << endl;
    cout << "end remote " << copyCmd << " command" << endl; 
  }

  int theFid;
  if ((theFid = exp_spawnv((char*)copyCmd.c_str(), argv)) <= 0) {
    errMessage = "Unable to spawn " + copyCmd + " command";
    return false;
  }

  string cdCmd = putFlag? "cd ": "lcd ";
  cdCmd += toFile;

  string mkdirCmdBase = putFlag? "mkdir ": "lmkdir ";
  string mkdirCmd;

  bool ftp_bye = true;
  bool done;
  string xfer_type = putFlag? "put ": "get ";
  int xfer_count = 0;
  bool cdFlag = false;
  bool mkdirFlag = false;
  string::size_type slash = toFile.find('/');

  // passcode prompting variables
  string passCmd;
  FILE* passPtr;
  char passBuf[MAXLINE];
  passBuf[0] = '\0';
  char codeBuf[MAXLINE];

  do {
    done = true;  // be optimistic

    switch (exp_expectl(theFid, exp_glob, "ftp: Command not found", 1,
                                exp_glob, "sftp: Command not found", 1,
                                exp_glob, "execvp(", 1,
                                exp_glob, "unknown host\r\nftp> $", 2,
                                exp_glob, "unknown host\r\nsftp> $", 2,
                                exp_glob, "(yes/no)? $", 3,
                                exp_glob, "Name *: $", 4,
                                exp_glob, "Password:$", 5,
                                exp_glob, "Password: $", 5,
                                exp_glob, "password: $", 5,
                                exp_glob, "Login failed.\r\nftp> $", 6,
                                exp_glob, "Login failed.\r\nsftp> $", 6,
                                exp_glob, "Permission denied", 6,
                                exp_glob, "logged in*\r\nftp> $", 7,
                                exp_glob, "logged in*\r\nsftp> $", 7,
                                exp_glob, "login ok*\r\nftp> $", 7,
                                exp_glob, "login ok*\r\nsftp> $", 7,
                                exp_glob, "Type set to *\r\nftp> $", 8,
                                exp_glob, "Type set to *\r\nsftp> $", 8,
                                exp_glob,
                                "No such file or directory*\r\nftp> $", 9,
                                exp_glob,
                                "No such file or directory*\r\nsftp> $", 9,
                                exp_glob, "does not exist.\r\nftp> $", 9,
                                exp_glob, "does not exist.\r\nsftp> $", 9,
                                exp_glob, "Couldn't create directory*\r\nsftp> $", 9,
                                exp_glob, "not found.*\r\nsftp> $", 9,
                                exp_glob, "Permission denied*\r\nftp> $", 10,
                                exp_glob, "Permission denied*\r\nsftp> $", 10,
                                exp_glob, "No space left*\r\nftp> $", 11,
                                exp_glob, "No space left*\r\nsftp> $", 11,
                                exp_glob, "disk space exceeded*\r\nftp> $", 11,
                                exp_glob, "disk space exceeded*\r\nsftp> $", 11,
                                exp_glob,
                                "CWD command successful.\r\nftp> $",12,
                                exp_glob,
                                "CWD command successful.\r\nsftp> $",12,
                                exp_glob, "Local directory now*\r\nftp> $",12,
                                exp_glob, "Local directory now*\r\nsftp> $",12,
                                exp_glob,
                                "Transfer complete.\r\n*\r\nftp> $", 14,
                                exp_glob,
                                "Transfer complete.\r\n*\r\nsftp> $", 14,
                                exp_glob, "100%*\r\nsftp> $", 14,
                                // this check for the prompt by itself
                                // needs to be after the check for 100%
                                // file transfer
                                exp_glob, "\r\nsftp> $", 8,
                                exp_glob, "PASSCODE:$", 15,
                                exp_glob, "PASSCODE: $", 15,
                                exp_end)) {
      case 1:
        errMessage = "Unable to find " + copyCmd + " command";
        ftp_bye = false;
        break;

      case 2:
        errMessage = "Unknown or unavailable host " + theMachine;
        break;

      case 3:
        // Allows yes/no questions of any type and just says "yes".
        // Should only see this for the man-in-the-middle attack warning
        if (!fidwrite(theFid, "yes", errMessage)) break;
        done = false;
        break;

      case 4:
        // should also work when userName isn't given since the default <cr>
        // response for ftp is the current user
        if (!fidwrite(theFid, userName, errMessage)) break;
        done = false;
        break;

      case 5:
        if (thePass=="" &&
            !RCommand::getPassCache(copyCmd, theMachine, theUser, thePass)) {
          passCmd = Ecce::ecceBinCommand("passdialog") + " password " +
                    theMachine + " " + theUser;
          if ((passPtr = popen(passCmd.c_str(), "r")) != NULL) {
            if (fgets(passBuf, sizeof(passBuf), passPtr) != NULL) {
              // strip off the trailing newline
              passBuf[strlen(passBuf)-1] = '\0';
              // handle password dialog cancel button
              if (strcmp(passBuf, "") == 0) {
                // close the pipe
                pclose(passPtr);
                break;
              }

              thePass = passBuf;
            } else {
              // close the pipe
              pclose(passPtr);
              break;
            }

            // close the pipe
            pclose(passPtr);
          } else
            break;
        }

        exp_elide(thePass.c_str());
        if (!fidwrite(theFid, thePass, errMessage)) break;
        done = false;
        break;

      case 6:
        if (thePass == "")
          errMessage = "No password configured for " + theMachine +
                       " (did you set a new passphrase without reconfiguring?)";
        else
          errMessage = "Invalid username " + theUser +
                       " or password for host " + theMachine;
        break;

      case 7:
        if (!fidwrite(theFid, "bin", errMessage)) break;
        done = false;
        break;

      case 8:
        if (!mkdirFlag) {
          slash = toFile.find('/', slash+1);
          if (slash != string::npos) {
            mkdirCmd = mkdirCmdBase + toFile.substr(0, slash);
            if (!fidwrite(theFid, mkdirCmd, errMessage)) break;
          } else {
            mkdirFlag = true;
            mkdirCmd = mkdirCmdBase + toFile;
            if (!fidwrite(theFid, mkdirCmd, errMessage)) break;
          }
        } else if (!cdFlag) {
          cdFlag = true;
          if (!fidwrite(theFid, cdCmd, errMessage)) break;
        } else {
          if (!fidwrite(theFid, xfer_type + fromFiles[0] +
                        RCommand::ftpTarget(fromFiles[0]), errMessage)) break;
        }
        done = false;
        break;

      case 9:
        if (!mkdirFlag) {
          slash = toFile.find('/', slash+1);
          if (slash != string::npos) {
            mkdirCmd = mkdirCmdBase + toFile.substr(0, slash);
            if (!fidwrite(theFid, mkdirCmd, errMessage)) break;
          } else {
            mkdirFlag = true;
            mkdirCmd = mkdirCmdBase + toFile;
            if (!fidwrite(theFid, mkdirCmd, errMessage)) break;
          }
          done = false;
        } else if (!cdFlag) {
          cdFlag = true;
          if (!fidwrite(theFid, cdCmd, errMessage)) break;
          done = false;
        } else {
          errMessage = copyCmd + " failed due to nonexistent source files or "
                     "destination directory";
        }
        break;

      case 10:
        errMessage = copyCmd +" failed due to lack of read or write permission";
        break;

      case 11:
        errMessage = copyCmd + " failed due to lack of disk space under " +
                     toFile;
        if (putFlag)
          errMessage += " on " + theMachine;
        break;

      case 12:
        if (!fidwrite(theFid, xfer_type + fromFiles[0] +
                      RCommand::ftpTarget(fromFiles[0]), errMessage)) break;
        done = false;
        break;

      case 14:
        // Successful transfer of a single file
        xfer_count++;
        if (fromFiles[xfer_count] != NULL) {
          if (!fidwrite(theFid, xfer_type + fromFiles[xfer_count] +
               RCommand::ftpTarget(fromFiles[xfer_count]), errMessage)) break;
          done = false;
        }
        else
          status = true;
        break;

      case 15:
        passCmd = Ecce::ecceBinCommand("passdialog") + " passcode " +
                  theMachine + " " + theUser;
        if ((passPtr = popen(passCmd.c_str(), "r")) != NULL) {
          if (fgets(codeBuf, sizeof(codeBuf), passPtr) != NULL) {
            // strip off the trailing newline
            codeBuf[strlen(codeBuf)-1] = '\0';
            // handle password dialog cancel button
            if (strcmp(codeBuf, "") == 0) {
              // close the pipe
              pclose(passPtr);
              break;
            }

            exp_elide(codeBuf);
            if (!fidwrite(theFid, codeBuf, errMessage)) {
              // close the pipe
              pclose(passPtr);
              break;
            }
            done = false;
          } else {
            // close the pipe
            pclose(passPtr);
            break;
          }

          // close the pipe
          pclose(passPtr);
        }
        break;

      case EXP_TIMEOUT:
        errMessage = "Timeout running " + copyCmd + " for " + theMachine;
        break;

      case EXP_EOF:
      default:
        errMessage = "Unrecognized " + copyCmd + " failure for " + theMachine;
    }
  } while (!done);

  exp_elide(NULL);

  // Set timeout back to normal.
  // Since the ftp/sftp is a one shot operation this should only be significant
  // when the ftp/sftp is done while there is also a separate open RCommand
  // shell connection which actually does happen in the launch process because
  // it keeps an ongoing shell connection
  exp_timeout = RC_EXEC_TIMEOUT;

  if (ftp_bye && !fidwrite(theFid, "bye", errMessage))
    status = false;

  // Shouldn't complain even if spawned process has already been closed by EOF
  close(theFid);

  // it appears this was a successful connection so cache the user entered
  // password if applicable
  if (thePass != "")
    RCommand::setPassCache(copyCmd, theMachine, theUser, thePass);

  return status;
}


bool RCommand::globFiles(const char** inFiles, char**& outFiles, int& numFiles)
{
  if (inFiles[0] == NULL)
    return false;

  glob_t globbedFiles;
  glob(inFiles[0], 0, NULL, &globbedFiles);
  int it;
  for (it=1; inFiles[it]!=NULL; it++)
    glob(inFiles[it], GLOB_APPEND, NULL, &globbedFiles);

  outFiles = (char **)malloc((globbedFiles.gl_pathc+1) * sizeof(char*));
  for (it=0; it<globbedFiles.gl_pathc; it++)
    outFiles[it] = strdup(globbedFiles.gl_pathv[it]);

  outFiles[globbedFiles.gl_pathc] = NULL;

  numFiles = globbedFiles.gl_pathc;

  globfree(&globbedFiles);

  return true;
}


#ifdef ECCE_HAVE_LIBSSH
// The machines the ssh transport serves; the same test as the constructor's.
static bool useSshCopy(const string& machine, const string& remShell,
                       const string& userName)
{
  const char* mode = getenv("ECCE_TRANSPORT");
  return mode && string(mode)=="ssh" &&
         RCommand::isRemote(machine, remShell, userName) &&
         (remShell=="" || remShell=="ssh" || remShell=="sshpass" ||
          remShell.find("ssh/")==0);
}

// scp -r over SFTP, with the pty path's leniency: copy() ignores "No such
// file or directory", so a missing source or a several-files-to-a-file
// target is skipped with a warning and the call still succeeds, which is what
// callers were written against.  Remote paths are relative to the login
// directory, local wildcards are expanded here and remote ones by the remote
// shell.
static void copyWarn(const string& what)
{
  if (getenv("ECCE_RCOM_LOGMODE")) cout << "copy warning: " << what << endl;
}

bool RCommand::sshCopy(bool putFlag, const string& machine,
                       const string& remShell, const string& userName,
                       const string& password, const vector<string>& files,
                       const string& toFile, string& errMessage)
{
  RCommand rc(machine, remShell, "csh", userName, password);
  if (!rc.isOpen()) {
    errMessage = rc.commError();
    return false;
  }
  SshTransport* t = static_cast<SshTransport*>(rc.p_transport);
  string err;
  vector<string> src;

  if (putFlag) {
    vector<const char*> in;
    for (size_t i = 0; i < files.size(); i++) in.push_back(files[i].c_str());
    in.push_back(NULL);
    char** globbed;
    int num;
    if (!RCommand::globFiles(&in[0], globbed, num)) return false;
    for (int i = 0; i < num; i++) { src.push_back(globbed[i]); free(globbed[i]); }
    free(globbed);
    for (size_t i = 0; i < files.size(); i++)
      if (access(files[i].c_str(), F_OK) != 0 &&
          files[i].find_first_of("*?[") == string::npos)
        copyWarn(files[i] + ": No such file or directory");
    if (src.empty()) return true;
    if (src.size() > 1) {
      // scp creates a missing target directory here and skips an existing file.
      int kind = t->remoteKind(toFile);
      if (kind == 0) {
        copyWarn(toFile + ": Not a directory");
        return true;
      }
      if (kind < 0 && t->run("mkdir -- " + shQuote(toFile), 30).status != 0) {
        errMessage = "cannot create " + toFile;
        return false;
      }
    }
    for (size_t i = 0; i < src.size(); i++)
      if (!t->putTree(src[i], toFile, err)) { errMessage = err; return false; }
    return true;
  }

  for (size_t i = 0; i < files.size(); i++) {
    vector<string> one;
    if (!t->remoteGlob(files[i], one, err)) {
      if (err == files[i] + ": No such file or directory" ||
          t->remoteKind(files[i]) < 0) {
        copyWarn(err);
        continue;
      }
      errMessage = err;
      return false;
    }
    for (size_t k = 0; k < one.size(); k++) {
      if (t->remoteKind(one[k]) < 0) {
        copyWarn(one[k] + ": No such file or directory");
        continue;
      }
      src.push_back(one[k]);
    }
  }
  if (src.empty()) return true;
  struct stat sb;
  if (src.size() > 1 && !(stat(toFile.c_str(), &sb)==0 && S_ISDIR(sb.st_mode))) {
    copyWarn(toFile + ": Not a directory");
    return true;
  }
  for (size_t i = 0; i < src.size(); i++)
    if (!t->getTree(src[i], toFile, err)) { errMessage = err; return false; }
  return true;
}
#endif

bool RCommand::get(string& errMessage,
                   const string& machine, const string& remShell,
                   const string& userName, const string& password,
                   int numFiles, ...)
{
  string toFile;
  char** fromFileStrs;
  int it;

  if (numFiles < 2)
    return false;

  va_list ap;
  va_start(ap, numFiles);

#ifdef ECCE_HAVE_LIBSSH
  if (useSshCopy(machine, remShell, userName)) {
    vector<string> fs;
    for (int k=0; k<numFiles-1; k++) fs.push_back(va_arg(ap, char*));
    string to = va_arg(ap, char*);
    va_end(ap);
    return RCommand::sshCopy(false, machine, remShell, userName, password,
                             fs, to, errMessage);
  }
#endif

  bool isRemote = RCommand::isRemote(machine, remShell, userName);

  if (isRemote) {
    string removed = RCommand::removedShellMessage(remShell);
    if (removed != "") {
      errMessage = removed;
      va_end(ap);
      return false;
    }
  }

  int argc = 0;
  char** argv = (char**)malloc((numFiles+MAXARGS) * sizeof(char*));

  string proxyAuth;
  string theCopy = RCommand::copyCommand(remShell, isRemote, machine,
                                         userName, proxyAuth, argc, argv);

  if (theCopy!="ftp" && theCopy!="sftp" && isRemote) {
    string fromFileBaseStr = "";
    if (userName!="")
      fromFileBaseStr = userName + "@";

    fromFileBaseStr += machine + ":";

    if (proxyAuth != "") {
      if (!RCommand::userproxy(proxyAuth, machine, userName,
                               password, errMessage))
        return false;
    }

    fromFileStrs = (char**)malloc(numFiles * sizeof(char*));
    fromFileStrs[numFiles-1] = NULL;
    int fromFileBaseLen = fromFileBaseStr.length() + 1;

    char *vaptr;
    for (it=0; it<numFiles; it++) {
      vaptr = (char*)va_arg(ap, char*);
      fromFileStrs[it] = (char*)malloc(fromFileBaseLen + strlen(vaptr));
      strcpy(fromFileStrs[it], fromFileBaseStr.c_str());
      strcat(fromFileStrs[it], vaptr);
    }

    toFile = (char*)va_arg(ap, char*);
    va_end(ap);

  } else if (!isRemote) {
    char **fromFiles = (char**)malloc(numFiles * sizeof(char*));
    for (it=0; it<numFiles-1; it++)
      fromFiles[it] = (char*)va_arg(ap, char*);
    fromFiles[numFiles-1] = NULL;

    toFile = (char*)va_arg(ap, char*);
    va_end(ap);

    int numGlob;
    if (RCommand::globFiles((const char**)fromFiles, fromFileStrs, numGlob))
      argv = (char**)realloc(argv, (numGlob+MAXARGS) * sizeof(char*));
    else
      return false;

    free(fromFiles);
  }

  bool status;
  if (theCopy=="ftp" || theCopy=="sftp") {
    fromFileStrs = (char**)malloc(numFiles * sizeof(char*));
    for (it=0; it<numFiles-1; it++)
      fromFileStrs[it] = strdup((char*)va_arg(ap, char*));
    fromFileStrs[numFiles-1] = NULL;

    toFile = (char*)va_arg(ap, char*);
    va_end(ap);

    status = RCommand::ftp(errMessage, machine, theCopy, userName, password,
                           argv, (const char**)fromFileStrs, toFile, false);
  }
  else {
    for (it=0; fromFileStrs[it]!=NULL; it++)
      argv[argc++] = (char*)fromFileStrs[it];

    argv[argc++] = strdup((char*)toFile.c_str());
    argv[argc] = (char*)0;

    status = RCommand::copy(errMessage, machine, theCopy, userName,
                            password, argv);
  }

  for (it=0; fromFileStrs[it]!=NULL; it++)
    free(fromFileStrs[it]);
  free(fromFileStrs);

  free(argv);

  return status;
}

/**
 * Method that uses vector to pass in filenames.
 * This method creates the data structures required by the original
 * methods and invokes them rather than re-implementing.
 */
bool RCommand::get(string& errMessage,
                   const string& machine, const string& remShell,
                   const string& userName, const string& password,
                   const vector<string>& fromFiles, const string& toFile)
{
   bool ret = false;
   int idx = 0;

   char **fileStrs = (char**)malloc((fromFiles.size()+1)*sizeof(char*));

   string fullFile;
   for (idx=0; idx<fromFiles.size(); idx++) {
      fileStrs[idx] = strdup((char*)fromFiles[idx].c_str());
   }

   // last char* must be NULL
   fileStrs[fromFiles.size()] = NULL;
   ret =  RCommand::get(errMessage, machine, remShell,
                        userName, password, (const char **)fileStrs, toFile);

   // Clean up memory
   for (idx=0; fileStrs[idx]!=NULL;  idx++) {
      free(fileStrs[idx]);
   }
   free((char*)fileStrs);

   return ret;
}

bool RCommand::get(string& errMessage,
                   const string& machine, const string& remShell,
                   const string& userName, const string& password,
                   const char** fromFiles, const string& toFile)
{
  char** fromFileStrs;
  int it;

  int numFiles;
  for (numFiles=0; fromFiles[numFiles]!=NULL; numFiles++);

#ifdef ECCE_HAVE_LIBSSH
  if (useSshCopy(machine, remShell, userName)) {
    vector<string> fs(fromFiles, fromFiles + numFiles);
    return RCommand::sshCopy(false, machine, remShell, userName, password,
                             fs, toFile, errMessage);
  }
#endif

  bool isRemote = RCommand::isRemote(machine, remShell, userName);

  if (isRemote) {
    string removed = RCommand::removedShellMessage(remShell);
    if (removed != "") {
      errMessage = removed;
      return false;
    }
  }

  int argc = 0;
  char** argv = (char**)malloc((numFiles+MAXARGS) * sizeof(char*));

  string proxyAuth;
  string theCopy = RCommand::copyCommand(remShell, isRemote, machine,
                                         userName, proxyAuth, argc, argv);

  if (theCopy!="ftp" && theCopy!="sftp" && isRemote) {
    string fromFileBaseStr = "";
    if (userName!="")
      fromFileBaseStr = userName + "@";

    fromFileBaseStr += machine + ":";

    if (proxyAuth != "") {
      if (!RCommand::userproxy(proxyAuth, machine, userName,
                               password, errMessage))
        return false;
    }

    fromFileStrs = (char**)malloc((numFiles+1) * sizeof(char*));
    fromFileStrs[numFiles] = NULL;
    int fromFileBaseLen = fromFileBaseStr.length() + 1;

    for (it=0; it<numFiles; it++) {
      fromFileStrs[it] = (char*)malloc(fromFileBaseLen + strlen(fromFiles[it]));
      strcpy(fromFileStrs[it], fromFileBaseStr.c_str());
      strcat(fromFileStrs[it], fromFiles[it]);
    }
  }
  else if (!isRemote) {
    int numGlob;
    if (RCommand::globFiles(fromFiles, fromFileStrs, numGlob))
      argv = (char**)realloc(argv, (numGlob+MAXARGS) * sizeof(char*));
    else
      return false;
  }

  bool status;
  if (theCopy=="ftp" || theCopy=="sftp")
    status = RCommand::ftp(errMessage, machine, theCopy, userName, password,
                            argv, fromFiles, toFile, false);
  else {
    for (it=0; fromFileStrs[it]!=NULL; it++)
      argv[argc++] = (char*)fromFileStrs[it];

    argv[argc++] = strdup((char*)toFile.c_str());
    argv[argc] = (char*)0;

    status = RCommand::copy(errMessage, machine, theCopy, userName, password,
                            argv);

    for (it=0; fromFileStrs[it]!=NULL; it++)
      free(fromFileStrs[it]);
    free(fromFileStrs);
  }

  free(argv);

  return status;
}


bool RCommand::put(string& errMessage,
                   const string& machine, const string& remShell,
                   const string& userName, const string& password,
                   int numFiles, ...)
{
  if (numFiles < 2)
    return false;

  int it;
  va_list ap;
  va_start(ap, numFiles);

#ifdef ECCE_HAVE_LIBSSH
  if (useSshCopy(machine, remShell, userName)) {
    vector<string> fs;
    for (int k=0; k<numFiles-1; k++) fs.push_back(va_arg(ap, char*));
    string to = va_arg(ap, char*);
    va_end(ap);
    return RCommand::sshCopy(true, machine, remShell, userName, password,
                             fs, to, errMessage);
  }
#endif

  char **fromFiles = (char**)malloc(numFiles * sizeof(char*));
  for (it=0; it<numFiles-1; it++)
    fromFiles[it] = (char*)va_arg(ap, char*);
  fromFiles[numFiles-1] = NULL;

  string toFile;
  bool isRemote = RCommand::isRemote(machine, remShell, userName);

  if (isRemote) {
    string removed = RCommand::removedShellMessage(remShell);
    if (removed != "") {
      errMessage = removed;
      va_end(ap);
      return false;
    }
  }

  int argc = 0;
  char** argv = (char**)malloc((numFiles+MAXARGS) * sizeof(char*));

  string proxyAuth;
  string theCopy = RCommand::copyCommand(remShell, isRemote, machine,
                                         userName, proxyAuth, argc, argv);

  if (theCopy=="ftp" || theCopy=="sftp" || theCopy=="cp")
    toFile = (char*)va_arg(ap, char*);
  else {
    toFile = "";
    if (userName!="")
      toFile = userName + "@";
    toFile += machine + ":" + (char*)va_arg(ap, char*);

    if (proxyAuth != "") {
      if (!RCommand::userproxy(proxyAuth, machine, userName,
                               password, errMessage))
        return false;
    }
  }

  va_end(ap);

  char** globbedFiles;
  int numGlob;
  if (RCommand::globFiles((const char**)fromFiles, globbedFiles, numGlob))
    argv = (char**)realloc(argv, (numGlob+MAXARGS) * sizeof(char*));
  else
    return false;

  bool status;

  if (theCopy=="ftp" || theCopy=="sftp")
    status  = RCommand::ftp(errMessage, machine, theCopy, userName, password,
                             argv, (const char**)globbedFiles, toFile, true);
  else {
    for (it=0; globbedFiles[it]!=NULL; it++)
      argv[argc++] = (char*)globbedFiles[it];

    argv[argc++] = strdup((char*)toFile.c_str());
    argv[argc] = (char*)0;

    status  = RCommand::copy(errMessage, machine, theCopy, userName, password,
                             argv);
  }

  free(fromFiles);

  for (it=0; globbedFiles[it]!=NULL; it++)
    free(globbedFiles[it]);
  free(globbedFiles);

  free(argv);

  return status;
}


bool RCommand::put(string& errMessage,
                   const string& machine, const string& remShell,
                   const string& userName, const string& password,
                   const char** fromFiles, const string& toFile)
{
  char **globbedFiles;

#ifdef ECCE_HAVE_LIBSSH
  if (useSshCopy(machine, remShell, userName)) {
    int n;
    for (n=0; fromFiles[n]!=NULL; n++);
    vector<string> fs(fromFiles, fromFiles + n);
    return RCommand::sshCopy(true, machine, remShell, userName, password,
                             fs, toFile, errMessage);
  }
#endif

  int numFiles;
  if (!RCommand::globFiles(fromFiles, globbedFiles, numFiles))
    return false;

  string fullToFile;
  bool isRemote = RCommand::isRemote(machine, remShell, userName);

  if (isRemote) {
    string removed = RCommand::removedShellMessage(remShell);
    if (removed != "") {
      errMessage = removed;
      return false;
    }
  }

  int argc = 0;
  char* argv[MAXARGS];

  string proxyAuth;
  string theCopy = RCommand::copyCommand(remShell, isRemote, machine,
                                         userName, proxyAuth, argc, argv);

  if (theCopy=="ftp" || theCopy=="sftp" || theCopy=="cp")
    fullToFile = toFile;
  else {
    fullToFile = "";
    if (userName!="")
      fullToFile = userName + "@";
    fullToFile += machine + ":" + toFile;

    if (proxyAuth != "") {
      if (!RCommand::userproxy(proxyAuth, machine, userName,
                               password, errMessage))
        return false;
    }
  }

  int it;
  bool status;
  if (theCopy=="ftp" || theCopy=="sftp")
    status = RCommand::ftp(errMessage, machine, theCopy, userName, password,
                           argv, (const char**)globbedFiles, fullToFile, true);
  else {
    char** globArgv = (char**)malloc((numFiles+MAXARGS) * sizeof(char*));

    for (it=0; it<argc; it++)
      globArgv[it] = argv[it];

    for (it=0; globbedFiles[it]!=NULL; it++)
      globArgv[argc++] = (char*)globbedFiles[it];

    globArgv[argc++] = strdup((char*)fullToFile.c_str());
    globArgv[argc] = (char*)0;

    status = RCommand::copy(errMessage, machine, theCopy, userName, password,
                            globArgv);

    free(globArgv);
  }

  for (it=0; globbedFiles[it]!=NULL; it++)
    free(globbedFiles[it]);
  free(globbedFiles);

  return status;
}


/**
 * Method that uses vector to pass in filenames.
 * This method creates the data structures required by the original
 * methods and invokes it rather than re-implementing.
 */
bool RCommand::put(string& errMessage,
                   const string& machine, const string& remShell,
                   const string& userName, const string& password,
                   const vector<string>& fromFiles, const string& toFile)
{
   bool ret = false;
   int idx = 0;

   char **fileStrs = (char**)malloc((fromFiles.size()+1)*sizeof(char*));

   string fullFile;
   for (idx=0; idx<fromFiles.size(); idx++) {
      fileStrs[idx] = strdup((char*)fromFiles[idx].c_str());
   }

   // last char* must be NULL
   fileStrs[fromFiles.size()] = NULL;
   ret =  RCommand::put(errMessage, machine, remShell,
                        userName, password, (const char **)fileStrs, toFile);

   // Clean up memory
   for (idx=0; fileStrs[idx]!=NULL;  idx++) {
      free(fileStrs[idx]);
   }
   free((char*)fileStrs);

   return ret;
}


// Copy one file byte for byte, the way the direct-mode shellput/shellget
// stand in for the pty's dd and cat.
static bool copyFileData(const string& from, const string& to)
{
  ifstream in(from.c_str(), std::ios::binary);
  if (!in) return false;
  ofstream out(to.c_str(), std::ios::binary | std::ios::trunc);
  if (!out) return false;
  out << in.rdbuf();
  out.flush();
  return (bool)out;
}

// A path as the "remote" side sees it: relative to the directory cd() set.
static string underDir(const string& dir, const string& path)
{
  if (dir.empty() || (!path.empty() && path[0]=='/'))
    return path;
  return dir + "/" + path;
}

// Direct mode copies locally; ssh mode goes over SFTP.  remote is already
// resolved against the directory cd() set.
bool RCommand::transferFile(bool putFlag, const string& local,
                            const string& remote)
{
#ifdef ECCE_HAVE_LIBSSH
  if (p_ssh) {
    string error;
    SshTransport* t = static_cast<SshTransport*>(p_transport);
    return putFlag ? t->put(local, remote, error) : t->get(remote, local, error);
  }
#endif
  return putFlag ? copyFileData(local, remote) : copyFileData(remote, local);
}

bool RCommand::shellput(const char** fromFiles, const string& toFile)
{
  string fullToFile;
  string cmdstat;
  char* baseFrom;
  bool status = false;
  char **globbedFiles;
  char buf[5000];
  int fid;
  struct stat statbuf;
  int iread, nread, xread, tread;
  int ig;

  int numFiles;
  if (!RCommand::globFiles(fromFiles, globbedFiles, numFiles))
    return false;

  if (!p_connected) return false;

  if (p_direct) {
    for (ig=0; globbedFiles[ig]!=NULL; ig++) {
      if ((baseFrom = strrchr(globbedFiles[ig], '/')) != NULL)
        fullToFile = toFile + baseFrom;
      else
        fullToFile = toFile + "/" + globbedFiles[ig];

      status = transferFile(true, globbedFiles[ig],
                            underDir(p_transport->dir(), fullToFile));
      if (!status) {
        p_errMessage = string("Failed executing put file script");
        break;
      }
    }
    for (ig=0; globbedFiles[ig]!=NULL; ig++)
      free(globbedFiles[ig]);
    free(globbedFiles);
    return status;
  }

  for (ig=0; globbedFiles[ig]!=NULL; ig++) {
    if ((baseFrom = strrchr(globbedFiles[ig], '/')) != NULL)
      fullToFile = toFile + baseFrom;
    else
      fullToFile = toFile + "/" + globbedFiles[ig];

    // open local input file
    if (access(globbedFiles[ig], F_OK) == 0) {
      // attempted to optimize the read/write in perl with the following
      // script.  But, it turned out to be nearly twice as slow as the
      // unoptimized line-by-line version so I'll just leave it here for
      // posterity.  Note that this version is so terse because there is
      // a 256 character limit for commands to C shell.  The STOP_XFER
      // write is not needed for the sysread/syswrite version because it
      // it reads based on the size of the file.

	//16 June 2015 -- on modern systems the C shell limit seems to be 64k
	//or higher

      //cmdstat = "perl -e 'open(TMP,\">" + fullToFile + "\");$n=" + nstr + ";$t=0;while ($t<$n){$i=sysread(STDIN,$s,$n-$t);$t=$t+$i;syswrite(TMP,$s,$i);}close(TMP);';echo CMDSTAT=$status";

      // this version proved to be unreliable, especially to mpp2.  I'm not
      // sure why, but it would lock up quite consistently when trying to
      // write the data in the file to stdout (the expect pty).  But, I'll
      // save this one for posterity too.  Note that this protocol uses
      // both a START_XFER and STOP_XFER message for synchronization rather
      // than a byte count.
      //cmdstat = "perl -e '$nl = chr(10); open(TMP, \">" + fullToFile + "\"); print \"START_XFER$nl\"; while ( <STDIN> ) { if ( /STOP_XFER/ ) { close(TMP); exit;} print TMP $_;}'; echo CMDSTAT=$status";

      fid = open(globbedFiles[ig], O_RDONLY);
      (void)fstat(fid, &statbuf);
      nread = statbuf.st_size;

      sprintf(buf, "%d", nread);
      string nreadstr = buf;

      cmdstat = "dd ibs=1 of=" + fullToFile + " count=" + nreadstr +
                (p_remoteBash ? "; echo CMDSTAT=$?" : "; echo CMDSTAT=$status");

      if (!expwrite(cmdstat)) return false;
      // scan until start of file to be transferred
      for (*buf='\0'; *buf!='\n'; read(p_fid, buf, 1));

      //the line below is for supporting the perl script protocol for
      //file transfer and not needed for the dd based implementation
      //(void)expect1("START_XFER");

      xread = nread - 4096;
      tread = 0;

#if 000
      // this allows stream i/o in case that proves more reliable
      FILE* fp = fdopen(p_fid, "r+");
      setbuf(fp, (char*)0);
#endif

      while (tread < xread) {
        iread = read(fid, buf, 4096);
        tread += iread;
#if 111
        if (write(p_fid, buf, iread) != iread) return false;
#else
        // stream i/o version
        buf[iread] = '\0';
        if (fputs(buf, fp) <= 0) return false;
#endif
      }

      while (tread < nread) {
        iread = read(fid, buf, nread-tread);
        tread += iread;
#if 111
        if (write(p_fid, buf, iread) != iread) return false;
#else
        // stream i/o version
        buf[iread] = '\0';
        if (fputs(buf, fp) <= 0) return false;
#endif
      }
      (void)close(fid);

      //the line below is for supporting the perl script protocol for
      //file transfer and not needed for the dd based implementation
      //if (!expwrite("STOP_XFER")) return false;

      switch (exp_expectl(p_fid, exp_glob, "CMDSTAT=0*\r\n+go+", 1,
                                 exp_glob, "Command not found*\r\n+go+", 2,
                                 exp_glob, "CMDSTAT=1*\r\n+go+", 3,
                                 exp_glob, "CMDSTAT=2*\r\n+go+", 3,
                                 exp_glob, "\r\n+go+", 4, exp_end)) {

        case -1:
          p_connected = false;
          p_errMessage = "Lost remote shell connection attempting to read "
                         "output of put file operation";
          break;

        case 1:
          status = true;
          break;

        case 2:
          p_errMessage = "Could not find put file script";
          break;

        case 3:
          p_errMessage = "Failed executing put file script";
          break;

        case 4:
          p_errMessage = "No status returned from put file script";
          break;

        case EXP_EOF:
          status = true;
          p_errMessage = "Unexpected termination of remote shell executing put file script";
          break;

        case EXP_TIMEOUT:
          p_errMessage = "Unexpected timeout executing put file script";
          break;

        default:
          p_errMessage = "Unexpected output executing put file script";
      }

      if (!status)
        return false;

    } else
      return false;
  }

  for (ig=0; globbedFiles[ig]!=NULL; ig++)
    free(globbedFiles[ig]);
  free(globbedFiles);

  return status;
}


/**
 * Method that uses vector to pass in filenames.
 * This method creates the data structures required by the original
 * methods and invokes them rather than re-implementing.
 */
bool RCommand::shellget(const vector<string>& fromFiles, const string& toFile)
{
   bool ret = false;
   int idx = 0;

   char **fileStrs = (char**)malloc((fromFiles.size()+1)*sizeof(char*));

   string fullFile;
   for (idx=0; idx<fromFiles.size(); idx++) {
      fileStrs[idx] = strdup((char*)fromFiles[idx].c_str());
   }

   // last char* must be NULL
   fileStrs[fromFiles.size()] = NULL;
   ret = shellget((const char **)fileStrs, toFile);

   // Clean up memory
   for (idx=0; fileStrs[idx]!=NULL;  idx++) {
      free(fileStrs[idx]);
   }
   free((char*)fileStrs);

   return ret;
}


bool RCommand::shellget(const char** fromFiles, const string& toFile)
{
  string fullToFile;
  string cmd;
  string countstr;
  string globbedFileStr;
  string globbedFile;
  char* baseFrom;
  bool status = false;
  char buf[5000];
  string wcstr;
  char* endptr;
  int lines, bytes;
  int nread, tread, iread, xread;
  char* bufptr;
  char* crptr;

  if (!p_connected) return false;

  for (int it=0; fromFiles[it]!=NULL; it++) {
    cmd = "ls ";
    cmd += fromFiles[it];
    if (execout(cmd, globbedFileStr)) {

      StringTokenizer next(globbedFileStr);
      while (!(globbedFile=next.next(" \t\r\n")).empty()) {

        char* gstr = (char*)globbedFile.c_str();
        if ((baseFrom = strrchr(gstr, '/')) != NULL)
          fullToFile = toFile + baseFrom;
        else
          fullToFile = toFile + "/" + globbedFile;

        if (p_direct) {
          status = transferFile(false, fullToFile,
                                underDir(p_transport->dir(), globbedFile));
          if (!status) {
            p_errMessage = "Failed executing cat command for get file "
                           "operation";
            return false;
          }
          continue;
        }

        cmd = "wc -lc " + globbedFile;
        execout(cmd, wcstr);
        lines = (int)strtol((char*)wcstr.c_str(), &endptr, 10);
        bytes = (int)strtol(endptr, NULL, 10);

        ofstream os(fullToFile.c_str());

        if (os) {
          cmd = "cat " + globbedFile +
                (p_remoteBash ? "; echo CMDSTAT=$?" : "; echo CMDSTAT=$status");
          if (!expwrite(cmd)) return false;

          // the commented out logic was failing because I found that
          // there were backspace characters sometimes being introduced
          // in the echo of the cat command.  Therefore, the only reliable
          // way to skip over this line is to search character by character
          // for a newline.  I left the old logic just because it is such a
          // mystery where these backspace characters are coming from.
          // nread = cmd.length() + 3;
          // tread = 0;
          // // read passed the cat command to the file data
          // while (tread < nread) {
          //   tread += read(p_fid, buf, nread-tread);
          // }
          for (*buf='\0'; *buf!='\n'; read(p_fid, buf, 1));

          nread = bytes + lines - 1;
          tread = 0;
          xread = nread - 4096;

          // get the bulk of the file in 4096 byte chunks max
          // so buf is guaranteed not to overflow
          while (tread < xread) {
            iread = read(p_fid, buf, 4096);
            tread += iread;
            buf[iread] = '\0';

            // strip carriage returns out of buf
            bufptr = buf;
            crptr = strchr(bufptr, 13);
            while (crptr != NULL) {
              *crptr = '\0';
              os << bufptr;      
              bufptr = crptr+1;
              crptr = strchr(bufptr, 13);
            }
            os << bufptr;      
          }

          // get the remainder of the file (<4096 bytes)
          while (tread < nread) {
            iread = read(p_fid, buf, nread-tread);
            tread += iread;
            buf[iread] = '\0';

            // strip carriage returns out of buf
            bufptr = buf;
            crptr = strchr(bufptr, 13);
            while (crptr != NULL) {
              *crptr = '\0';
              os << bufptr;      
              bufptr = crptr+1;
              crptr = strchr(bufptr, 13);
            }
            os << bufptr;      
          }

          os << endl;      
          os.close(); 
        }

        switch (exp_expectl(p_fid, exp_glob, "CMDSTAT=0*\r\n+go+", 1,
                                   exp_glob, "CMDSTAT=1*\r\n+go+", 2,
                                   exp_glob, "CMDSTAT=2*\r\n+go+", 2,
                                   exp_glob, "\r\n+go+", 3, exp_end)) {

          case -1:
            p_connected = false;
            p_errMessage = "Lost remote shell connection attempting to cat "
                           "file for get file operation";
            break;

          case 1:
            status = true;
            break;

          case 2:
            p_errMessage = "Failed executing cat command for get file operation";
            break;

          case 3:
            p_errMessage = "No status returned from executing cat command for "
                           "get file operation";
            break;

          case EXP_EOF:
            p_errMessage = "Unexpected termination of remote shell executing "
                           "cat command for get file operation";
            break;

          case EXP_TIMEOUT:
            p_errMessage = "Unexpected timeout executing cat command for"
                           "get file operation";
            break;

          default:
            p_errMessage = "Unexpected output executing cat command for "
                           "get file operatoin";
        }

        if (!status)
          return false;
      }
    }
  }

  return status;
}


const bool RCommand::getPassCache(const string& shell, const string& machine,
                                  const string& user, string& password)
{
  bool ret = false;

  string protocol = RCommand::copyToShell(shell);
  string url = protocol + "://" + machine;
  BasicAuth *ba = AuthCache::getCache().getAuthentication(url, user, "", 1);
  if (ba != NULL) {
    ret = true;
    password = ba->m_pass;
    delete ba;
  }

  return ret;
}


void RCommand::setPassCache(const string& shell, const string& machine,
                            const string& user, const string& password)
{
  // add to AuthCache so it is available to other apps
  string protocol = RCommand::copyToShell(shell);
  string url = protocol + "://" + machine;
  AuthCache::getCache().addAuthentication(url, user, password, "",true);
}

