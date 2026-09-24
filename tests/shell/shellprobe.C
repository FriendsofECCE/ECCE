///////////////////////////////////////////////////////////////////////////////
// shellprobe -- drive a real RCommand connection to the local machine and
// report, as PASS/FAIL lines, the behaviours the job-submission path depends
// on.  Built (not installed) for tests/shell; see tests/shell/README.md.
//
// Every check here corresponds to a bug that was real:
//
//   echo      #69 bug 2 -- JobStore::initMon() waits for a byte-for-byte echo
//             of the command it sent.  Under bash with readline enabled, a
//             command longer than the terminal width comes back with readline's
//             horizontal-scroll '<' marker instead of the real text, so the
//             match can never succeed and monitoring hangs forever.
//   glob      #59 -- the local connection hardcoded csh's "-f" (skip .cshrc),
//             which in bash means "disable globbing", silently breaking any
//             command with a wildcard in it.
//   rc        #59 -- "-f" as a command flag (csh's "-fc") likewise.
//   status    execout()'s exit-status reporting ($status vs $?).
//   bg        #69 bug 1 -- a backgrounded job must survive the connection
//             being torn down, even when it installs its own SIGHUP handler
//             (Gaussian does), which discards nohup's inherited SIG_IGN.
//
// Usage: shellprobe <shell> <workdir> [check ...]
///////////////////////////////////////////////////////////////////////////////
#include <unistd.h>
#include <stdlib.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
  using std::cout;
  using std::endl;
  using std::ifstream;
  using std::string;
  using std::vector;

#include "comm/RCommand.H"

static int failures = 0;

static void check(const string& name, bool ok, const string& detail = "")
{
  if (!ok) failures++;
  cout << (ok ? "PASS " : "FAIL ") << name;
  if (detail != "") cout << "  [" << detail << "]";
  cout << endl;
}

static string slurp(const string& path)
{
  ifstream in(path.c_str());
  string all, line;
  while (getline(in, line)) all += line + "|";
  return all;
}

static RCommand* connect(const string& shell)
{
  // Exactly how Launch::validateRemoteLogin() builds its connection: the
  // machine's own name, which RCommand::isRemote() resolves to the local
  // "spawn a shell on a pty" path rather than ssh.
  return new RCommand(RCommand::whereami(), "ssh", shell, "", "");
}

int main(int argc, char** argv)
{
  if (argc < 3) {
    cout << "usage: shellprobe <shell> <workdir> [check ...]" << endl;
    return 2;
  }
  string shell = argv[1];
  string dir = argv[2];
  vector<string> checks;
  for (int i = 3; i < argc; i++) checks.push_back(argv[i]);
  bool all = checks.empty();
  bool want_echo = all, want_glob = all, want_rc = all, want_bg = all;
  for (size_t i = 0; i < checks.size(); i++) {
    if (checks[i] == "echo") want_echo = true;
    else if (checks[i] == "glob") want_glob = true;
    else if (checks[i] == "rc") want_rc = true;
    else if (checks[i] == "bg") want_bg = true;
  }

  string output;

  {
    RCommand* rc = connect(shell);
    check("connect", rc->isOpen(), rc->commError());
    if (!rc->isOpen()) { delete rc; return 1; }

    if (!rc->cd(dir)) check("cd", false, rc->commError());

    if (want_rc) {
      bool ok = rc->execout("echo hello_from_" + shell, output);
      check("exec.output", ok && output.find("hello_from_" + shell) !=
                           string::npos, output);

      // A command that fails must be reported as failing ($? vs $status).
      output = "";
      ok = rc->execout("test -e definitely_not_here_4711", output);
      check("exec.failure-detected", !ok, output);
    }

    if (want_glob) {
      // The whole point of #59: the local connection used to run the shell
      // with csh's "-f", which bash reads as "disable globbing".
      output = "";
      bool ok = rc->execout("echo globme*.dat", output);
      check("glob.expanded",
            ok && output.find("globme1.dat") != string::npos &&
                  output.find("globme2.dat") != string::npos, output);
    }

    if (want_echo) {
      // JobStore::initMon()'s exact-echo handshake, verbatim in shape and
      // well past any plausible terminal width (readline's default is 80
      // when COLUMNS is unset, which it is for a pty nobody sized).
      string cmd = "echo LONG_COMMAND_ECHO_TEST_"
                   "perl_eccejobmonitor_-configFile_eccejobmonitor.conf_"
                   "-jobId_1000044_-bookmark_0_-parseTypes_ALL_"
                   "echo_eccejobmonitor_went_bye_bye_0123456789012345678901234";
      rc->exptimeout(10);
      bool wrote = rc->expwrite(cmd);
      int matched = wrote ? rc->expect1((cmd + "\r\n").c_str()) : -1;
      check("echo.exact-match", matched == 1,
            matched == 1 ? "" : "command echo never came back intact");
      rc->exptimeout(0);
      // Resynchronise: swallow whatever the echo command produced.
      output = "";
      (void)rc->execout("echo resync", output);
    }

    delete rc;
  }

  if (want_bg) {
    // Launch::launchNormal()'s path: submit in the background, then the
    // connection goes away while the job is still running.
    string log = dir + "/victim.log";
    (void)unlink(log.c_str());
    RCommand* rc = connect(shell);
    check("bg.connect", rc->isOpen(), rc->commError());
    if (rc->isOpen()) {
      if (!rc->cd(dir)) check("bg.cd", false, rc->commError());
      output = "";
      bool ok = rc->execbg("./victim_submit", output);
      check("bg.submitted", ok, output);
      check("bg.jobid-is-a-pid", output.find_first_not_of("0123456789") ==
            string::npos && output != "", "job id was \"" + output + "\"");
      sleep(2);
      delete rc;                 // tears the pty down, mid-job
    } else
      delete rc;

    // The victim runs for ~8s and writes COMPLETED if it is left alone.
    for (int i = 0; i < 40; i++) {
      string txt = slurp(log);
      if (txt.find("COMPLETED") != string::npos ||
          txt.find("GOT SIGHUP") != string::npos) break;
      sleep(1);
    }
    string txt = slurp(log);
    check("bg.survived-teardown", txt.find("COMPLETED") != string::npos, txt);
  }

  cout << (failures ? "RESULT FAIL " : "RESULT PASS ") << shell << endl;
  return failures ? 1 : 0;
}
