// The session id and key (#233): the shell (ecce-session-lib.sh) and C++
// (Ecce::sessionKey) derivations must name the same files and topics for
// any HOST, and both generators must make well-formed, distinct ids.
//
//   session_key_test <path to ecce-session-lib.sh>

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <set>
#include <string>
#include <vector>

#include <mosquitto.h>

#include "util/Ecce.H"

using std::cout;
using std::endl;
using std::string;

static int g_fail = 0;
static string g_lib;

static void check(bool ok, const string& what)
{
  cout << (ok ? "PASS  " : "FAIL  ") << what << endl;
  if (!ok) g_fail++;
}

// Runs a bash snippet with the lib sourced, in this process's environment.
static string shell(const string& snippet, int* status = 0)
{
  string cmd = "bash -c '. \"$0\" && " + snippet + "' '" + g_lib + "'";
  FILE* p = popen(cmd.c_str(), "r");
  if (!p) return "<popen failed>";
  string out;
  char buf[256];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), p)) > 0) out.append(buf, n);
  int st = pclose(p);
  if (status) *status = WIFEXITED(st) ? WEXITSTATUS(st) : -1;
  return out;
}

static void setOrUnset(const char* name, const char* value)
{
  if (value) setenv(name, value, 1);
  else unsetenv(name);
}

static string printable(const string& s)
{
  string r;
  for (size_t i = 0; i < s.size(); i++) {
    unsigned char c = s[i];
    if (c >= 32 && c < 127) r += c;
    else {
      char b[8];
      snprintf(b, sizeof(b), "\\x%02x", c);
      r += b;
    }
  }
  return r;
}

static bool topicLevelOk(const string& key)
{
  if (key.empty() || key[0] == '$') return false;
  if (key.find_first_of("/+#") != string::npos) return false;
  string topic = "ecce/user/session/" + key + "/ecce_poll";
  return mosquitto_pub_topic_check(topic.c_str()) == MOSQ_ERR_SUCCESS &&
         mosquitto_sub_topic_check(topic.c_str()) == MOSQ_ERR_SUCCESS;
}

int main(int argc, char** argv)
{
  if (argc < 2) {
    cout << "usage: session_key_test <ecce-session-lib.sh>" << endl;
    return 2;
  }
  g_lib = argv[1];
  setenv("ECCE_REALUSERHOME", "/nonexistent/home", 1);

  const char* goodId = "0123456789abcdef";
  struct Case { const char* host; const char* ecceHost; const char* id; };
  const Case cases[] = {
    {"testhost", 0, goodId},
    {"beryllium.example.org", 0, goodId},
    {"my host", 0, goodId},
    {"a/b:c", 0, goodId},
    {"x+y#z", 0, goodId},
    {"$HOME", 0, goodId},
    {"$(id)`id`", 0, goodId},
    {"-n", 0, goodId},
    {"tab\there", 0, goodId},
    {"new\nline", 0, goodId},
    {"*", 0, goodId},
    {"'quoted'\"too\"", 0, goodId},
    {"\xc3\xbcn\xc3\xafc\xc3\xb8" "de", 0, goodId},   // UTF-8
    {"other", "override", goodId},
    {"other", "", goodId},          // empty ECCE_HOST: HOST
    {"", 0, goodId},                // empty HOST: the host name
    {0, 0, goodId},                 // no HOST: the host name
    {"testhost", 0, ""},            // invalid ids: no key on either side
    {"testhost", 0, "0123456789ABCDEF"},
    {"testhost", 0, "0123"},
    {"testhost", 0, "0123456789abcdef0"},
    {"testhost", 0, "0123456789abcdeg"},
    {"testhost", 0, "../../etc/passwd"},
    {"testhost", 0, 0},
  };

  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    const Case& c = cases[i];
    setOrUnset("HOST", c.host);
    setOrUnset("ECCE_HOST", c.ecceHost);
    setOrUnset("ECCE_SESSION_ID", c.id);
    string cxx = Ecce::sessionKey();
    int st = 0;
    string sh = shell("ecce_session_key", &st);
    string what = string("HOST=") + (c.host ? printable(c.host) : "<unset>") +
                  " ECCE_HOST=" + (c.ecceHost ? c.ecceHost : "<unset>") +
                  " id=" + (c.id ? printable(c.id) : "<unset>");
    bool valid = c.id && Ecce::validSessionId(c.id);
    if (valid) {
      check(st == 0 && sh == cxx && !cxx.empty(),
            what + ": shell and C++ agree on '" + printable(cxx) + "'");
      check(cxx.size() > 17 &&
            cxx.compare(cxx.size() - 17, 17, string("_") + c.id) == 0,
            what + ": the key ends in _<id>");
      check(topicLevelOk(cxx), what + ": the key is a valid MQTT topic level");
      string files = shell("STATEDIR=/s; ecce_broker_file; echo; "
                           "ecce_auth_file; echo; ecce_session_prefix");
      check(files == "/s/broker_" + cxx + "\n/s/authcache_" + cxx + "\n" +
                     cxx.substr(0, cxx.size() - 16),
            what + ": broker, credential file and prefix named from the key");
    } else {
      check(cxx.empty() && sh.empty() && st != 0,
            what + ": no key on either side");
    }
  }

  // The C++ generator.
  std::set<string> seen;
  bool formed = true;
  const int N = 100000;
  for (int i = 0; i < N; i++) {
    string id = Ecce::newSessionId();
    if (!Ecce::validSessionId(id)) {
      if (formed) cout << "      malformed: " << printable(id) << endl;
      formed = false;
    }
    seen.insert(id);
  }
  check(formed, "C++: 100000 ids are 16 lower-case hex characters");
  check((int)seen.size() == N, "C++: 100000 ids are distinct");

  // The shell generator, and ecce_session_ensure.
  std::set<string> shellSeen;
  bool shellFormed = true;
  for (int i = 0; i < 200; i++) {
    string id = shell("ecce_new_session_id");
    if (!Ecce::validSessionId(id)) shellFormed = false;
    shellSeen.insert(id);
  }
  check(shellFormed, "shell: 200 ids are 16 lower-case hex characters");
  check(shellSeen.size() == 200, "shell: 200 ids are distinct");

  setenv("ECCE_SESSION_ID", goodId, 1);
  check(shell("ecce_session_ensure; printf %s \"$ECCE_SESSION_ID\"") == goodId,
        "ecce_session_ensure keeps an inherited id");
  setenv("ECCE_SESSION_ID", "not-an-id", 1);
  string made = shell("ecce_session_ensure; printf %s \"$ECCE_SESSION_ID\"");
  check(Ecce::validSessionId(made), "ecce_session_ensure replaces a malformed id");
  unsetenv("ECCE_SESSION_ID");
  made = shell("ecce_session_ensure; printf %s \"$ECCE_SESSION_ID\"");
  check(Ecce::validSessionId(made), "ecce_session_ensure makes one when unset");

  cout << (g_fail ? "FAILED" : "ALL PASS") << endl;
  return g_fail ? 1 : 0;
}
