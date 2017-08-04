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
                     theShell=="telnet" || theShell.find("telnet/")==0 ||
                     theShell=="Globus" || theShell.find("Globus/")==0 ||
                     theShell=="Globus-ssh" ||theShell.find("Globus-ssh/")==0 ||
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

