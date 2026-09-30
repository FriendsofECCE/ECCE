///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: EcceShell.C
//
//
// DESIGN:
//   This class is intended to isolate the code for creating remote shells.
//   Currently this class supports only xterm for the shell.  ECCE colors
//   are used for the xterm colors.
//
///////////////////////////////////////////////////////////////////////////////

// system includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> // access
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>

#include <sstream>
#include <vector>

#include <iostream>
  using std::cout;
  using std::endl;

#include <string>

using std::string;

// library includes

// application includes
#include "util/ErrMsg.H"
#include "util/Host.H"
#include "util/Color.H"
#include "util/UserEditor.H"

#include "comm/RCommand.H"
#include "tdat/RefMachine.H"

#include "comm/EcceShell.H"

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
EcceShell::EcceShell(void)
{
  p_status = 0;
}
 
// ---------- Virtual Destructor ------------
EcceShell::~EcceShell(void)
{
}


string EcceShell::dirshell(const string& title,
                           const string& machineName,
                           const string& shell,
                           const string& user,
                           const string& password,
                           const string& pathBase,
                           const string& pathFull
)
{
  return remoteShell(machineName, shell, user, password,
                     pathBase, pathFull, title);
}

string EcceShell::cmdshell(const string& title,
                           const string& machineName,
                           const string& shell,
                           const string& user,
                           const string& password,
                           const string& cmd,
                           const string& file,
                           bool xtermFlag)
{
  return remoteShell(machineName, shell, user, password,
                     "", file, title, cmd, xtermFlag);
}

string EcceShell::topshell(const string& machineName,
                           const string& shell,
                           const string& user,
                           const string& password)
{
  return remoteShell(machineName, shell, user, password,
                     "", "", machineName);
}


static void xhostCmd(const string& refMachineName, const string& machineName)
{
  string xhostStr;
  string configName = RefMachine::configFile(refMachineName);

  if (access(configName.c_str(), F_OK) == 0) {
    ifstream is(configName.c_str());
    char buf[256];
    while (!is.eof()) {
      is.getline(buf,255);
      if (strncmp(buf,"xhost:",6) == 0) {
        // strips off the first 6 characters and leaves the rest
        xhostStr = "xhost +";

        xhostStr += &buf[6];
        // guarantee we aren't just completely opening an X server
        if (xhostStr.find_last_not_of(" \t") != 6)
          system(xhostStr.c_str());
        is.close();
        return;
      }
    }
    is.close();
  }

  xhostStr = "xhost +" + machineName;
  // guarantee we aren't just completely opening an X server
  if (xhostStr.find_last_not_of(" \t") != 6)
    system(xhostStr.c_str());
}



// ---------------------------------------------------------------------------
// ECCE_TRANSPORT=ssh: local terminal running the OpenSSH client
// ---------------------------------------------------------------------------

static string shQuote(const string& s)
{
  string q = "'";
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '\'') q += "'\\''";
    else q += s[i];
  }
  return q + "'";
}

// Left bare when plain, so the common case reads as typed.
static string shWord(const string& s)
{
  if (!s.empty() && s.find_first_not_of(
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
        "_./~+@%:=,-") == string::npos)
    return s;
  return shQuote(s);
}

static string baseName(const string& path)
{
  string::size_type slash = path.rfind('/');
  return slash == string::npos ? path : path.substr(slash+1);
}

static string findInPath(const string& prog)
{
  if (prog.find('/') != string::npos)
    return access(prog.c_str(), X_OK) == 0 ? prog : "";
  const char* p = getenv("PATH");
  std::istringstream is(p ? p : "");
  string dir;
  while (std::getline(is, dir, ':')) {
    string f = dir + "/" + prog;
    if (access(f.c_str(), X_OK) == 0) return f;
  }
  return "";
}

// How a terminal is told to run a command: "" for programs that take the
// command as their remaining arguments.
static string execFlag(const string& base)
{
  if (base == "gnome-terminal" || base == "ptyxis" || base == "wezterm")
    return "--";
  if (base == "xfce4-terminal" || base == "mate-terminal")
    return "-x";
  if (base == "kitty" || base == "foot" || base == "footclient")
    return "";
  return "-e";
}

