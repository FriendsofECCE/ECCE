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
#include <set>
#include <memory>
  using std::map;

#include <stdlib.h> // getenv
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h> // va_start
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
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"

#include "tdat/AuthCache.H"

#include "comm/RCommand.H"

#include "comm/DirectTransport.H"
#include "comm/OpenSshTransport.H"
#include "comm/RemoteTransport.H"
#ifdef ECCE_HAVE_LIBSSH
#include "comm/SshTransport.H"
#endif

#define MAXARGS 32
/*#define MAXLINE 256*/
#define MAXLINE 16384

// A machine CONFIG's locShell can be a bare name or a full path
// ("/usr/bin/bash", "/bin/csh", ...). Classify by basename. ECCE only needs to
// source a machine's file in csh, tcsh or bash; anything else is refused
// rather than guessed at as csh (#143/#69).
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

  // -f and --norc --noprofile keep everything but the file itself out of
  // the diff.
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

  // The file may cd; later commands start where it left off.
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
  if (!Ecce::guiAvailable()) return false;
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

// Logs in over libssh: the password given, then AuthCache, then passdialog;
// passdialog's "passcode" for what looks like a second factor.
bool RCommand::sshConnectLibssh(const string& machine, const string& userName,
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

  adoptTransport(t);

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

#endif  // ECCE_HAVE_LIBSSH

// A new transport replaces the one this connection had.
void RCommand::adoptTransport(Transport* t)
{
  if (p_transport) {
    stopStream();
    delete p_transport;
  }
  p_transport = t;
  p_ssh = true;
  p_timeout = RC_EXEC_TIMEOUT;
  p_connected = true;
}

// The program that asks for a login while a shared connection is opened:
// ECCE_ASKPASS, else ECCE's own dialogs when there is a display to show them
// on; "" means never ask.
static string askpassProgram()
{
  const char* over = getenv("ECCE_ASKPASS");
  if (over && *over) return over;
  const char* home = getenv("ECCE_HOME");
  if (!home || !*home || !Ecce::guiAvailable()) return "";
  string path = string(home) + "/scripts/ecce-askpass";
  return access(path.c_str(), X_OK) == 0 ? path : "";
}

// The OpenSSH client as a subprocess: it reuses a connection the user's
// ssh configuration shares (ControlMaster), which libssh cannot, and it
// never prompts.
bool RCommand::sshConnectOpenssh(const string& machine, const string& userName,
                                 const string& jumpHost)
{
  OpenSshTransport* t = new OpenSshTransport(machine, 0, userName);
  t->setConnectTimeout(RC_CONNECT_TIMEOUT);
  if (jumpHost != "") t->setJumpHost(jumpHost, 0, userName);
  t->setAskpass(askpassProgram());
  string error;
  if (!t->connect(error)) {
    p_errMessage = error;
    delete t;
    return false;
  }
  adoptTransport(t);
  if (getenv("ECCE_RCOM_LOGMODE"))
    cout << "ssh transport: commands run over the OpenSSH client on " << machine
         << (jumpHost == "" ? "" : " through " + jumpHost + " (ssh -J)") << endl;
  return true;
}

bool RCommand::useOpenssh(const string& machine, const string& userName,
                          const string& jumpHost)
{
#ifdef ECCE_HAVE_LIBSSH
  OpenSshTransport::Backend b = OpenSshTransport::backendFor(machine, userName);
  // A front end whose own login is shared needs OpenSSH as well.
  if (b == OpenSshTransport::BACKEND_LIBSSH && jumpHost != "" &&
      OpenSshTransport::requestedBackend() == OpenSshTransport::BACKEND_AUTO &&
      OpenSshTransport::configSharesConnection(jumpHost, userName))
    b = OpenSshTransport::BACKEND_OPENSSH;
  return b == OpenSshTransport::BACKEND_OPENSSH;
#else
  (void)machine; (void)userName; (void)jumpHost;
  return true;
#endif
}

// Connects to an ssh machine with the backend that serves it.
bool RCommand::sshConnect(const string& machine, const string& userName,
                          const string& password, const string& jumpHost)
{
  if (useOpenssh(machine, userName, jumpHost))
    return sshConnectOpenssh(machine, userName, jumpHost);
#ifdef ECCE_HAVE_LIBSSH
  return sshConnectLibssh(machine, userName, password, jumpHost);
#else
  (void)password;
  return false;
#endif
}

// hop(): a new connection to hopMachine, through the same front end the
// current one used, or through the current machine when there was none.
// Compute nodes are reached from the front end, so hops do not nest.
bool RCommand::sshHop(const string& hopMachine, const string& locShell,
                      const string& userName, const string& password,
                      const string& shellPath, const string& libPath,
                      const string& sourceFile)
{
  RemoteTransport* cur = static_cast<RemoteTransport*>(p_transport);
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
string RCommand::sshBackend() const
{
  if (!p_ssh || !p_transport) return "";
  return dynamic_cast<const OpenSshTransport*>(p_transport) ? "openssh" : "libssh";
}

vector<string> RCommand::terminalSshOptions() const
{
  const OpenSshTransport* t = p_ssh ?
    dynamic_cast<const OpenSshTransport*>(p_transport) : 0;
  return t ? t->controlArgs() : vector<string>();
}

string RCommand::frontEndMode() const
{
  if (p_ssh && p_transport) {
    const RemoteTransport* t = static_cast<const RemoteTransport*>(p_transport);
    if (t->jumpHost() != "") return t->nested() ? "nested" : "forward";
  }
  return "";
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

int RCommand::streamFd(void)
{
  return p_stream.rfd;
}

bool RCommand::streamWrite(const string& command)
{
  if (p_stream.wfd < 0) {
    p_errMessage = "No job monitor stream is open";
    return false;
  }
  if (p_ssh ? sendAll(p_stream.wfd, command + "\n")
            : static_cast<DirectTransport*>(p_transport)->writeStream(
                p_stream, command + "\n"))
    return true;
  p_errMessage = "Lost connection to the job monitor attempting to send command";
  return false;
}

bool RCommand::streamWrite(const char* command)
{
  return streamWrite(string(command));
}

void RCommand::streamTimeout(const int& timeout)
{
  p_timeout = timeout == 0 ? RC_EXEC_TIMEOUT : timeout;
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
  // Only the ssh family is left: telnet, Globus, rsh and the site-defined
  // shells of remote_shells.site are all refused alike.
  string name = remShell.substr(0, remShell.find('/'));
  if (name=="" || name=="ssh" || name=="sshpass")
    return "";
  return "The remote shell '" + name + "' is no longer supported by ECCE; "
         "edit this machine in Machine Registration and choose ssh.";
}

// ---------- Constructors ------------
// An 8.x eccejobmaster exports ECCE_TRANSPORT=pty to the programs it starts.
// There is one transport now, so the value is only noted, never an error.
static void noteLegacyTransport()
{
  static bool noted = false;
  const char* env = getenv("ECCE_TRANSPORT");
  if (noted || !env || strcmp(env, "pty") != 0) return;
  noted = true;
  cerr << "ECCE_TRANSPORT=pty is no longer supported; using the default "
          "transport" << endl;
}

// A remote machine reached through the ssh family of shells.
bool RCommand::usesSsh(const string& machine, const string& remShell,
                       const string& userName)
{
  return RCommand::isRemote(machine, remShell, userName) &&
         RCommand::removedShellMessage(remShell) == "";
}

RCommand::RCommand(const string& machine, const string& remShell,
                   const string& locShell, const string& userName,
                   const string& password, const string& frontendMachine,
                   const string& frontendBypass, const string& shellPath,
                   const string& libPath, const string& sourceFile)
{
  p_connected = false;
  p_timeout = RC_EXEC_TIMEOUT;
  p_transport = 0;
  p_ssh = false;
  p_sshStream = 0;

  noteLegacyTransport();

  if (getenv("ECCE_RCOM_LOGMODE")) {
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
  }

  // Set the machine name for the benefit of error messages
  p_machine = (machine=="" || machine=="-f" || machine=="system")?
               RCommand::whereami(): machine;

  const bool remote = RCommand::isRemote(machine, remShell, userName);
  if (remote) {
    p_errMessage = RCommand::removedShellMessage(remShell);
    if (p_errMessage != "")
      return;
  }

  if (!remote) {
    p_transport = new DirectTransport;

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
      cout << "Local machine: commands run without a shell session" << endl;

    p_connected = true;
    return;
  }

  p_shell = "ssh";
  const string pathLine = shellPath == "" ? "" :
    "PATH=" + shQuote(shellPath) + ":$PATH; export PATH\n";
  const string libLine = libPath == "" ? "" :
    "LD_LIBRARY_PATH=" + shQuote(libPath) + ":$LD_LIBRARY_PATH; "
    "export LD_LIBRARY_PATH\n";
  p_scriptPrefix = pathLine + libLine;
  // A refused or failed login is final.  A machine inside the front end's
  // domain is reached directly.
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
}


bool RCommand::hop(const string& hopMachine, const string& locShell,
                   const string& userName, const string& password,
                   const string& shellPath, const string& libPath,
                   const string& sourceFile)
{
  if (!p_ssh || !p_transport) {
    p_errMessage = "hop is only available on an ssh connection";
    return false;
  }
  return sshHop(hopMachine, locShell, userName, password, shellPath, libPath,
                sourceFile);
}


// ---------- Destructors ------------
RCommand::~RCommand(void)
{
  stopStream();
  delete p_transport;
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

  if (getenv("ECCE_RCOM_LOGMODE")) {
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
  }

  RCommand rcmd(machine, remShell, locShell, userName, password,
                frontendMachine, frontendBypass);
  if (rcmd.isOpen()) {
    // No timeout: these may be xterms and the like that the user doesn't
    // want disappearing even after ECCE has been closed.
    rcmd.streamTimeout(-1);

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

string RCommand::argsToCommand(const string& command, const string& args,
                               const string& remShell, const bool& isRemote,
                               string& commandWithArgs)
{
  (void)isRemote;
  commandWithArgs = command;
  if (args != "") {
    if (commandWithArgs == "")
      commandWithArgs = args;
    else
      commandWithArgs += " " + args;
  }
  return remShell;
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

bool RCommand::fileOp(const string& op, const string& filename)
{
  if (!p_connected) return false;

  const string opone = op.substr(0, 1);

  if (!(opone=="e" || opone=="d" || opone=="w" || opone=="r" ||
        opone=="x" || opone=="o" || opone=="z"))
    return false;

  // `test` is a real command with the same syntax in every shell, so no
  // dialect branching is needed; execout() supplies the exit status.
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
  if (!p_connected) return false;

  if (!fileOp("d", directory)) {
    p_errMessage = "Directory " + directory + " does not exist";
    return false;
  }

  // Nothing persists between commands, so remember where we are, as an
  // absolute path; the transport prepends the cd to every later command.
  string output;
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

bool RCommand::which(const string& filename, string& path)
{
  bool ret = true;
  path = filename;

  if (path[0] != '/') {
    string pathvar;
    if (execout("echo $PATH", pathvar)) {
      // drop the trailing CR LF
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

// Callers were written against output with every newline as CR LF and the
// stray backspace and colour sequences removed.
static string crlfOutput(const string& raw)
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

bool RCommand::execout(const string& command, string& output,
                       const string& errorMessage, const int& timeout)
{
  output = "";
  if (!p_connected) return false;

  if (command == "\003") {
    if (p_sshStream) {
      static_cast<RemoteTransport*>(p_transport)->interruptStream(p_sshStream);
      return true;
    }
    if (p_ssh || p_stream.pid <= 0) return false;
    static_cast<DirectTransport*>(p_transport)->interruptStream(p_stream);
    return true;
  }

  if (timeout != 0)
    p_timeout = timeout;

  TransportResult r = p_transport->run(p_scriptPrefix + "exec 2>&1\n" +
                                       command + "\n",
                                       p_timeout > 0 ? p_timeout : -1);

  if (timeout > 0)
    p_timeout = RC_EXEC_TIMEOUT;

  if (getenv("ECCE_RCOM_LOGMODE"))
    cout << "Command (" << command << ") in ("
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
    // Callers were written against a status that begins with 0, 1 or 2;
    // anything else has always been reported as "no status returned".
    char lead = std::to_string(r.status)[0];
    if (lead=='1' || lead=='2')
      p_errMessage = errorMessage != "" ? errorMessage :
                     "Failed executing command " + command;
    else
      p_errMessage = "No status returned from executing command " + command;
  }

  output = crlfOutput(r.out);
  return status;
}

bool RCommand::startStream(const string& command, bool onThisLogin)
{
  if (!p_transport || !p_connected || p_stream.rfd >= 0) return false;
  string error;
  if (p_ssh) {
    int fd = -1;
    RemoteTransport* t = static_cast<RemoteTransport*>(p_transport);
    p_sshStream = onThisLogin
      ? t->openStreamOnLogin(p_scriptPrefix + command, fd, error)
      : t->openStream(p_scriptPrefix + command, fd, error);
    if (!p_sshStream) {
      p_errMessage = "Could not start " + command + ": " + error;
      return false;
    }
    p_stream.rfd = p_stream.wfd = fd;
    if (getenv("ECCE_RCOM_LOGMODE"))
      cout << "ssh stream (" << command << ") in (" << p_transport->dir()
           << ") " << (onThisLogin ? "on this connection's login"
                                   : "on its own session") << endl;
    return true;
  }
  if (!static_cast<DirectTransport*>(p_transport)->openStream(
        command, p_stream, error)) {
    p_errMessage = "Could not start " + command + ": " + error;
    return false;
  }
  if (getenv("ECCE_RCOM_LOGMODE"))
    cout << "Direct stream (" << command << ") in (" << p_transport->dir()
         << ") pid " << p_stream.pid << " on pipes" << endl;
  return true;
}

void RCommand::stopStream(int graceMs)
{
  if (!p_transport) return;
  if (p_sshStream) {
    static_cast<RemoteTransport*>(p_transport)->closeStream(p_sshStream, graceMs);
    p_sshStream = 0;
    p_stream.rfd = p_stream.wfd = -1;
    if (getenv("ECCE_RCOM_LOGMODE")) cout << "ssh stream closed" << endl;
    return;
  }
  if (p_ssh || p_stream.pid <= 0) return;
  int st = static_cast<DirectTransport*>(p_transport)->closeStream(
             p_stream, graceMs);
  if (getenv("ECCE_RCOM_LOGMODE"))
    cout << "Direct stream closed, status " << st << endl;
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

  // nohup so the job outlives this connection; the PID comes back on stdout.
  string error;
  long pid = p_transport->spawnDetached(p_scriptPrefix + "nohup " + command,
                                        error);
  if (getenv("ECCE_RCOM_LOGMODE"))
    cout << "Background command (" << command << ") in ("
         << p_transport->dir() << ") pid " << pid << endl;
  if (pid < 0) {
    p_errMessage = errorMessage != "" ? errorMessage :
                   "Failed executing background command " + command;
    output = "";
    return false;
  }
  output = std::to_string(pid);
  return true;
}

bool RCommand::isOpen(void)
{
  if (!p_connected && p_errMessage.empty())
    p_errMessage = "Failed to open a connection to " + p_machine;

  return p_connected;
}

string RCommand::commError(void)
{ return p_errMessage; }

// A local get/put as one `cp -r`.  The verdict comes from cp's messages:
// "No such file" is only a warning, any other "cp: " line fails the copy.
static bool localCopy(string& errMessage, const string& machine, char** argv)
{
  string script = "exec 2>&1; exec cp -r --";
  int n = 2;
  for (; argv[n]; n++) script += " " + shQuote(argv[n]);

  DirectTransport t;
  TransportResult res = t.run(script, RC_COPY_TIMEOUT);
  if (res.status == -1 || res.timedOut) {
    errMessage = res.timedOut ? "Timeout running copy command cp"
                              : "Unable to spawn copy command cp: " + res.error;
    return false;
  }

  const string where = (machine=="" || machine=="-f" || machine=="system") ?
                       RCommand::whereami() : machine;
  string text = res.out + res.err;
  size_t pos = 0;
  while (pos < text.size()) {
    size_t eol = text.find('\n', pos);
    string line = text.substr(pos, eol == string::npos ? eol : eol - pos);
    const size_t start = pos;
    pos = eol == string::npos ? text.size() : eol + 1;
    if (line.find("No such file or directory") != string::npos ||
        line.find("No match") != string::npos)
      continue;
    if (line.compare(0, 4, "cp: ") != 0) continue;
    if (line.find("specified more than once") != string::npos) return true;
    // Everything cp said from here on.
    errMessage = "Copy command cp failed for " + where + ": " +
                 text.substr(start + 4,
                             text.find_last_not_of("\n") + 1 - (start + 4));
    return false;
  }
  return true;
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


// scp -r over SFTP, with the old scp behaviour callers were written against:
// a missing source or a several-files-to-a-file target is skipped with a
// warning and the call still succeeds.  Remote paths are relative to the login
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
  RCommand rc(machine, remShell, "bash", userName, password);
  if (!rc.isOpen()) {
    errMessage = rc.commError();
    return false;
  }
  RemoteTransport* t = static_cast<RemoteTransport*>(rc.p_transport);
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

// The one entry the get/put overloads share.  An ssh machine goes over SFTP
// (sshCopy), any other remote shell is refused, and a local machine is a
// `cp -r`: sources that do not exist are dropped by the glob, and if none
// are left cp itself reports the missing operand.
bool RCommand::copyFiles(bool putFlag, string& errMessage,
                         const string& machine, const string& remShell,
                         const string& userName, const string& password,
                         const vector<string>& files, const string& toFile)
{
  if (RCommand::isRemote(machine, remShell, userName)) {
    string removed = RCommand::removedShellMessage(remShell);
    if (removed != "") {
      errMessage = removed;
      return false;
    }
    return RCommand::sshCopy(putFlag, machine, remShell, userName, password,
                             files, toFile, errMessage);
  }

  vector<const char*> in;
  for (size_t i = 0; i < files.size(); i++) in.push_back(files[i].c_str());
  in.push_back(NULL);
  char** globbed;
  int num;
  if (!RCommand::globFiles(&in[0], globbed, num))
    return false;

  vector<char*> argv;
  argv.push_back((char*)"cp");
  argv.push_back((char*)"-r");
  for (int i = 0; i < num; i++) argv.push_back(globbed[i]);
  argv.push_back((char*)toFile.c_str());
  argv.push_back((char*)0);

  bool status = localCopy(errMessage, machine, &argv[0]);

  for (int i = 0; i < num; i++) free(globbed[i]);
  free(globbed);
  return status;
}

static vector<string> collectFiles(const char** fromFiles)
{
  vector<string> fs;
  for (int n = 0; fromFiles[n] != NULL; n++) fs.push_back(fromFiles[n]);
  return fs;
}

bool RCommand::get(string& errMessage,
                   const string& machine, const string& remShell,
                   const string& userName, const string& password,
                   int numFiles, ...)
{
  if (numFiles < 2)
    return false;

  va_list ap;
  va_start(ap, numFiles);
  vector<string> fs;
  for (int k=0; k<numFiles-1; k++) fs.push_back(va_arg(ap, char*));
  string to = va_arg(ap, char*);
  va_end(ap);

  return RCommand::copyFiles(false, errMessage, machine, remShell, userName,
                             password, fs, to);
}

bool RCommand::get(string& errMessage,
                   const string& machine, const string& remShell,
                   const string& userName, const string& password,
                   const vector<string>& fromFiles, const string& toFile)
{
  return RCommand::copyFiles(false, errMessage, machine, remShell, userName,
                             password, fromFiles, toFile);
}

bool RCommand::get(string& errMessage,
                   const string& machine, const string& remShell,
                   const string& userName, const string& password,
                   const char** fromFiles, const string& toFile)
{
  return RCommand::copyFiles(false, errMessage, machine, remShell, userName,
                             password, collectFiles(fromFiles), toFile);
}

bool RCommand::put(string& errMessage,
                   const string& machine, const string& remShell,
                   const string& userName, const string& password,
                   int numFiles, ...)
{
  if (numFiles < 2)
    return false;

  va_list ap;
  va_start(ap, numFiles);
  vector<string> fs;
  for (int k=0; k<numFiles-1; k++) fs.push_back(va_arg(ap, char*));
  string to = va_arg(ap, char*);
  va_end(ap);

  return RCommand::copyFiles(true, errMessage, machine, remShell, userName,
                             password, fs, to);
}

bool RCommand::put(string& errMessage,
                   const string& machine, const string& remShell,
                   const string& userName, const string& password,
                   const vector<string>& fromFiles, const string& toFile)
{
  return RCommand::copyFiles(true, errMessage, machine, remShell, userName,
                             password, fromFiles, toFile);
}

bool RCommand::put(string& errMessage,
                   const string& machine, const string& remShell,
                   const string& userName, const string& password,
                   const char** fromFiles, const string& toFile)
{
  return RCommand::copyFiles(true, errMessage, machine, remShell, userName,
                             password, collectFiles(fromFiles), toFile);
}


// Copy one file byte for byte: a local machine has no transport to carry it.
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

// A local machine copies the file itself; an ssh machine goes over SFTP.
// remote is already resolved against the directory cd() set.
bool RCommand::transferFile(bool putFlag, const string& local,
                            const string& remote)
{
  if (p_ssh) {
    string error;
    RemoteTransport* t = static_cast<RemoteTransport*>(p_transport);
    return putFlag ? t->put(local, remote, error) : t->get(remote, local, error);
  }
  return putFlag ? copyFileData(local, remote) : copyFileData(remote, local);
}

bool RCommand::shellput(const char** fromFiles, const string& toFile)
{
  string fullToFile;
  char* baseFrom;
  bool status = false;
  char **globbedFiles;
  int ig;

  int numFiles;
  if (!RCommand::globFiles(fromFiles, globbedFiles, numFiles))
    return false;

  if (!p_connected) return false;

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
  string globbedFileStr;
  string globbedFile;
  char* baseFrom;
  bool status = false;

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

        status = transferFile(false, fullToFile,
                              underDir(p_transport->dir(), globbedFile));
        if (!status) {
          p_errMessage = "Failed executing cat command for get file "
                         "operation";
          return false;
        }
      }
    }
  }

  return status;
}


const bool RCommand::getPassCache(const string& shell, const string& machine,
                                  const string& user, string& password)
{
  bool ret = false;

  string url = shell + "://" + machine;
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
  string url = shell + "://" + machine;
  AuthCache::getCache().addAuthentication(url, user, password, "",true);
}

