/*
 * ecce-fetch [--list] <calculation path or URL> [pattern ...]
 *
 * Copies files from the calculation's run directory on its compute machine
 * into its stored Outputs, for a job whose copy-back missed them. The
 * request goes to the Organizer over the session's broker and uses the login
 * Reconnect uses, so a password is asked for in the Organizer.
 *
 * Without patterns: *.cube *.cub *.pdos. --list only lists the directory.
 * Prints "name: fetched (N bytes)", "pattern: no file matched" or
 * "calc: not fetched: reason". Exit 0: all fetched; 1: some not (or no
 * answer); 2: no running ECCE session; 3: the Organizer is busy with a
 * Launch Selected batch; 64: usage.
 */
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <climits>
#include <thread>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "tdat/AuthCache.H"
#include "util/Ecce.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/JMSSubscriber.H"
#include "util/LocalData.H"
#include "util/MqttLink.H"

using std::string;
using std::vector;

namespace {

const int kAckSeconds = 8;          // an Organizer answers at once
const int kBatchSeconds = 5 * 60;   // each cancel is a login round trip

string gReplyId;
bool gAck = false, gDone = false, gBusy = false;
vector<string> gLines;

void replyCB(JMSMessage& msg)
{
  if (msg.getProperty("reply_id") != gReplyId) return;
  string kind = msg.getProperty("kind");
  if (kind == "ack") { gAck = true; return; }
  int n = msg.getIntProperty("n");
  for (int i = 0; i < n; i++) gLines.push_back(msg.getProperty("line" + std::to_string(i)));
  gAck = true;
  if (kind == "busy") gBusy = true;
  gDone = true;
}

int notRunning()
{
  std::cerr << "ECCE is not running: start ECCE first" << std::endl;
  return 2;
}

// The session this account runs: the id in the environment, else the newest
// one (what ecce-<app> wrappers join). "" if none has a broker file.
string sessionKey()
{
  string key = Ecce::sessionKey();
  if (key.empty()) {
    std::ifstream pointer(Ecce::sessionPointerFile());
    string id;
    if (pointer && std::getline(pointer, id) && Ecce::validSessionId(id)) {
      key = Ecce::sessionKeyFor(id);
      setenv("ECCE_SESSION_ID", id.c_str(), 1);
    }
  }
  struct stat st;
  if (key.empty() ||
      stat((string(Ecce::realUserPrefPath()) + "broker_" + key).c_str(), &st) != 0)
    return "";
  return key;
}

// A path inside the data folder is the calculation's URL there.
bool toUrl(const string& arg, string& url, string& why)
{
  if (arg.find("://") != string::npos) { url = arg; return true; }
  string root = LocalData::dir();
  if (root.empty()) {
    why = "this session uses a data server: give the calculation's URL";
    return false;
  }
  char buf[PATH_MAX], rbuf[PATH_MAX];
  if (!realpath(arg.c_str(), buf)) { why = "no such folder"; return false; }
  string path = buf;
  string base = realpath(root.c_str(), rbuf) ? string(rbuf) : root;
  if (path.compare(0, base.size() + 1, base + "/") != 0) {
    why = "not inside the data folder " + base;
    return false;
  }
  url = "file://" + path;
  return true;
}

}  // namespace

