// Ask the running Organizer to copy a calculation through New from This >
// Rerun, keeping its setup and optionally submitting the new calculation.
#include "util/Filesystem.H"

#include <chrono>
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

const char *const kNoAnswer =
    "no answer from the Organizer; it may have been carried out — check before repeating";
const int kAckSeconds = 8;          // an Organizer answers at once
const int kBatchSeconds = 10 * 60;  // a batch submits one by one

string gReplyId;
bool gAck = false, gDone = false;
vector<string> gLines;

void replyCB(JMSMessage& msg)
{
  if (msg.getProperty("reply_id") != gReplyId) return;
  string kind = msg.getProperty("kind");
  if (kind == "ack") { gAck = true; return; }
  int n = msg.getIntProperty("n");
  for (int i = 0; i < n; i++) gLines.push_back(msg.getProperty("line" + std::to_string(i)));
  gAck = true;
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
#ifdef _WIN32
      _putenv_s("ECCE_SESSION_ID", id.c_str());
#else
      setenv("ECCE_SESSION_ID", id.c_str(), 1);
#endif
    }
  }
  std::error_code ec;
  if (key.empty() ||
      !ecce::fs::exists(string(Ecce::realUserPrefPath()) + "broker_" + key, ec))
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
  std::error_code ec;
  const auto path = ecce::fs::canonical(arg, ec);
  if (ec) { why = "no such folder"; return false; }
  const auto base = ecce::fs::canonical(root, ec);
  if (ec) { why = "cannot read the data folder"; return false; }
  const auto relative = path.lexically_relative(base);
  if (relative.empty() || relative == "." || *relative.begin() == "..") {
    why = "not inside the data folder " + base.generic_string();
    return false;
  }
  const string full = path.generic_string();
  url = "file://" + (full[0] == '/' ? string() : string("/")) + full;
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

  string on, wall;
  unsigned long cores = 0;
  bool launch = false, lastGeom = false, badUsage = false;
  vector<string> args;
  for (int i = 1; i < argc; i++) {
    string a = argv[i];
    if (a.compare(0, 3, "on=") == 0) {
      on = a.substr(3);
      if (on.empty()) badUsage = true;
    } else if (a.compare(0, 6, "cores=") == 0) {
      char *e = 0;
      cores = strtoul(a.c_str() + 6, &e, 10);
      if (a.size() == 6 || *e || cores == 0 || cores > 1000000) badUsage = true;
    } else if (a.compare(0, 5, "wall=") == 0) {
      wall = a.substr(5);
      if (wall.empty()) badUsage = true;
    } else if (a == "--last-geometry") lastGeom = true;
    else if (a == "launch") launch = true;
    else if (a.compare(0, 1, "-") == 0) badUsage = true;
    else args.push_back(a);
  }
  if (args.empty() || badUsage) {
    std::cerr << "usage: ecce-rerun [--last-geometry] [cores=N] [wall=\"D H:M\"] "
                 "[on=MACHINE] [launch] <calculation path or URL>..."
              << std::endl;
    return 64;
  }

  if (sessionKey().empty()) return notRunning();

  bool allOk = true;
  vector<string> urls;
  for (size_t i = 0; i < args.size(); i++) {
    string url, why;
    if (toUrl(args[i], url, why)) urls.push_back(url);
    else {
      std::cout << args[i] << ": not rerun: " << why << std::endl;
      allOk = false;
    }
  }
  if (urls.empty()) return 1;

  // Needed for a session on a TCP broker (central server), where the
  // account's data server login is the broker login.
  MqttLink::setCredentialProvider(AuthCache::brokerCredential);

  JMSSubscriber sub("EcceRerun");
  if (!sub.messagingEnabled() || !sub.subscribe("ecce_rerun_reply", replyCB) ||
      !sub.startSubscriber() || sub.getSocketID() < 0)
    return notRunning();

  using clock = std::chrono::steady_clock;
  auto end = clock::now() + std::chrono::seconds(kAckSeconds);
  while (!MqttLink::instance().connected() && clock::now() < end) std::this_thread::sleep_for(std::chrono::milliseconds(50));
  if (!MqttLink::instance().connected()) return notRunning();

  gReplyId = std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count());
  JMSPublisher pub("EcceRerun");
  JMSMessage *msg = pub.newMessage();
  msg->addProperty("reply_id", gReplyId);
  msg->addProperty("on", on);
  msg->addProperty("launch", launch ? "yes" : "no");
  msg->addProperty("last_geometry", lastGeom ? "yes" : "no");
  msg->addProperty("cores", std::to_string(cores));
  msg->addProperty("wall", wall);
  msg->addIntProperty("n", (int)urls.size());
  for (size_t i = 0; i < urls.size(); i++)
    msg->addProperty("url" + std::to_string(i), urls[i]);
  bool sent = pub.publish("ecce_rerun_request", *msg);
  delete msg;
  if (!sent) return notRunning();

  end = clock::now() + std::chrono::seconds(kAckSeconds);
  bool timedOut = false, acknowledged = false;
  while (!gDone) {
    if (clock::now() >= end) { timedOut = true; break; }
    sub.processMessage();
    if (gDone) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (gAck && !acknowledged) {
      acknowledged = true;
      end = clock::now() + std::chrono::seconds(kBatchSeconds);
    }
  }
  if (timedOut) {
    if (!gAck) return notRunning();
    for (size_t i = 0; i < args.size(); i++)
      std::cout << args[i] << ": " << kNoAnswer << std::endl;
    return 4;  // no answer: may have been carried out
  }
  for (size_t i = 0; i < gLines.size(); i++) {
    size_t a = gLines[i].find('\t');
    size_t b = a == string::npos ? a : gLines[i].find('\t', a + 1);
    if (b == string::npos) { std::cout << gLines[i] << std::endl; allOk = false; continue; }
    string name = gLines[i].substr(0, a), ok = gLines[i].substr(a + 1, b - a - 1),
           text = gLines[i].substr(b + 1);
    for (char& c : text)
      if (c == '\n' || c == '\r') c = ' ';
    if (ok == "ok" || ok == "launch_failed") {
      std::cout << name << ": rerun as " << text << std::endl;
      if (ok == "launch_failed") allOk = false;
    }
    else { std::cout << name << ": not rerun: " << text << std::endl; allOk = false; }
  }
  return allOk ? 0 : 1;
}
