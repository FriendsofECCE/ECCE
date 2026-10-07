// Run Management's Tail (TailSource): the file's last lines, then lines as
// they are appended, then nothing left running on the machine once closed.
//
//   testTail                      a local machine (DirectTransport); ctest
//   testTail --remote MACHINE USER FILE APPEND-COMMAND
//                                 a registered ssh machine; FILE must hold
//                                 lines A, B, TAILLINE, and APPEND-COMMAND
//                                 appends a line NEWLINE-<pid> to it by other
//                                 means.  tests/transport/sshd/tail_test.sh
//                                 runs it and counts the logins.

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

#include "comm/TailSource.H"

using namespace std;

static int failures = 0;

static void check(const string& name, bool ok, const string& detail = "")
{
  cout << (ok ? "ok   " : "FAIL ") << name << endl;
  if (!ok) {
    failures++;
    if (!detail.empty()) cout << "     [" << detail << "]" << endl;
  }
}

static void put(const string& f, const string& text, bool append = false)
{
  ofstream(f.c_str(), append ? ios::app : ios::trunc) << text;
}

// Reads until text holds want or secs pass; false at the stream's end.
static bool waitFor(TailSource& t, string& text, const string& want, int secs)
{
  time_t end = time(0) + secs;
  while (text.find(want) == string::npos && time(0) < end) {
    if (!t.read(text)) return text.find(want) != string::npos;
    usleep(50000);
  }
  return text.find(want) != string::npos;
}

static int countLines(const string& s)
{
  int n = 0;
  for (size_t i = 0; i < s.size(); i++) if (s[i] == '\n') n++;
  return n;
}

static bool tailRunning(const string& path)
{
  string cmd = "pgrep -f -- " + TailSource::shPath("tail -n [0-9]* -F -- " + path) +
               " >/dev/null";
  return system(cmd.c_str()) == 0;
}

// No tail left running on this host that follows path.
static bool noTailFor(const string& path)
{
  for (int i = 0; i < 30; i++) {
    if (!tailRunning(path)) return true;
    usleep(100000);
  }
  return false;
}

static int remote(char** argv)
{
  const string machine = argv[0], user = argv[1], file = argv[2],
               append = argv[3];
  TailSource t;
  string error, text;
  bool missing = true;
  bool ok = t.open(machine, "", user, file, 100, error, &missing);
  check("remote: opened", ok, error);
  if (!ok) return 1;
  cout << "backend " << t.backend() << endl;
  check("remote: the file was found", !missing);
  check("remote: existing lines arrive", waitFor(t, text, "TAILLINE\n", 20), text);
  const string marker = "NEWLINE-" + to_string(getpid());
  setenv("TAIL_MARKER", marker.c_str(), 1);
  check("remote: the append command ran", system(append.c_str()) == 0);
  check("remote: an appended line arrives", waitFor(t, text, marker, 20), text);
  t.close();
  check("remote: closed", t.fd() == -1);
  cout << "--- received\n" << text << "---" << endl;
  return failures ? 1 : 0;
}

int main(int argc, char** argv)
{
  if (argc == 6 && string(argv[1]) == "--remote") return remote(argv + 2);

  // The quoting, as the script reaches sh.
  check("shPath quotes", TailSource::shPath("/a b/it's") == "'/a b/it'\\''s'");
  check("shPath keeps ~", TailSource::shPath("~/x y") == "\"$HOME\"/'x y'");

  char tmpl[] = "/tmp/ecce-tailXXXXXX";
  const string tmp = mkdtemp(tmpl);
  const string home = tmp + "/home", eh = tmp + "/ecce";
  mkdir(home.c_str(), 0755);
  mkdir((home + "/.ECCE").c_str(), 0755);
  mkdir(eh.c_str(), 0755);
  if (symlink(ECCE_SOURCE_DIR "/siteconfig", (eh + "/siteconfig").c_str())) {}
  if (symlink(ECCE_SOURCE_DIR "/data", (eh + "/data").c_str())) {}
  put(home + "/.ECCE/MyMachines",
      "tlocal\tlocalhost\tt\tt\tt\t1:1\tssh\tna\tna\n");
  setenv("ECCE_HOME", eh.c_str(), 1);
  setenv("ECCE_REALUSERHOME", home.c_str(), 1);
  if (!getenv("ECCE_REALUSER")) {
    const char* u = getenv("USER");
    setenv("ECCE_REALUSER", u ? u : "nobody", 1);
  }

  const string dir = tmp + "/run dir 'q'";
  mkdir(dir.c_str(), 0755);
  const string file = dir + "/out file.txt";
  string all;
  for (int i = 1; i <= 50; i++) all += "line " + to_string(i) + "\n";
  put(file, all);

  {
    TailSource t;
    string error, text;
    bool missing = true;
    bool ok = t.open("tlocal", "", "", file, 10, error, &missing);
    check("local: opened", ok, error);
    check("local: backend is local", t.backend() == "local", t.backend());
    check("local: the file was found", ok && !missing);
    check("local: the last lines arrive", waitFor(t, text, "line 50\n", 10), text);
    usleep(300000);
    t.read(text);
    check("local: only the last 10 lines", countLines(text) == 10 &&
          text.compare(0, 8, "line 41\n") == 0, text);
    put(file, "appended 1\nappended 2\n", true);
    check("local: appended lines arrive", waitFor(t, text, "appended 2\n", 10), text);
    check("local: tail runs while open", tailRunning(file));
    time_t before = time(0);
    t.close();
    check("local: closing does not wait out the grace period",
          time(0) - before <= 1);
    check("local: closed", t.fd() == -1);
    check("local: no tail left running", noTailFor(file));
  }
  {
    // A queued job: the output file comes later.
    const string later = dir + "/later.out";
    TailSource t;
    string error, text;
    bool missing = false;
    bool ok = t.open("tlocal", "", "", later, 10, error, &missing);
    check("local, no file yet: opened", ok, error);
    check("local, no file yet: reported missing", missing);
    usleep(500000);
    put(later, "it came\n");
    check("local, no file yet: its lines arrive once it exists",
          waitFor(t, text, "it came\n", 15), text);
    t.close();
    check("local, no file yet: no tail left running", noTailFor(later));
  }
  {
    TailSource t;
    string error;
    check("unregistered machine is refused",
          !t.open("nosuch", "", "", file, 10, error) &&
          error.find("not currently registered") != string::npos, error);
  }

  string rm = "rm -rf " + TailSource::shPath(tmp);
  if (system(rm.c_str())) {}
  cout << (failures ? "FAILED" : "PASSED") << endl;
  return failures ? 1 : 0;
}
