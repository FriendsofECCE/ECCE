// Drives a real local RCommand session and checks that a long command's
// echo comes back byte for byte (#143, #69 Bug 2).
//
// eccejobstore's initMon() sends the eccejobmonitor command and then
// waits, with no timeout, for exactly cmd + "\r\n".  A shell whose line
// editor redraws long lines (tcsh's editor, bash's readline) never
// produces that, and monitoring hangs forever.  This makes the same
// calls initMon() makes, against the shell named on the command line,
// with a finite timeout so a failure is a failure and not a hang.
//
// usage: testRCommandEcho <shell> [length]
// exit 0 = exact echo seen and the command ran; 1 = not; 2 = no session

#include <cstdlib>
#include <iostream>
#include <string>
using namespace std;

#include "comm/RCommand.H"

int main(int argc, char** argv)
{
  if (argc < 2) {
    cerr << "usage: testRCommandEcho <shell> [length]" << endl;
    return 2;
  }
  string shell = argv[1];
  size_t length = argc > 2 ? (size_t)atoi(argv[2]) : 400;

  RCommand rc("system", "", shell);
  if (!rc.isOpen()) {
    cout << "NO SESSION " << shell << ": " << rc.commError() << endl;
    return 2;
  }

  string arg;
  while (arg.length() < length)
    arg += " -arg /tmp/ecce_mca32/jobs/direct__da8YQu/nwchem.desc";

  // execout() slices its output out of the buffer after the command's
  // own echoed "$status"/"$?", so a mangled echo corrupts its results
  // too.  Checked first, so it does not depend on the match below.
  string out;
  string expect = "ECCE_EXECOUT" + arg;
  bool execOk = rc.execout("echo " + expect, out) &&
                out.substr(0, expect.length()) == expect;

  // Shaped like initMon()'s command: a long first part, then the
  // "did it die" echo.  No glob characters: expect1() matches with glob.
  string cmd = "echo ECCE_LONG_ECHO" + arg +
               "; echo eccejobmonitor_went_bye_bye";

  rc.exptimeout(10);
  if (!rc.expwrite(cmd)) {
    cout << "WRITE FAILED " << shell << endl;
    return 1;
  }
  bool echoed = rc.expect1((cmd + "\r\n").c_str()) == 1;
  bool ran = echoed &&
             rc.expect1("eccejobmonitor_went_bye_bye\r\n+go+") == 1;

  cout << (echoed && ran && execOk ? "PASS " : "FAIL ") << shell
       << " len=" << cmd.length()
       << " exact-echo=" << (echoed ? "yes" : "NO")
       << " ran=" << (ran ? "yes" : "NO")
       << " execout=" << (execOk ? "yes" : "NO") << endl;
  return echoed && ran && execOk ? 0 : 1;
}