string EcceShell::remoteCommand(const string& mshell, const string& sourceFile,
                                const string& dir, const string& cmd,
                                string& error)
{
  // csh expands ! even inside single quotes, and no quoting of it is
  // the same in csh and sh.
  if (sourceFile.find('!') != string::npos || dir.find('!') != string::npos ||
      cmd.find('!') != string::npos) {
    error = "A '!' in a file name or command cannot be passed safely to the "
            "remote login shell.";
    return "";
  }
  const string m = mshell == "" ? "sh" : mshell;
  const bool csh = m.find("csh") != string::npos;

  // The file is written for the machine's own shell, so it is sourced by
  // that shell, as the pty path did; the quoting below is for the login
  // shell that receives the line, which may be another one.
  string inner;
  if (sourceFile != "")
    inner = csh ? "if (-e " + sourceFile + ") source " + sourceFile :
                  "[ -e " + sourceFile + " ] && . " + sourceFile;
  if (inner != "") inner += "; ";
  if (dir != "") inner += "cd " + shWord(dir) + " && ";
  inner += cmd != "" ? cmd : "exec $SHELL";
  return "exec " + m + " -c " + shQuote(inner);
}

bool EcceShell::terminalArgv(const SshTerminal& t, vector<string>& argv,
                             string& error)
{
  argv.clear();
  std::istringstream is(UserEditor::getTerminal());
  string w;
  vector<string> words;
  while (is >> w) words.push_back(w);
  if (words.empty()) words.push_back("xterm");
  const string path = findInPath(words[0]);
  if (path == "") {
    error = "Could not find terminal " + words[0] + " in path.";
    return false;
  }
  const string base = baseName(words[0]);

  argv.push_back(path);
  for (size_t i = 1; i < words.size(); i++) argv.push_back(words[i]);

  // Title, colours and geometry are xterm options.
  if (base == "xterm") {
    argv.push_back("-title");
    argv.push_back(t.title);
    if (getenv("ECCE_XTERM_FONT")) {
      argv.push_back("-fn");
      argv.push_back(getenv("ECCE_XTERM_FONT"));
    }
    argv.push_back("-bg");
    argv.push_back(string(Color::READONLY));
    argv.push_back("-fg");
    argv.push_back(string(Color::TEXT));
    argv.push_back("-sb");
    if (t.geometry != "") {
      argv.push_back("-geom");
      argv.push_back(t.geometry);
    }
  }
  const string flag = execFlag(base);
  if (flag != "") argv.push_back(flag);

  argv.push_back("ssh");
  argv.push_back("-t");
  if (t.frontend != "" && t.frontendMode == "nested") {
    // ssh on the front end to the node; the node's command is quoted for
    // the front end's login shell and then for the node's.
    if (t.user != "") { argv.push_back("-l"); argv.push_back(t.user); }
    argv.push_back(t.frontend);
    string inner = "ssh -t";
    if (t.user != "") inner += " -l " + shQuote(t.user);
    argv.push_back(inner + " " + shQuote(t.host) + " " +
                   shQuote(t.remoteCommand));
    return true;
  }
  if (t.frontend != "") {
    argv.push_back("-J");
    argv.push_back(t.user != "" ? t.user + "@" + t.frontend : t.frontend);
  }
  if (t.user != "") { argv.push_back("-l"); argv.push_back(t.user); }
  argv.push_back(t.host);
  argv.push_back(t.remoteCommand);
  return true;
}

bool EcceShell::spawnDetached(const vector<string>& argv, string& error)
{
  if (argv.empty()) { error = "No command."; return false; }
  vector<char*> av;
  for (size_t i = 0; i < argv.size(); i++)
    av.push_back(const_cast<char*>(argv[i].c_str()));
  av.push_back(0);

  int rp[2];
  if (pipe2(rp, O_CLOEXEC) < 0) { error = strerror(errno); return false; }
  pid_t mid = fork();
  if (mid < 0) {
    error = strerror(errno);
    close(rp[0]); close(rp[1]);
    return false;
  }
  if (mid == 0) {
    setsid();
    pid_t pid = fork();
    if (pid == 0) {
      int dn = open("/dev/null", O_RDWR);
      if (dn >= 0) { dup2(dn, 0); dup2(dn, 1); dup2(dn, 2); }
      execv(av[0], &av[0]);
      int e = errno;
      if (write(rp[1], &e, sizeof e) < 0) {}
      _exit(127);
    }
    _exit(pid < 0 ? 1 : 0);
  }
  close(rp[1]);
  int status = 0;
  waitpid(mid, &status, 0);
  int e = 0;
  ssize_t n = read(rp[0], &e, sizeof e);
  close(rp[0]);
  if (n == (ssize_t)sizeof e) {
    error = "Unable to run " + argv[0] + ": " + strerror(e);
    return false;
  }
  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    error = "Unable to start " + argv[0];
    return false;
  }
  return true;
}

