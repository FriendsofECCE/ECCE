//////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: RCommand.C
//
// DESIGN:
//
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
  using std::cout;
  using std::endl;
#include <fstream>
  using std::ifstream;
  using std::ofstream;
#include <map>
  using std::map;

#include <stdlib.h> // getenv
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

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

#define MAXARGS 32
#define MAXLINE 256

static const char* sshpass_opts[] = {"-o", "PasswordAuthentication=yes",
                                     "-o", "StrictHostKeyChecking=no",
                                     "-o", "FallBackToRsh=no",
                                     "-o", "UseRsh=no",
                                     "-o", "RhostsAuthentication=no",
                                     "-o", "RhostsRSAAuthentication=no",
                                     "-o", "RSAAuthentication=no",
                                     "-o", "TISAuthentication=no",
                                     0};

static const char* globus_opts[] = {"-o", "GSSAPIAuthentication=yes",
                                    "-o", "StrictHostKeyChecking=no",
                                    "-o", "FallBackToRsh=no",
                                    "-o", "UseRsh=no",
                                    "-o", "PasswordAuthentication=no",
                                    "-o", "RhostsAuthentication=no",
                                    "-o", "RhostsRSAAuthentication=no",
                                    "-o", "RSAAuthentication=no",
                                    "-o", "TISAuthentication=no",
                                    0};


int RCommand::expect1(const char* pat)
{
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
  int it;

  for (it=0; p_pats[it].type != exp_end; it++)
    free(p_pats[it].pattern);

  free((char*)p_pats);
}

