/*
 * ecce-reparse [--json] <calculation path or URL>...
 *
 * Runs Run Mgmt > Re-Parse Outputs inside the running session: the current
 * parsers read the outputs stored with each finished calculation. The request
 * goes to the Organizer over the session's broker; the answer comes when the
 * last job store has sent its 'reparse' notice.
 *
 * Prints "name: done", "name: failed: reason" or "name: refused: reason" per
 * calculation (a running job, GROMACS and NWChemMD are refused as in the
 * menu); --json prints one array of {"name","status","reason"}. Exit 0: all
 * done; 1: some were not; 2: no running ECCE session; 4: no answer in time
 * (it may have been carried out); 5: the Organizer ended before answering;
 * 64: usage.
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
#include "util/SessionLease.H"

using std::string;
using std::vector;

namespace {

const char *const kNoAnswer =
    "no answer from the Organizer; it may have been carried out — check before repeating";
const char *const kOrganizerEnded =
    "the Organizer ended before answering; the request may have been carried out — check before repeating";
const int kAckSeconds = 8;          // an Organizer answers at once
const int kParseSeconds = 30 * 60;  // a big output takes minutes to parse

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

string jsonString(const string& s)
{
  string o = "\"";
  for (unsigned char c : s) {
    if (c == '"' || c == '\\') { o += '\\'; o += (char)c; }
    else if (c < 0x20) { char b[8]; snprintf(b, sizeof b, "\\u%04x", c); o += b; }
    else o += (char)c;
  }
  return o + "\"";
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

  bool json = false;
  vector<string> args;
  for (int i = 1; i < argc; i++) {
    string a = argv[i];
    if (a == "--json") json = true;
    else args.push_back(a);
  }
  if (args.empty()) {
    std::cerr << "usage: ecce-reparse [--json] <calculation path or URL>..."
              << std::endl;
    return 64;
  }

  const string sessKey = sessionKey();
  if (sessKey.empty()) return notRunning();

  bool allOk = true;
  vector<string> urls, objs;
  auto report = [&](const string& name, const string& status, const string& why) {
    if (json)
      objs.push_back("{\"name\":" + jsonString(name) + ",\"status\":" +
                     jsonString(status) + ",\"reason\":" + jsonString(why) + "}");
    else
      std::cout << name << ": " << status << (why.empty() ? "" : ": " + why)
                << std::endl;
    if (status != "done") allOk = false;
  };
  auto finish = [&](int rc) {
    if (json) {
      std::cout << "[";
      for (size_t i = 0; i < objs.size(); i++) std::cout << (i ? "," : "") << objs[i];
      std::cout << "]" << std::endl;
    }
    return rc;
  };
  for (size_t i = 0; i < args.size(); i++) {
    string url, why;
    if (toUrl(args[i], url, why)) urls.push_back(url);
    else report(args[i], "refused", why);
  }
  if (urls.empty()) return finish(1);

  // Needed for a session on a TCP broker (central server), where the
  // account's data server login is the broker login.
  MqttLink::setCredentialProvider(AuthCache::brokerCredential);

  JMSSubscriber sub("EcceReparse");
  if (!sub.messagingEnabled() || !sub.subscribe("ecce_reparse_reply", replyCB) ||
      !sub.startSubscriber() || sub.getSocketID() < 0)
    return notRunning();

  using clock = std::chrono::steady_clock;
  auto end = clock::now() + std::chrono::seconds(kAckSeconds);
  while (!MqttLink::instance().connected() && clock::now() < end) std::this_thread::sleep_for(std::chrono::milliseconds(50));
  if (!MqttLink::instance().connected()) return notRunning();

  gReplyId = std::to_string(getpid()) + "-" + std::to_string(time(0));
  // Armed before the request: the pid that will answer it (#816).
  SessionLease::ProgramWatch organizer(sessKey, "organizer");
  organizer.arm();
  JMSPublisher pub("EcceReparse");
  JMSMessage *msg = pub.newMessage();
  msg->addProperty("reply_id", gReplyId);
  msg->addIntProperty("n", (int)urls.size());
  for (size_t i = 0; i < urls.size(); i++)
    msg->addProperty("url" + std::to_string(i), urls[i]);
  bool sent = pub.publish("ecce_reparse_request", *msg);
  delete msg;
  if (!sent) return notRunning();

  end = clock::now() + std::chrono::seconds(kAckSeconds);
  bool organizerEnded = false;
  bool timedOut = false, batchLimitSet = false;
  while (!gDone) {
    if (clock::now() >= end) { timedOut = true; break; }
    sub.processMessage();
    if (gDone) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (gAck && !gDone && organizer.gone()) { organizerEnded = true; break; }
    if (gAck && !gDone && !batchLimitSet) {
      batchLimitSet = true;   // once: resetting every pass would never time out
      end = clock::now() + std::chrono::seconds(kParseSeconds);
    }
  }
  if (organizerEnded) {
    std::cerr << "ecce-reparse: " << kOrganizerEnded << std::endl;
    return 5;
  }
  if (timedOut) {
    if (!gAck) return notRunning();
    for (size_t i = 0; i < urls.size(); i++) report(urls[i], "no answer", kNoAnswer);
    return finish(4);  // no answer: may have been carried out
  }

  for (size_t i = 0; i < gLines.size(); i++) {
    size_t a = gLines[i].find('\t');
    size_t b = a == string::npos ? a : gLines[i].find('\t', a + 1);
    if (b == string::npos) { report(gLines[i], "failed", ""); continue; }
    report(gLines[i].substr(0, a), gLines[i].substr(a + 1, b - a - 1),
           gLines[i].substr(b + 1));
  }
  return finish(allOk ? 0 : 1);
}