string EcceShell::sshTerminal(RefMachine* refMachine, const string& shell,
                              const string& user, const string& password,
                              const string& pathBase, const string& pathFull,
                              const string& title, const string& cmd)
{
  const string machineName = refMachine->fullname();
  string ret;

  // The same login the other operations make; it answers the directory
  // checks and, for a front end, says whether it was forwarded or nested.
  RCommand rcmd(refMachine->fullname(), shell, refMachine->shell(), user,
                password, refMachine->frontendMachine(),
                refMachine->frontendBypass(), refMachine->shellPath(),
                refMachine->libPath(), refMachine->sourceFile());
  if (!rcmd.isOpen()) {
    ErrMsg().flush();
    p_status = -1;
    return rcmd.commError().c_str();
  }

  SshTerminal t;
  t.title = title;
  t.host = refMachine->fullname();
  t.user = user;
  t.frontendMode = rcmd.frontEndMode();
  if (t.frontendMode != "") t.frontend = refMachine->frontendMachine();

  string dir, run;
  if (pathBase != "" && pathFull != "") {
    if (rcmd.cd(pathFull.c_str())) {
      dir = pathFull;
    } else if (rcmd.cd(pathBase.c_str())) {
      dir = pathBase;
      ret = "The calculcation directory on " + machineName + " does not "
            "exist--starting shell in base directory.";
    } else {
      ret = "The calculation and base directories on " + machineName +
            " do not exist--starting shell in home directory.";
    }
  } else if (cmd != "") {
    run = cmd;
    if (pathFull != "") {
      if (!rcmd.exists(pathFull.c_str())) {
        p_status = -1;
        return "The file " + pathFull + " on " + machineName +
               " does not exist--cannot run remote command.";
      }
      t.geometry = "80x40";
      if (pathFull.find("amica.out") != string::npos) {
        string pcmd = "perl -e 'open(INFILE, \"" + pathFull + "\"); "
                      "while (<INFILE>) {exit(0) if (length() > 81); "
                      "exit(1) if ($lines_in++ > 1000);} exit(1);'";
        if (rcmd.exec(pcmd)) t.geometry = "132x40";
      }
    }
  } else {
    dir = "~";
  }

  string error;
  t.remoteCommand = remoteCommand(refMachine->shell(), refMachine->sourceFile(),
                                  dir, run, error);
  vector<string> argv;
  if (t.remoteCommand == "" || !terminalArgv(t, argv, error) ||
      !spawnDetached(argv, error)) {
    p_status = -1;
    return error;
  }
  return ret;
}