int RCommand::patexpect(void)
{
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

int RCommand::expfid(void)
{
  return p_fid;
}

bool RCommand::expwrite(const string& command)
{
  int comlen = command.length();

  if (comlen >= 256) {
    p_errMessage = "Exceeds maximum C shell command length of 256 characters";
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
  int comlen = strlen(command);

  if (comlen >= 256) {
    p_errMessage = "Exceeds maximum C shell command length of 256 characters";
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

  if (comlen >= 256) {
    errMessage = "Exceeds maximum C shell command length of 256 characters";
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

  // Hardwire return value for testing local Globus launches
#if 111
  if (machine=="" || machine=="-f" || machine=="system" ||
      remShell.find("Globus")!=0) {
#endif
    string whereami = RCommand::whereami();

    if (whereami != "")
      localFlag = (machine=="" || machine=="-f" ||
                   machine=="system" || machine==whereami ||
                   (machine.length()>whereami.length() &&
                    machine.substr(0,whereami.length())==whereami &&
                    machine[whereami.length()]=='.'));

    if (localFlag)
      localFlag = userName=="" || strcmp(userName.c_str(), Ecce::realUser())==0;
#if 111
  }
#endif

  return !localFlag;
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

string RCommand::globusRSL(const string& command, const string& queueRSL)
{
  string rsl = "";

  if (command != "") {
    char* cmd = strtok((char*)command.c_str(), " \t\n");

    // Grab all the remaining arguments in one shot
    char* args = strtok(NULL, "\0");

    rsl = "&(executable=";
    rsl += cmd;
    rsl += ")";

    if (args != NULL) {
      // Start after the first space character
      rsl += "(arguments=";
      rsl += args+1;
      rsl += ")";
    }
  }

  if (queueRSL != "")
    rsl += queueRSL;

  return rsl;
}

string RCommand::shellCommand(const string& remShell, const string& machine,
                              const string& locShell, const string& userName,
                              const bool& hopFlag, string& proxyAuth,
                              char** argv)
{
  string theShell;

  static const char* minfc  =  "-fc";
  static const char* minc  =  "-c";
  string echoshell = "echo +hi+ && " + locShell + " -f";

  static const char* minl  =  "-l";
  static const char* cmd[] = {"echo", "+hi+", "&&", "", "-i", 0};
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
        remShell.find("ssh/")==0 || remShell=="Globus-ssh" ||
        remShell.find("Globus-ssh/")==0) {
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

    else if (remShell=="telnet" || remShell.find("telnet/") == 0)
      theShell = "telnet";

    else if (remShell=="Globus" || remShell.find("Globus/")==0) {
      theShell = "globusrun";

      // enable GSI ssh verbose mode
      argv[argc++] = (char*)minv;

      // GSI ssh does not work with port forwarding
      // argv[argc++] = (char*)mino;
      // argv[argc++] = (char*)minx;

      // All the globus ssh "-o" options that attempt to force GSSAPI
      // authentication.  They seem to have some effect although I'm sure the
      // server side sshd daemon ultimately decides what authentication it
      // will accept.
      for (it=0; globus_opts[it]!=(char*)0; it++)
        argv[argc++] = (char*)globus_opts[it];

    } else {
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

    if (theShell != "telnet") {
      // user name is irrelevant for Globus
      if (theShell!="globusrun" && userName!="") {
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
        remShell.find("ssh/")==0 || remShell=="Globus-ssh" ||
        remShell.find("Globus-ssh/")==0) {
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

    } else if (remShell=="telnet" || remShell.find("telnet/") == 0)
      theShell = "telnet";

    else if (remShell=="Globus" || remShell.find("Globus/")==0) {
      theShell = "globusrun";

      // GSI ssh does not work with port forwarding
      // enable ssh X11 port forwarding for Globus
      // argv[argc++] = (char*)mino;
      // argv[argc++] = (char*)minx;

      // All the globus ssh "-o" options that attempt to force GSSAPI
      // authentication.  They seem to have some effect although I'm sure the
      // server side sshd daemon ultimately decides what authentication it
      // will accept.
      for (it=0; globus_opts[it]!=(char*)0; it++)
        argv[argc++] = (char*)globus_opts[it];

    } else {
      theShell = RCommand::userShellCommandArgs(remShell, proxyAuth, argc,argv);
      if (theShell=="ssh" || (theShell.find("/ssh")!=string::npos &&
                              theShell.find("/ssh")==theShell.length()-4)) {
        // enable ssh X11 port forwarding
        argv[argc++] = (char*)mino;
        argv[argc++] = (char*)minx;
      }
    }

    if (theShell != "telnet") {
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
            if (theShell=="ssh" || theShell=="rsh" || theShell=="globusrun") {
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
          if (theShell=="ssh" || theShell=="rsh" || theShell=="globusrun") {
            string cshCommand = locShell + " -c '" + command + "'";
            argv[argc++] = strdup((char*)cshCommand.c_str());
          } else
            argv[argc++] = strdup((char*)command.c_str());
        }

      } else {
        // user name is irrelevant for Globus
        if (theShell!="globusrun" && userName!="") {
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
          if (theShell=="ssh" || theShell=="rsh" || theShell=="globusrun") {
            string cshCommand = locShell + " -c '" + command + "'";
            argv[argc++] = strdup((char*)cshCommand.c_str());
          } else
            argv[argc++] = strdup((char*)command.c_str());
        }
#endif
      }
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
        remShell=="ssh" || remShell=="sshpass" || remShell=="Globus-ssh") {
      theCopy = "scp";

      if (remShell == "sshpass")
        for (it=0; sshpass_opts[it]!=(char*)0; it++)
          argv[argc++] = (char*)sshpass_opts[it];

      argv[argc++] = (char*)minr;

    } else if (remShell=="globus-rcp" ||
               (remShell.find("/globus-rcp")!=string::npos &&
                remShell.find("/globus-rcp")==remShell.length()-11) ||
               remShell=="Globus") {
      theCopy = "globus-rcp";

      for (it=0; globus_opts[it]!=(char*)0; it++)
        argv[argc++] = (char*)globus_opts[it];

      argv[argc++] = (char*)minr;

    } else if (remShell=="rcp" ||
               (remShell.find("/rcp")!=string::npos &&
                remShell.find("/rcp")==remShell.length()-4) ||
               remShell=="rsh") {
      theCopy = "rcp";
      argv[argc++] = (char*)minr;

    } else if (remShell=="ftp" ||
               (remShell.find("/ftp")!=string::npos &&
                remShell.find("/ftp")==remShell.length()-4) ||
               remShell=="telnet") {
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
  else if (copyCmd == "globus-rcp")
    shellCmd = "Globus";
  else if (copyCmd == "rcp")
    shellCmd = "rsh";
  else if (copyCmd == "ftp")
    shellCmd = "telnet";


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
RCommand::RCommand(const string& machine, const string& remShell,
                   const string& locShell, const string& userName,
                   const string& password, const string& frontendMachine,
                   const string& frontendBypass, const string& shellPath,
                   const string& libPath, const string& sourceFile)
{
  p_connected = false;
  p_background = false;
  p_hopCount = 0;

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

  // Globus uses the GSSAPI version of ssh for persistent connections
  if (p_shell == "globusrun") {
    if (!RCommand::globusproxy(password, p_errMessage))
      return;

    p_shell = "ssh";
    argv[0] = strdup((char*)p_shell.c_str());
  } else if (proxyAuth != "") {
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
  bool first_telnet_prompt = true;
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
                               exp_glob, "telnet> $", 11,
                               exp_glob, "login: $", 12,
                               exp_glob, "Authentication succeeded", 14,
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
          passCmd = "./passdialog password " + theMachine + " " + theUser;
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
        passCmd = "./passdialog passcode " + theMachine + " " + theUser;
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

      case 11:
        if (first_telnet_prompt) {
          if (!expwrite("environ define TERM ECCE_TELNET")) return;
          first_telnet_prompt = false;
        }
        else {
          if (!expwrite("open " + theMachine)) return;
        }
        done = false;
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
  if (!expwrite("unalias precmd; set prompt=+go+; unset echo")) return;
  if (expect1("+go+$") != 1) {
    p_errMessage =
      "Unsuccessful remote shell login--invalid username or password";
    if (p_shell == "telnet")
      p_errMessage += "\nDid you forget to modify the .login file to remove"
                      " interactive prompts based on the $TERM variable?";
    return;
  }

  // Set timeout back to normal
  exp_timeout = RC_EXEC_TIMEOUT;

  if (!expwrite("unalias *")) return;
  if (expect1("\r\n+go+$") != 1) {
    p_errMessage = "Unsuccessful remote shell login--unalias * failed";
    return;
  }

  if (p_shell == "telnet") {
    if (!expwrite("setenv TERM xterm")) return;
    if (expect1("\r\n+go+$") != 1) {
      p_errMessage = "Unsuccessful telnet login--setenv TERM xterm failed";
      return;
    }
  }

  // set $PATH
  if (shellPath != "") {
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

  // set $LD_LIBRARY_PATH
  if (libPath != "") {
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

  // source file, if specified
  if (sourceFile != "") {
    cmd = "if (-e " + sourceFile + ") source " + sourceFile;
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
  p_hopCount++;

  bool done;
  string output;
  bool first_telnet_prompt = true;
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
                               exp_glob, "telnet> $", 11,
                               exp_glob, "login: $", 12,
                               exp_glob, "Authentication succeeded", 14,
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
          passCmd = "./passdialog password " + hopMachine + " " + theUser;
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
        passCmd = "./passdialog passcode " + hopMachine + " " + theUser;
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

      case 11:
        if (first_telnet_prompt) {
          if (!expwrite("environ define TERM ECCE_TELNET")) return false;
          first_telnet_prompt = false;
        }
        else {
          if (!expwrite("open " + p_machine)) return false;
        }
        done = false;
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
  }

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
  if (!expwrite("unalias precmd; set prompt=+go+; unset echo")) return false;
  if (expect1("+go+$") != 1) {
    p_errMessage =
      "Unsuccessful remote shell login--invalid username or password";
    if (p_shell == "telnet")
      p_errMessage += "\nDid you forget to modify the .login file to remove"
                      " interactive prompts based on the $TERM variable?";
    return false;
  }

  // Set timeout back to normal
  exp_timeout = RC_EXEC_TIMEOUT;

  if (!expwrite("unalias *")) return false;
  if (expect1("\r\n+go+$") != 1) {
    p_errMessage = "Unsuccessful remote shell login--unalias * failed";
    return false;
  }

  if (p_shell == "telnet") {
    if (!expwrite("setenv TERM xterm")) return false;
    if (expect1("\r\n+go+$") != 1) {
      p_errMessage = "Unsuccessful telnet login--setenv TERM xterm failed";
      return false;
    }
  }

  // set $PATH
  if (shellPath != "") {
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

  // set $LD_LIBRARY_PATH
  if (libPath != "") {
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

  // source file, if specified
  if (sourceFile != "") {
    cmd = "if (-e " + sourceFile + ") source " + sourceFile;
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
  if (p_connected) {
hopToExit:
    // send the exit to the shell.  If it fails I really don't know what an
    // appropriate reaction would be so I ignore the return value and just
    // hope for the GOODBYE acknowledgement.  Note that if there are
    // running jobs when the destructor is called then we've only expressed
    // our intent to exit as soon as those jobs are done.
    (void)expwrite("exit; echo GOODBYE");
    (void)expect1("GOODBYE\r\n");

    if (p_shell=="ssh" || p_shell=="telnet") {
      // telnet is a bad boy.  Without the security of knowing what the
      // prompt will be once the first exit (closes the csh/tcsh created within
      // the telnet session) is sent we are effectively blind and just hoping
      // that we are correctly closing the connection.  The sleep call is just
      // a way of trying to let the first exit finish cleanly before issuing
      // the second one which causes the actual logout.
      sleep(1);
      (void)expwrite("exit");
      if (p_shell == "ssh") {
        // If background commands were issued in this shell, then waiting for
        // the regular ssh "connection closed" message will result in the
        // exit hanging.  Use the ssh verbose mesage about "exit-status"
        // because this happens before it waits on background jobs.
        // (void)expect1("Connection to * closed.");
        if (p_background)
          (void)expect1("exit-status reply");
        else
          (void)expect1("Connection to * closed.");
      } else
        (void)expect1("Connection closed");
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
  string path = "./ecmd";

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
       remShell.find("/rsh")==remShell.length()-4) || 
      remShell=="telnet" || remShell.find("telnet/")==0 ||
      remShell=="Globus" || remShell.find("Globus/")==0 ||
      remShell=="Globus-ssh" || remShell.find("Globus-ssh/")==0) {
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
      remShell=="rsh" || remShell.find("rsh/")==0 ||
      remShell=="telnet" || remShell.find("telnet/")==0 ||
      remShell=="Globus" || remShell.find("Globus/")==0 ||
      remShell=="Globus-ssh" || remShell.find("Globus-ssh/")==0) {
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
       theShell.find("/rsh")==theShell.length()-4) || 
      theShell=="telnet" || theShell.find("telnet/")==0 ||
      theShell=="Globus" || theShell.find("Globus/")==0 ||
      theShell=="Globus-ssh" || theShell.find("Globus-ssh/")==0)
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

bool RCommand::globusproxy(const string& password, string& errMessage)
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

  // This should finish quickly
  exp_timeout = RC_EXEC_TIMEOUT;

  static const char* command = "grid-proxy-init";
  static const char* minh = "-hours";
  static const char* hours = "336";   // 2 weeks

  char *argv[4];
  argv[0] = (char*)command;
  argv[1] = (char*)minh;
  argv[2] = (char*)hours;
  argv[3] = (char*)0;

  if (exp_loguser == 1) {
    cout << "globus proxy command:" << endl;
    for (int it=0; it<3; it++)
      cout << "arg " << it << ": " << argv[it] << endl;
    cout << "end globus proxy command" << endl; 
  }

  int theFid;
  if ((theFid = exp_spawnv((char*)command, argv)) <= 0) {
    errMessage = "Unable to spawn command ";
    errMessage += command;
    return false;
  }

  string output;
  bool done;
  do {
    done = true;  // be optimistic

    switch (exp_expectl(theFid, exp_glob, "Command not found", 1,
                                exp_glob, "execvp(", 1,
                                exp_glob, "Bad", 2,
                                exp_glob, "ERROR", 2,
                                exp_glob, "pass phrase for this identity:$", 3,
                                exp_end)) {
      case 1:
        errMessage = "Unable to find grid-proxy-init command";
        errMessage += " (not in the path?)";
        break;

      case 2:
        if (strlen(exp_buffer) > 2)
          exp_buffer[strlen(exp_buffer)-2] = '\0';
        output = (exp_buffer != NULL)? exp_buffer: "";
        errMessage = "Unsuccessful Globus proxy authentication";
        if (output != "")
          errMessage += "\nError output: " + output;
        break;

      case 3:
        // Globus requires 4+ character pass phrases so send it a bogus
        // pass phrase so it doesn't go into an infinite loop prompting for
        // one that's too short
        exp_elide(password.c_str());
        if (password.length() < 4) {
          if (!fidwrite(theFid, "XXXX", errMessage)) break;
        }
        else if (!fidwrite(theFid, password, errMessage)) break;

        done = false;
        break;

      case EXP_EOF:
        // Successful login
        status = true;
        break;

      case EXP_TIMEOUT:
        errMessage = "Timeout running Globus proxy authentication";
        break;

      default:
        errMessage = "Unrecognized Globus proxy authentication failure";
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


///////////////////////////////////////////////////////////////////////////////
//
//  Description
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
bool RCommand::globusrun(const string& command, string& output,
                         string& errMessage,
                         const string& machine, const string& contact,
                         const string& password, const string& queueRSL)
{
  if (!RCommand::globusproxy(password, errMessage))
    return false;

  bool status = false;
  string globuscn;
  string globusrsl;

  if (getenv("ECCE_RCOM_DEBUGGING"))
    exp_is_debugging = 1;
  else
    exp_is_debugging = 0;

  if (getenv("ECCE_RCOM_LOGMODE"))
    exp_loguser = 1;
  else
    exp_loguser = 0;

  // Bump up timeout because we can't predict how long a command takes
  exp_timeout = RC_COMMAND_TIMEOUT;

  // Set the machine and user name variables for the benefit of error
  // messages (the values passed in are const)
  string theMachine = (machine=="" || machine=="-f" || machine=="system")?
                       RCommand::whereami(): machine;

  char *argv[MAXARGS];
  int argc = 1;

  // local user/machine connections don't need a remote shell
  static const char* theShell = "globusrun";
  static const char* minq = "-q";
  static const char* minb = "-b";
  static const char* minr = "-r";

  argv[argc++] = (char*)minq;
  argv[argc++] = (char*)minb;
  argv[argc++] = (char*)minr;

  globuscn = machine;
  if (contact != "")
    globuscn += "/" + contact;
  argv[argc++] = strdup((char*)globuscn.c_str());

  globusrsl = RCommand::globusRSL(command, queueRSL);
  argv[argc++] = strdup((char*)globusrsl.c_str());

  argv[0] = (char*)theShell;
  argv[argc] = (char*)0;

  if (exp_loguser == 1) {
    cout << "globusrun command:" << endl;
    for (int it=0; it<argc; it++)
      cout << "arg " << it << ": " << argv[it] << endl;
    cout << "end globusrun command" << endl; 
  }

  int theFid;
  if ((theFid = exp_spawnv((char*)theShell, argv)) <= 0) {
    errMessage = "Unable to run remote shell ";
    errMessage += theShell;
    errMessage += " (not in the path?)";
    return false;
  }

  bool done;
  do {
    done = true;  // be optimistic

    switch (exp_expectl(theFid, exp_glob, "globusrun: Command not found", 1,
                                exp_glob, "execvp(", 1,
                                exp_glob, "denied", 2,
                                exp_glob, "failed", 2,
                                exp_glob, " closed", 2,
                                exp_glob, "remote server failed", 2,
                                exp_glob, "Bad host name", 3,
                                exp_glob, "Unknown host", 3,
                                exp_glob, "error code*\n", 4,
                                exp_end)) {
      case 1:
        errMessage = "Unable to find remote shell ";
        errMessage += theShell;
        errMessage += " (not in the path?)";
        break;

      case 2:
        errMessage = "Permission to run shell ";
        errMessage += theShell;
        errMessage += " denied for " + machine;
        break;

      case 3:
        errMessage = "Unknown or unavailable host " + machine;
        break;

      case 4:
        if (strlen(exp_buffer) > 2)
          exp_buffer[strlen(exp_buffer)-2] = '\0';
        output = (exp_buffer != NULL)? exp_buffer: "";
        errMessage = "Could not run " + command;
        if (output != "")
          errMessage += "\nError output: " + output;
        break;

      case EXP_EOF:
        // Successful login without password
        status = true;
        if (strlen(exp_buffer) > 2)
          exp_buffer[strlen(exp_buffer)-2] = '\0';
        output = (exp_buffer != NULL)? exp_buffer: "";
        break;

      case EXP_TIMEOUT:
        errMessage = "Timeout running remote shell ";
        errMessage += theShell;
        errMessage += " for " + machine;
        break;

      default:
        errMessage ="Unrecognized authentication failure running remote shell ";
        errMessage += theShell;
        errMessage += " for " + machine;
        break;
    }
  } while (!done);

  // Must wait for EOF before closing descriptor
  (void)wait(NULL);

  // Set timeout back to normal
  exp_timeout = RC_EXEC_TIMEOUT;

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

  string cmd;
  if (filename == "~")
    cmd = "if (-" + opone + " " + filename + "/) echo TRUE";
  else
    cmd = "if (-" + opone + " " + filename + ") echo TRUE";

  if (!expwrite(cmd)) return false;

  return expect2("\n*TRUE*\r\n+go+$", "\r\n+go+$")==1;
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
  // Because cd is interpreted by the C shell it doesn't work to append an
  // echo $status to the end.  The only way to recognize success is to check
  // if exists and then cd to it checking if an error string is output.

  if (!p_connected) return false;

  // save a microsecond by calling fileOp directly
  if (!fileOp("d", directory)) {
    p_errMessage = "Directory " + directory + " does not exist";
    return false;
  }

  string cdcmd = "cd " + directory;

  if (!expwrite(cdcmd)) return false;
  switch (expect1("\r\n+go+$")) {
    case 1:
      *exp_match = '\0';
      break;

    case EXP_EOF:
      p_errMessage = "Unexpected termination of remote shell executing cd to "
                     + directory;
      break;

    case EXP_TIMEOUT:
      p_errMessage = "Unexpected timeout executing cd to " + directory;
      break;

    default:
      p_errMessage = "Unexpected output executing cd to " + directory;
  }

  cdcmd += "\r\n";
  RCommand::expMungedOutputFix();
  char* line = strstr(exp_buffer, cdcmd.c_str());
  string output = (line != NULL)? line+cdcmd.length(): "";

  return output=="";
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

bool RCommand::execout(const string& command, string& output,
                       const string& errorMessage, const int& timeout)
{
  if (!p_connected) return false;

  bool status = false;
  string cmdstat = command;

  cmdstat.append("; echo CMDSTAT=$status");

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

  char* line = strstr(exp_buffer, "$status\r\n");

  if (line != NULL)
    output = line+9;
  else if ((line = strstr(exp_buffer, "$status\r")) != NULL)
    // this fixes some weird problem that occured on a Dell Linux workstation
    output = line+10;
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

  bool status = false;

  if (!expwrite(command + "&; sleep 2")) return false;

  // This implementation trades off reliability of the return status for
  // quick turnaround.  Since the "Exit" message from a background
  // command is an asynchronous event I would have to wait for a timeout
  // to occur to be fairly certain the command succeeded.  Even this wouldn't
  // be guaranteed reliable.  There are also problems relying on the timeout
  // if the command happens to finish (say an xterm started in the background
  // that the user immediately closes) before the timeout period elapses.
  // I do attempt to catch background command exits when the "Exit" comes back
  // prior to the shell prompt.  But, for the most part, developers using
  // execbg should not depend on the return value being correct like they
  // can with the non-background exec methods.

  // GDB 7/15/01  Thought of a little trick to get a semi-reliable return value
  // from execbg.  If you put the remote connection to sleep just after
  // issuing the background command then any Exit messages from the command
  // will be queued and output before the prompt is returned.  It's not 100%
  // guaranteed reliable (I thought 2 seconds would be the best compromise
  // between turnaround and reliability) but it is significantly better than
  // never getting a failure.  This makes the comments above obsolete but I
  // wanted to preserve them so that all impacts were understood.

  switch (expect2("Exit*\r\n+go+", "\r\n+go+")) {
    case 1:
      if (errorMessage != "")
        p_errMessage = errorMessage;
      else
        p_errMessage = "Failed executing background command " + command;
      break;

    case 2:
      p_background = true;
      status = true;
      *exp_match = '\0';
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
      p_errMessage = "Unexpected output executing background command "+ command;
  }

  RCommand::expMungedOutputFix();

  char* line = strstr(exp_buffer, "; sleep 2\r\n");

  if (line != NULL)
    output = line+11;
  else if ((line = strstr(exp_buffer, "; sleep 2\r")) != NULL)
    // this fixes some weird problem that occured on a Dell Linux workstation
    output = line+12;
  else if (status)
    output = "";
  else
    output = exp_buffer;

  return status;
}

bool RCommand::isOpen(void)
{
  if (p_connected) {
    p_connected = exec("date");
    if (!p_connected)
      p_errMessage = "Opened remote shell on " + p_machine +
                     ", but unable to process new commands";
  } else
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
        errMessage = "Globus remote copy authentication failure";
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
          passCmd = "./passdialog password " + theMachine + " " + theUser;
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
        passCmd = "./passdialog passcode " + theMachine + " " + theUser;
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
          passCmd = "./passdialog password " + theMachine + " " + theUser;
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
        passCmd = "./passdialog passcode " + theMachine + " " + theUser;
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

  bool isRemote = RCommand::isRemote(machine, remShell, userName);

  int argc = 0;
  char** argv = (char**)malloc((numFiles+MAXARGS) * sizeof(char*));

  string proxyAuth;
  string theCopy = RCommand::copyCommand(remShell, isRemote, machine,
                                         userName, proxyAuth, argc, argv);

  if (theCopy!="ftp" && theCopy!="sftp" && isRemote) {
    string fromFileBaseStr = "";
    if (theCopy!="globus-rcp" && userName!="")
      fromFileBaseStr = userName + "@";

    fromFileBaseStr += machine + ":";

    if (theCopy == "globus-rcp") {
      if (!RCommand::globusproxy(password, errMessage))
        return false;

      theCopy = "scp";
      argv[0] = strdup((char*)theCopy.c_str());
    } else if (proxyAuth != "") {
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

  bool isRemote = RCommand::isRemote(machine, remShell, userName);

  int numFiles;
  for (numFiles=0; fromFiles[numFiles]!=NULL; numFiles++);

  int argc = 0;
  char** argv = (char**)malloc((numFiles+MAXARGS) * sizeof(char*));

  string proxyAuth;
  string theCopy = RCommand::copyCommand(remShell, isRemote, machine,
                                         userName, proxyAuth, argc, argv);

  if (theCopy!="ftp" && theCopy!="sftp" && isRemote) {
    string fromFileBaseStr = "";
    if (theCopy!="globus-rcp" && userName!="")
      fromFileBaseStr = userName + "@";

    fromFileBaseStr += machine + ":";

    if (theCopy == "globus-rcp") {
      if (!RCommand::globusproxy(password, errMessage))
        return false;

      theCopy = "scp";
      argv[0] = strdup((char*)theCopy.c_str());
    } else if (proxyAuth != "") {
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

  char **fromFiles = (char**)malloc(numFiles * sizeof(char*));
  for (it=0; it<numFiles-1; it++)
    fromFiles[it] = (char*)va_arg(ap, char*);
  fromFiles[numFiles-1] = NULL;

  string toFile;
  bool isRemote = RCommand::isRemote(machine, remShell, userName);

  int argc = 0;
  char** argv = (char**)malloc((numFiles+MAXARGS) * sizeof(char*));

  string proxyAuth;
  string theCopy = RCommand::copyCommand(remShell, isRemote, machine,
                                         userName, proxyAuth, argc, argv);

  if (theCopy=="ftp" || theCopy=="sftp" || theCopy=="cp")
    toFile = (char*)va_arg(ap, char*);
  else {
    toFile = "";
    if (theCopy!="globus-rcp" && userName!="")
      toFile = userName + "@";
    toFile += machine + ":" + (char*)va_arg(ap, char*);

    if (isRemote && theCopy=="globus-rcp") {
      if (!RCommand::globusproxy(password, errMessage))
        return false;

      theCopy = "scp";
      argv[0] = strdup((char*)theCopy.c_str());
    } else if (proxyAuth != "") {
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

  int numFiles;
  if (!RCommand::globFiles(fromFiles, globbedFiles, numFiles))
    return false;

  string fullToFile;
  bool isRemote = RCommand::isRemote(machine, remShell, userName);

  int argc = 0;
  char* argv[MAXARGS];

  string proxyAuth;
  string theCopy = RCommand::copyCommand(remShell, isRemote, machine,
                                         userName, proxyAuth, argc, argv);

  if (theCopy=="ftp" || theCopy=="sftp" || theCopy=="cp")
    fullToFile = toFile;
  else {
    fullToFile = "";
    if (theCopy!="globus-rcp" && userName!="")
      fullToFile = userName + "@";
    fullToFile += machine + ":" + toFile;

    if (isRemote && theCopy=="globus-rcp") {
      if (!RCommand::globusproxy(password, errMessage))
        return false;

      theCopy = "scp";
      argv[0] = strdup((char*)theCopy.c_str());
    } else if (proxyAuth != "") {
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

      cmdstat = "dd ibs=1 of=" + fullToFile + " count=" + nreadstr + "; echo CMDSTAT=$status";

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

        cmd = "wc -lc " + globbedFile;
        execout(cmd, wcstr);
        lines = (int)strtol((char*)wcstr.c_str(), &endptr, 10);
        bytes = (int)strtol(endptr, NULL, 10);

        ofstream os(fullToFile.c_str());

        if (os) {
          cmd = "cat " + globbedFile + "; echo CMDSTAT=$status";
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

