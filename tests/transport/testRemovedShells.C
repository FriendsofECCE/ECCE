// Machines registered with a removed remote shell (telnet, Globus,
// Globus-ssh) must fail with a message that says what to do, through the
// channel connection errors already use, and must not reach any spawn.

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

#include "comm/RCommand.H"

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

  const char* removed[] = {"telnet", "Globus", "Globus-ssh", "telnet/ftp", 0};
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
  const char* kept[] = {"", "ssh", "sshpass", "ssh/ftp", "rsh", "sftp",
                        "mysiteshell", 0};
  for (int i = 0; kept[i]; i++)
    check(RCommand::removedShellMessage(kept[i]).empty(),
          string("'") + kept[i] + "' is still supported");

  // A site-defined shell (remote_shells.site) still takes the pty path:
  // the session is attempted, so the failure is not the removed-shell one.
  {
    RCommand rc("nosuchhost.invalid", "mysiteshell", "csh", "nobody", "x");
    check(!rc.isOpen(), "unknown site shell cannot open");
    check(rc.commError().find("no longer supported") == string::npos,
          "site shell is not reported as removed");
  }

  unlink((home + "/siteconfig/remote_shells.site").c_str());
  rmdir((home + "/siteconfig").c_str());
  rmdir(home.c_str());

  if (failures == 0) cout << "removed shells: ok" << endl;
  return failures ? 1 : 0;
}