string EcceShell::remoteShell
( 
  const string& machineName,
  const string& shell,
  const string& user,
  const string& password,
  const string& pathBase,
  const string& pathFull,
  const string& title,
  const string& cmd,
  bool xtermFlag
)
{
  p_status = 0;
  string ret = "";

  RefMachine* refMachine = RefMachine::refLookup(machineName);
  EE_RT_ASSERT(refMachine,EE_WARNING,"No RefMachine object");
  if (!refMachine) {
    // EE_WARNING never aborts (that's EE_FATAL only), so this guard is
    // the only thing standing between a machine name that's no longer
    // registered (e.g. renamed after a job referencing it was launched)
    // and a null dereference below. See RunMgmt::terminate() for the
    // same pattern found elsewhere in the run-management code path.
    p_status = -1;
    return "Machine \"" + machineName + "\" is not currently registered.";
  }

  if (xtermFlag && RCommand::usesLibssh(refMachine->fullname(), shell, user))
    return sshTerminal(refMachine, shell, user, password, pathBase, pathFull,
                       title, cmd);

  bool frontendFlag = refMachine->singleConnect() ||
                      (refMachine->frontendMachine()!="" &&
                       (refMachine->frontendBypass()=="" ||
                        !RCommand::isSameDomain(refMachine->frontendBypass())));
  bool presetDisplayFlag = true;

  RCommand* rcmd = 0;

  if (!frontendFlag) {
    rcmd = new RCommand(refMachine->fullname(), shell, 
                        refMachine->shell(), user, password,
                        refMachine->frontendMachine(),
                        refMachine->frontendBypass(),
                        refMachine->shellPath(),refMachine->libPath(),
                        refMachine->sourceFile());
    if (!rcmd->isOpen()) {
      ErrMsg().flush();
      p_status = -1;
      ret = rcmd->commError().c_str();
      delete rcmd;
      return ret;
    }

    // Added to handle Mac OS X ssh connections that don't use port forwarding
    // GDB 5/12/09
    presetDisplayFlag = rcmd->exec("echo $DISPLAY");
  }

  string theShell = RCommand::commandShell(refMachine->fullname(),shell,user);

  bool displayFlag = theShell=="rsh" || theShell.find("rsh/")==0 ||
                     !presetDisplayFlag;

  bool xsetFlag = displayFlag || theShell=="" || theShell=="ssh" ||
                  theShell=="sshpass" || theShell.find("ssh/")==0 ||
                  theShell.find("/ssh")==theShell.length()-4 ||
                  !RCommand::isRemote(refMachine->fullname(), theShell, user);

  bool dblXtermFlag = xtermFlag && xsetFlag;

  string execStr;
  if (dblXtermFlag)
    execStr = shellCmd(title);
  else if (cmd != "")
    execStr = cmd;

  string xsetStr = "xset q";
  string display = "";

  if (machineName!="" && displayFlag) {
    xhostCmd(machineName, refMachine->fullname());

    // Determine a good DISPLAY environment variable value.  This logic
    // even handles inter-domain remote shells when the current DISPLAY
    // value is inadequate (not fully qualified).
    if (getenv("DISPLAY"))
      display = getenv("DISPLAY");

    size_t colon = display.find(":");
    if (colon == string::npos) {
      display.append(":0.0");
      colon = display.find(":");
    }

    if (display.find(".")==string::npos || display.find(".")>colon) {
      // Note prefer machine name from DISPLAY rather than from library calls
      Host myhost;
      string machine;
      if (colon == 0) {
        machine = myhost.host_name();
      }
      else
        machine = display.substr(0, colon);

      string fullname = myhost.fullyQualifiedName();
      if (fullname != myhost.host_name()) {
        display = machine + fullname.substr(fullname.find('.')) + 
                  display.substr(colon, display.size()-colon);
      } else {
        display = machine +  display.substr(colon, display.size()-colon);
      }
    }

    execStr.append(" -display " + display);
    xsetStr.append(" -display " + display);
  }

  string args = "";

  // this would be a dirshell() invocation
  if (pathBase!="" && pathFull!="") {
    if (frontendFlag) {
      args = refMachine->shell() + " -c \"";
      if (refMachine->sourceFile() != "")
        args += "source " + refMachine->sourceFile() + " && ";
      args += "cd " + pathFull + " && $SHELL\"";
    } else if (!rcmd->cd(pathFull.c_str())) {
      if (!rcmd->cd(pathBase.c_str())) {
        args = refMachine->shell() + " -c \"";
        if (refMachine->sourceFile() != "")
          args += "source " + refMachine->sourceFile() + " && ";
        args += "$SHELL\"";
        ret = "The calculation and base directories on " + machineName +
              " do not exist--starting shell in home directory.";
      }
      else {
        args = refMachine->shell() + " -c \"";
        if (refMachine->sourceFile() != "")
          args += "source " + refMachine->sourceFile() + " && ";
        args += "cd " + pathBase + " && $SHELL\"";
        ret = "The calculcation directory on " + machineName + " does not "
              "exist--starting shell in base directory.";
      }
    } else {
      args = refMachine->shell() + " -c \"";
      if (refMachine->sourceFile() != "")
        args += "source " + refMachine->sourceFile() + " && ";
      args += "cd " + pathFull + " && $SHELL\"";
    }

    if (dblXtermFlag)
      execStr.append(" -e");

  // this is a cmdshell() invocation
  } else if (cmd != "") {
    args = cmd;

    if (dblXtermFlag) {
      if (pathFull != "") {
        if (frontendFlag) {
          execStr.append(" -geom 80x40");
        } else if (!rcmd->exists(pathFull.c_str())) {
          ret = "The file " + pathFull + " on " + machineName +
              " does not exist--cannot run remote command.";
          p_status = -1;
          delete rcmd;
          return ret;
        } else if (pathFull.find("amica.out") != string::npos) {
          // this little bit of magic checks if any line is > 80 characters
          // and overrides the default xterm width of 80 to 132 if it is
          // GDB 12/3/02 only do this logic for Amica at the request of evorpa
          string pcmd = "perl -e 'open(INFILE, \"" + pathFull + "\"); "
                        "while (<INFILE>) {exit(0) if (length() > 81); "
                        "exit(1) if ($lines_in++ > 1000);} exit(1);'";
          if (rcmd->exec(pcmd))
            execStr.append(" -geom 132x40");
          else
            execStr.append(" -geom 80x40");
        } else
          execStr.append(" -geom 80x40");
      }
      execStr.append(" -e");
    }

  // this is a topshell() invocation
  } else {
    args = refMachine->shell() + " -c \"";
    if (refMachine->sourceFile() != "")
      args += "source " + refMachine->sourceFile() + " && ";
    args += "cd ~ && $SHELL\"";

    if (dblXtermFlag)
      execStr.append(" -e");
  }

  string errorMessage = "Can't display to local machine.  ";

  if (displayFlag) {
    errorMessage += "Either you "
         "must explicitly issue the command 'xhost +" +refMachine->fullname()
         + "' on the local machine or the DISPLAY variable ";

    if (display != "")
      errorMessage += "set to " + display;
 
    errorMessage += " is incorrect.  It must be fully qualified with the "
          "domain name if you are displaying to a machine on another domain.";
  } else if (theShell=="" || theShell=="ssh" ||
             theShell=="sshpass" || theShell.find("ssh/")==0)
    errorMessage += "Ssh X11 port forwarding may be disabled by ssh "
                    "configuration or your ~/.ssh/known_hosts file may "
                    "contain invalid entries for this machine.";
  else
    errorMessage += "Site-specific remote X Windows shell command did "
                    "not work.";

  string output;
  string cmdErr;

  if (frontendFlag) {
    if (!RCommand::bgcommand(execStr, args, cmdErr, machineName,
                             shell, user, password)) {
      ret = cmdErr;
      ret += " -- " + errorMessage;
      p_status = -1;
    }
  } else if (xsetFlag) {
    if (!rcmd->exec(xsetStr.c_str(), errorMessage.c_str())) {
      ret = rcmd->commError().c_str();
      p_status = -1;
    } else if (!RCommand::bgcommand(execStr, args, cmdErr, machineName,
                                    shell, user, password)) {
      ret = cmdErr;
      ret += " -- " + errorMessage;
      p_status = -1;
    }
  } else if (!RCommand::bgcommand(execStr, args, cmdErr, machineName,
                                 shell, user, password)) {
    ret = cmdErr;
    ret += " -- " + errorMessage;
    p_status = -1;
  }

  if (rcmd != (RCommand*)0)
    delete rcmd;
  return ret;
}


