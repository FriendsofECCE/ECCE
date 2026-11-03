// OutputCopyCheck: what the job store makes of the outputs it copied back
// (#732): the host size on the monitor's count token, a short or missing
// copy, the normal-end line, and the reason the state then gives.

#include <cstdio>
#include <iostream>
#include <string>
#include <unistd.h>

#include "comm/OutputCopyCheck.H"

using namespace std;
using namespace OutputCopyCheck;

static int failures = 0;
static void check(const string& name, bool ok)
{
  cout << (ok ? "ok   " : "FAIL ") << name << endl;
  if (!ok) failures++;
}

static string writeFile(const string& text)
{
  char tmpl[] = "/tmp/ecce-occ.XXXXXX";
  int fd = mkstemp(tmpl);
  if (fd >= 0) {
    if (!text.empty() && write(fd, text.data(), text.size()) < 0) {}
    close(fd);
  }
  return tmpl;
}

int main()
{
  long long host;
  check("old count token: count 1", parseCountToken("1", host) == 1);
  check("old count token: no size", host == -1);
  check("new count token: count 1", parseCountToken("1:59941305", host) == 1);
  check("new count token: size", host == 59941305LL);
  check("count token with a zero size", parseCountToken("1:0", host) == 1 && host == 0);
  check("empty count token", parseCountToken("", host) == 0 && host == -1);

  check("missing copy is short, host unknown", copyIsShort(-1, -1));
  check("empty copy of a 60 MB file is short", copyIsShort(0, 59941305));
  check("partial copy is short", copyIsShort(1000, 2000));
  check("whole copy is not short", !copyIsShort(2000, 2000));
  check("empty copy, host unknown: not short", !copyIsShort(0, -1));
  check("empty file on the host too: not short", !copyIsShort(0, 0));

  check("ORCA marker", normalTerminationMarker("ORCA") == "ORCA TERMINATED NORMALLY");
  check("CP2K marker", normalTerminationMarker("CP2K") == "PROGRAM ENDED AT");
  check("NWChem has none", normalTerminationMarker("NWChem").empty());

  string good = writeFile(string(5 * 1024 * 1024, 'x') +
                          "\n                             ****ORCA TERMINATED NORMALLY****\n");
  string cut = writeFile("ORCA output cut off at the optimisation cycle limit\n");
  string empty = writeFile("");
  check("marker found at the end of a 5 MB output",
        hasMarker(good, "ORCA TERMINATED NORMALLY"));
  check("no marker in a cut-off output", !hasMarker(cut, "ORCA TERMINATED NORMALLY"));
  check("no marker in an empty output", !hasMarker(empty, "ORCA TERMINATED NORMALLY"));
  check("no marker in a missing output",
        !hasMarker("/nonexistent/orca.orcaout", "ORCA TERMINATED NORMALLY"));
  check("a code without a marker always passes", hasMarker(empty, ""));
  check("localSize of an empty file", localSize(empty) == 0);
  check("localSize of a missing file", localSize("/nonexistent/x") == -1);
  unlink(good.c_str());
  unlink(cut.c_str());
  unlink(empty.c_str());

  string r = incompleteOutputReason("orca.orcaout", "ORCA TERMINATED NORMALLY",
                                    0, 59941305);
  check("empty copy reason: " + r,
        r.find("orca.orcaout arrived empty") != string::npos &&
        r.find("57 MB in the run directory") != string::npos &&
        r.find("ecce-fetch") != string::npos);
  r = incompleteOutputReason("cp2k.cp2kout", "", 1000 * 1024, 2000 * 1024);
  check("short copy reason: " + r, r.find("has 1000 KB of 2000 KB") != string::npos);
  r = incompleteOutputReason("orca.orcaout", "ORCA TERMINATED NORMALLY", 500, -1);
  check("no-marker reason: " + r,
        r.find("has no 'ORCA TERMINATED NORMALLY'") != string::npos);
  r = incompleteOutputReason("orca.orcaout", "", -1, -1);
  check("missing copy reason: " + r, r.find("did not arrive") != string::npos);

  check("a failed reconnect login is a contact reason",
        isContactReason("Could not reconnect to the job (The login to "
                        "kebnekaise.hpc2n.umu.se was cancelled or not "
                        "answered.); ECCE tries again"));
  check("the code's reason is not",
        !isContactReason("the code stopped without converging (exit code 241)"));

  cout << (failures ? "FAILED" : "PASSED") << endl;
  return failures ? 1 : 0;
}
