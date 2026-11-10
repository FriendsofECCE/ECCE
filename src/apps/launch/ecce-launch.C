/*
 * ecce-launch [on=MACHINE] [profile=NAME] [queue=Q] [wall=TIME] <calculation path or URL>...  (#687)
 *
 * Launches Ready calculations with their stored settings inside the running
 * session: the request goes to the Organizer over the session's broker and
 * the Organizer's Launch Selected does the work, so its tree, the shared ssh
 * connection and the job monitors are the ones the user sees.
 *
 * Prints "name: job id N" or "name: not launched: reason" per calculation,
 * with "name: note: ..." before it for a setting that was filled in or
 * dropped (no stored queue, a machine that passes no time limit).
 * Every calculation given gets one line, also when ECCE gives no answer.
 * --move on=M: a Submitted calculation whose Slurm job is still pending is
 * cancelled and submitted on M (#830); lines read "not moved: ..." if refused.
 * Exit 4: no answer in time (it may have been carried out); 5: the Organizer ended before answering. Exit 0: all launched; 1: some were not; 2: no running
 * ECCE session; 3: the Organizer is busy with a Launch Selected batch;
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
const int kBatchSeconds = 10 * 60;  // a batch submits one by one

string gReplyId;
bool gAck = false, gDone = false, gBusy = false, gQueued = false;
vector<string> gLines;

void replyCB(JMSMessage& msg)
{
  if (msg.getProperty("reply_id") != gReplyId) return;
  string kind = msg.getProperty("kind");
  if (kind == "ack") { gAck = true; if (msg.getProperty("queued") == "1") gQueued = true; return; }
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

// The name the Organizer's lines use for a calculation: its folder name.
string leafOf(const string& url)
{
  string u = url;
  while (u.size() > 1 && u.back() == '/') u.pop_back();
  size_t slash = u.rfind('/');
  return slash == string::npos ? u : u.substr(slash + 1);
}

// The request has gone out, so a busy Organizer may still submit these later
// (it defers broker messages during a launch): never say "not launched".
void unanswered(const vector<string>& given, const vector<string>& urls,
                const string& why)
{
  for (size_t i = 0; i < urls.size(); i++)
    std::cout << given[i] << ": submitted? " << why
              << ": check the Organizer and the queue (job name submit__"
              << leafOf(urls[i]) << ") before launching it again" << std::endl;
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

  string on, profile, queue, wall;
  bool move = false;
  vector<string> args;
  for (int i = 1; i < argc; i++) {
    string a = argv[i];
    if (a.compare(0, 3, "on=") == 0) on = a.substr(3);
    else if (a.compare(0, 8, "profile=") == 0) profile = a.substr(8);
    else if (a.compare(0, 6, "queue=") == 0) queue = a.substr(6);
    else if (a.compare(0, 5, "wall=") == 0) wall = a.substr(5);
    else if (a == "--move") move = true;
    else args.push_back(a);
  }
  if (args.empty() || (move && on.empty())) {
    std::cerr << "usage: ecce-launch [--move] [on=MACHINE] [profile=NAME] [queue=QUEUE] "
                 "[wall=H:MM] <calculation path or URL>...\n"
                 "       --move needs on=MACHINE" << std::endl;
    return 64;
  }

  const string sessKey = sessionKey();
  if (sessKey.empty()) return notRunning();

  bool allOk = true;
  vector<string> urls, given;
  for (size_t i = 0; i < args.size(); i++) {
    string url, why;
    if (toUrl(args[i], url, why)) { urls.push_back(url); given.push_back(args[i]); }
    else {
      std::cout << args[i] << ": not launched: " << why << std::endl;
      allOk = false;
    }
  }
  if (urls.empty()) return 1;

  // Needed for a session on a TCP broker (central server), where the
  // account's data server login is the broker login.
  MqttLink::setCredentialProvider(AuthCache::brokerCredential);

  JMSSubscriber sub("EcceLaunch");
  if (!sub.messagingEnabled() || !sub.subscribe("ecce_launch_reply", replyCB) ||
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
  JMSPublisher pub("EcceLaunch");
  JMSMessage *msg = pub.newMessage();
  msg->addProperty("reply_id", gReplyId);
  msg->addProperty("on", on);
  msg->addProperty("profile", profile);
  msg->addProperty("queue", queue);
  msg->addProperty("wall", wall);
  msg->addProperty("move", move ? "yes" : "no");
  msg->addIntProperty("n", (int)urls.size());
  for (size_t i = 0; i < urls.size(); i++)
    msg->addProperty("url" + std::to_string(i), urls[i]);
  bool sent = pub.publish("ecce_launch_request", *msg);
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
      // set once: resetting it every pass would make the limit never fire
      end = clock::now() + std::chrono::seconds(kBatchSeconds);
      batchLimitSet = true;
      if (gQueued)
        std::cerr << "ecce-launch: queued behind a launch that is running; waiting"
                  << std::endl;
    }
  }
  if (organizerEnded) {
    std::cerr << "ecce-launch: " << kOrganizerEnded << std::endl;
    return 5;
  }
  if (timedOut) {
    if (!gAck) return notRunning();
    for (size_t i = 0; i < urls.size(); i++)
      std::cout << given[i] << ": " << kNoAnswer << " (job name submit__"
                << leafOf(urls[i]) << ")" << std::endl;
    return 4;  // no answer: may have been carried out
  }
  if (gBusy) {
    std::cerr << "ecce-launch: ECCE is submitting a Launch Selected batch; "
                 "try again when it has finished" << std::endl;
    return 3;
  }

  vector<string> answered;
  for (size_t i = 0; i < gLines.size(); i++) {
    size_t a = gLines[i].find('\t');
    size_t b = a == string::npos ? a : gLines[i].find('\t', a + 1);
    if (b == string::npos) { std::cout << gLines[i] << std::endl; allOk = false; continue; }
    string name = gLines[i].substr(0, a), ok = gLines[i].substr(a + 1, b - a - 1),
           text = gLines[i].substr(b + 1);
    if (ok != "warn") answered.push_back(name);
    if (ok == "warn") std::cout << name << ": note: " << text << std::endl;
    else if (ok == "ok") std::cout << name << ": job id " << text << std::endl;
    else { std::cout << name << (move ? ": not moved: " : ": not launched: ") << text << std::endl; allOk = false; }
  }
  //  The Organizer answers one line per calculation; match names only when
  //  some are missing, as a stored name need not equal its folder name.
  vector<string> missingGiven, missingUrls;
  for (size_t i = 0; answered.size() < urls.size() && i < urls.size(); i++) {
    bool found = false;
    for (const string& n : answered)
      if (n == leafOf(urls[i]) || n == urls[i]) { found = true; break; }
    if (!found) { missingGiven.push_back(given[i]); missingUrls.push_back(urls[i]); }
  }
  if (!missingUrls.empty()) {
    unanswered(missingGiven, missingUrls, "ECCE's answer has no line for it");
    allOk = false;
  }
  return allOk ? 0 : 1;
}