/////////////////////////////////////////////////////////////////////////////
// Description
//  Return xterm command string.  Note that the colors must be quoted
//  to work properly on local commands (system(3)) if the color is
//  an X hex color which it most likely is.  This doesn't seem to
//  cause problems for RCommand.
/////////////////////////////////////////////////////////////////////////////
string EcceShell::shellCmd(const string& title)
{
  char xtermStr[256];

  string read = Color::READONLY;
  string fg = Color::TEXT;

  if (getenv("ECCE_XTERM_FONT"))
    sprintf(xtermStr,
            "xterm -fn \"%s\" -title \"%s\" -bg \"%s\" -fg \"%s\" -sb",
            getenv("ECCE_XTERM_FONT"), title.c_str(), (char*)read.c_str(),
            (char*)fg.c_str());
  else
    sprintf(xtermStr,
            "xterm -title \"%s\" -bg \"%s\" -fg \"%s\" -sb",
            title.c_str(), (char*)read.c_str(), (char*)fg.c_str());

  return xtermStr;
}


///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Returns the status from the last command.
//
///////////////////////////////////////////////////////////////////////////////
int EcceShell::lastStatus(void) const
{
  return p_status;
}


// --------------------------
// Protected Member Functions
// --------------------------


// ------------------------
// Private Member Functions
// ------------------------

EcceShell::EcceShell(const EcceShell& XXX)
{
#ifdef DEBUG
  cerr << "EcceShell: Copy constructor not supported." << endl;
#endif
}