int main(int argc, char **argv)
{
  // Run from a terminal there is no wrapper, so ECCE_REALUSERHOME is unset.
  if (!getenv("ECCE_REALUSERHOME")) {
#ifdef _WIN32
    const char *h = getenv("USERPROFILE");
    if (h) _putenv_s("ECCE_REALUSERHOME", h);
#else
    const char *h = getenv("HOME");
    if (h) setenv("ECCE_REALUSERHOME", h, 0);
#endif
  }

  bool listOnly = false;
  vector<string> args;
  for (int i = 1; i < argc; i++) {
    string a = argv[i];
    if (a == "--list" && args.empty()) listOnly = true;
    else args.push_back(a);
  }
  if (args.empty()) {
    std::cerr << "usage: ecce-fetch [--list] <calculation path or URL> [pattern ...]"
              << std::endl;
    return 64;
  }
  vector<string> patterns(args.begin() + 1, args.end());
  if (patterns.empty() && !listOnly) patterns = {"*.cube", "*.cub", "*.pdos"};
  for (size_t i = 0; i < patterns.size(); i++)
    if (patterns[i].empty() || patterns[i][0] == '.' ||
        patterns[i].find('/') != string::npos) {
      std::cerr << "ecce-fetch: a pattern is a file name in the run directory, "
                   "without '/' and not starting with '.': " << patterns[i] << std::endl;
      return 64;
    }

  if (sessionKey().empty()) return notRunning();

  bool allOk = true;
  string url, why;
  if (!toUrl(args[0], url, why)) {
    std::cout << args[0] << ": not fetched: " << why << std::endl;
    return 1;
  }

  // Needed for a session on a TCP broker (central server), where the
  // account's data server login is the broker login.
  MqttLink::setCredentialProvider(AuthCache::brokerCredential);

  JMSSubscriber sub("EcceFetch");
  if (!sub.messagingEnabled() || !sub.subscribe("ecce_fetch_reply", replyCB) ||
      !sub.startSubscriber() || sub.getSocketID() < 0)
    return notRunning();

  using clock = std::chrono::steady_clock;
  auto end = clock::now() + std::chrono::seconds(kAckSeconds);
  while (!MqttLink::instance().connected() && clock::now() < end) std::this_thread::sleep_for(std::chrono::milliseconds(50));
  if (!MqttLink::instance().connected()) return notRunning();

  gReplyId = std::to_string(getpid()) + "-" + std::to_string(time(0));
  JMSPublisher pub("EcceFetch");
  JMSMessage *msg = pub.newMessage();
  msg->addProperty("reply_id", gReplyId);
  msg->addProperty("url", url);
  msg->addProperty("list", listOnly ? "1" : "0");
  msg->addIntProperty("n", (int)patterns.size());
  for (size_t i = 0; i < patterns.size(); i++)
    msg->addProperty("pattern" + std::to_string(i), patterns[i]);
  bool sent = pub.publish("ecce_fetch_request", *msg);
  delete msg;
  if (!sent) return notRunning();

  end = clock::now() + std::chrono::seconds(kAckSeconds);
  bool timedOut = false;
  while (!gDone) {
    if (clock::now() >= end) { timedOut = true; break; }
    sub.processMessage();
    if (gDone) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (gAck && !gDone) end = clock::now() + std::chrono::seconds(kBatchSeconds);
  }
  if (timedOut) {
    if (!gAck) return notRunning();
    std::cerr << "ecce-fetch: no answer from ECCE within "
              << kBatchSeconds / 60 << " minutes; see the Organizer for the "
                 "state of the calculations" << std::endl;
    return 1;
  }
  if (gBusy) {
    std::cerr << "ecce-fetch: ECCE is submitting a Launch Selected batch; "
                 "try again when it has finished" << std::endl;
    return 3;
  }

  bool anyDone = false;
  for (size_t i = 0; i < gLines.size(); i++) {
    size_t a = gLines[i].find('\t');
    size_t b = a == string::npos ? a : gLines[i].find('\t', a + 1);
    if (b == string::npos) { std::cout << gLines[i] << std::endl; allOk = false; continue; }
    string name = gLines[i].substr(0, a), ok = gLines[i].substr(a + 1, b - a - 1),
           text = gLines[i].substr(b + 1);
    if (ok == "ls") { std::cout << text << std::endl; anyDone = true; }
    else if (ok == "ok") { std::cout << name << ": fetched (" << text << " bytes)" << std::endl; anyDone = true; }
    else if (ok == "none") std::cout << name << ": no file matched" << std::endl;
    else { std::cout << name << ": not fetched: " << text << std::endl; allOk = false; }
  }
  //  A default pattern with no match (*.cub beside *.cube) is not a failure;
  //  fetching nothing at all is.
  return allOk && anyDone ? 0 : 1;
}
