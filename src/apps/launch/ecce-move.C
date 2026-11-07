/*
 * ecce-move <calculation or folder>... <folder>   (#824)
 * ecce-rename <calculation or folder> <new name>
 * ecce-copy <calculation or folder>... <folder>   (#890)
 *
 * The Organizer's own move and Edit > Rename, asked of the running Organizer
 * so that its tree is updated at once. The refusals are the Organizer's: a
 * calculation whose job is Submitted, Running or Waiting for Login, or a
 * folder holding one (#700), a name clash, a folder in another data store.
 * The folder may be in another tree (Local data, an SSH folder, an ECCE
 * server), given by its URL; a move there is a copy, a check of file counts
 * and sizes, and only then the removal of the source. ecce-copy is the same
 * without the removal and without the active-job refusal. Without a running
 * session nothing is done: a move behind the Organizer's back leaves stale tree nodes.
 *
 * Prints "name: moved to folder" / "name: renamed to new name" or
 * "name: not moved: reason" / "name: not renamed: reason". Exit 3 also means no answer in time (it may have been carried out). Exit 0: all done;
 * 1: some were not; 2: no running ECCE session; 64: usage.
 * One source, built twice: ECCE_RENAME_COMMAND selects ecce-rename.
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

#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"
#include "dsm/VDoc.H"
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
const int kBatchSeconds = 3600;     // a transfer between trees can be large

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

#ifdef ECCE_RENAME_COMMAND
  const bool rename = true;
  const char *me = "ecce-rename";
  const char *verb = "rename";
#elif defined(ECCE_COPY_COMMAND)
  const bool rename = false;
  const char *me = "ecce-copy";
  const char *verb = "copy";
#else
  const bool rename = false;
  const char *me = "ecce-move";
  const char *verb = "move";
#endif
  const bool copying = string(verb) == "copy";
  const string done = rename ? "renamed to " : copying ? "copied to " : "moved to ";
  const string undone = rename ? "not renamed" : copying ? "not copied" : "not moved";
  vector<string> args(argv + 1, argv + argc);
  if (args.size() < 2 || (rename && args.size() != 2)) {
    std::cerr << (rename ? "usage: ecce-rename <calculation or folder> <new name>"
                         : copying ? "usage: ecce-copy <calculation or folder>... <folder>"
                                   : "usage: ecce-move <calculation or folder>... <folder>")
              << std::endl;
    return 64;
  }
  const string last = args.back();   // the folder, or the new name
  args.pop_back();

  if (sessionKey().empty()) return notRunning();

  bool allOk = true;
  vector<string> urls;
  for (size_t i = 0; i < args.size(); i++) {
    string url, why;
    if (toUrl(args[i], url, why)) urls.push_back(url);
    else {
      std::cout << args[i] << ": " << undone << ": " << why << std::endl;
      allOk = false;
    }
  }
  if (urls.empty()) return 1;
  string secondUrl = last;
  if (!rename) {
    string why;
    if (!toUrl(last, secondUrl, why)) {
      for (size_t i = 0; i < args.size(); i++)
        std::cout << args[i] << ": " << undone << ": the folder " << last << ": " << why << std::endl;
      return 1;
    }
  }

  // Needed for a session on a TCP broker (central server), where the
  // account's data server login is the broker login.
  MqttLink::setCredentialProvider(AuthCache::brokerCredential);

  JMSSubscriber sub("EcceMove");
  if (!sub.messagingEnabled() || !sub.subscribe("ecce_move_reply", replyCB) ||
      !sub.startSubscriber() || sub.getSocketID() < 0)
    return notRunning();

  using clock = std::chrono::steady_clock;
  auto end = clock::now() + std::chrono::seconds(kAckSeconds);
  while (!MqttLink::instance().connected() && clock::now() < end) std::this_thread::sleep_for(std::chrono::milliseconds(50));
  if (!MqttLink::instance().connected()) return notRunning();

  gReplyId = std::to_string(getpid()) + "-" + std::to_string(time(0));
  JMSPublisher pub("EcceMove");
  JMSMessage *msg = pub.newMessage();
  msg->addProperty("reply_id", gReplyId);
  msg->addProperty("verb", verb);
  msg->addProperty("second", secondUrl);
  msg->addIntProperty("n", (int)urls.size());
  for (size_t i = 0; i < urls.size(); i++)
    msg->addProperty("url" + std::to_string(i), urls[i]);
  bool sent = pub.publish("ecce_move_request", *msg);
  delete msg;
  if (!sent) return notRunning();

  end = clock::now() + std::chrono::seconds(kAckSeconds);
  bool timedOut = false, batchLimitSet = false;
  while (!gDone) {
    if (clock::now() >= end) { timedOut = true; break; }
    sub.processMessage();
    if (gDone) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (gAck && !gDone && !batchLimitSet) {
      batchLimitSet = true;   // once: resetting every pass would never time out
      end = clock::now() + std::chrono::seconds(kBatchSeconds);
    }
  }
  if (timedOut) {
    if (!gAck) return notRunning();
    for (size_t i = 0; i < args.size(); i++)
      std::cout << args[i] << ": " << kNoAnswer << std::endl;
    return 3;
  }

  for (size_t i = 0; i < gLines.size(); i++) {
    size_t a = gLines[i].find('\t');
    size_t b = a == string::npos ? a : gLines[i].find('\t', a + 1);
    if (b == string::npos) { std::cout << gLines[i] << std::endl; allOk = false; continue; }
    string name = gLines[i].substr(0, a), ok = gLines[i].substr(a + 1, b - a - 1),
           reason = gLines[i].substr(b + 1);
    if (ok == "ok") std::cout << name << ": " << done << last << std::endl;
    else { std::cout << name << ": " << undone << ": " << reason << std::endl; allOk = false; }
  }
  return allOk ? 0 : 1;
}
