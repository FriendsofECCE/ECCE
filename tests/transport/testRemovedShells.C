// Machines registered with a removed remote shell (telnet, Globus,
// Globus-ssh, rsh, rcp, and anything remote_shells.site used to define) must
// fail with a message that says what to do, through the channel connection
// errors already use, and must not reach any spawn.

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

#include "comm/RCommand.H"
#include "tdat/RefMachine.H"

using namespace std;

static int failures = 0;

static void check(bool ok, const string& what)
{
  if (!ok) { cout << "FAIL: " << what << endl; failures++; }
}

int main()
{
  // A private ECCE_HOME carrying one site-defined shell.
  char tmpl[] = "/tmp/removedshellsXXXXXX";
  if (!mkdtemp(tmpl)) { cout << "mkdtemp failed" << endl; return 1; }
  const string home = tmpl;
  mkdir((home + "/siteconfig").c_str(), 0755);
  { ofstream f((home + "/siteconfig/remote_shells.site").c_str());
    f << "mysiteshell: /bin/false\n"; }
  setenv("ECCE_HOME", home.c_str(), 1);

  // mysiteshell is defined in the site file above, which is now ignored.
  const char* removed[] = {"telnet", "Globus", "Globus-ssh", "telnet/ftp",
                           "rsh", "rsh/ftp", "rcp", "mysiteshell", "sftp",
                           "scp", "ftp", "krsh/ftp", 0};
  for (int i = 0; removed[i]; i++) {
    const string shell = removed[i];
    const string name = shell.substr(0, shell.find('/'));

    RCommand rc("nosuchhost.invalid", shell, "csh", "nobody", "x");
    check(!rc.isOpen(), shell + ": must not open");
    const string err = rc.commError();
    check(err.find("'" + name + "' is no longer supported") != string::npos,
          shell + ": message names the shell (" + err + ")");
    check(err.find("choose ssh") != string::npos,
          shell + ": message says what to do (" + err + ")");

    string errMessage;
    check(!RCommand::get(errMessage, "nosuchhost.invalid", shell, "nobody",
                         "x", 2, "/etc/hostname", "/tmp"),
          shell + ": get must fail");
    check(errMessage == err, shell + ": get reports the same message");
    check(!RCommand::put(errMessage, "nosuchhost.invalid", shell, "nobody",
                         "x", 2, "/etc/hostname", "/tmp"),
          shell + ": put must fail");
  }

  // The supported shells are not reported as removed.
  const char* kept[] = {"", "ssh", "sshpass", "ssh/ftp", 0};
  for (int i = 0; kept[i]; i++)
    check(RCommand::removedShellMessage(kept[i]).empty(),
          string("'") + kept[i] + "' is still supported");

  // sshpass and ssh/ftp are plain ssh now: a machine saved with either
  // reads back as ssh, and everything else is left alone.
  check(RefMachine::sshFamilyName("sshpass") == "ssh", "sshpass reads as ssh");
  check(RefMachine::sshFamilyName("ssh/ftp") == "ssh", "ssh/ftp reads as ssh");
  check(RefMachine::sshFamilyName("ssh") == "ssh", "ssh stays ssh");
  check(RefMachine::sshFamilyName("telnet") == "telnet", "telnet stays telnet");

  unlink((home + "/siteconfig/remote_shells.site").c_str());
  rmdir((home + "/siteconfig").c_str());
  rmdir(home.c_str());

  if (failures == 0) cout << "removed shells: ok" << endl;
  return failures ? 1 : 0;
}
