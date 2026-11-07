/*
 * ecce-workflow make <calculation> [name]   (#807)
 * ecce-workflow move <calculation> <workflow>
 * ecce-workflow create <workflow-or-new-name> <calculation>...
 *               [--link A:B[=role]]... [--auto-links]   (#874)
 *
 * Make Workflow... and Move into Workflow > of the Organizer: the calculation
 * is moved into a new (or an existing) workflow of its folder as a step;
 * create does the same for several calculations of one folder, with the
 * links given (A and B are calculation names) or proposed. The Organizer
 * does the work, so ECCE must be running. Paths are inside the local data
 * folder, or URLs.
 *
 * Prints "ok: <workflow URL>" or "error: reason". Exit 0: done; 1: refused
 * or no answer; 2: ECCE is not running; 64: usage.
 */
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <climits>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "tdat/AuthCache.H"
#include "util/Ecce.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/JMSSubscriber.H"
#include "util/LocalData.H"
#include "util/MqttLink.H"

using std::string;

namespace {

const int kAckSeconds = 8;
const int kAnswerSeconds = 60;

string gReplyId;
bool gDone = false;
string gAnswer;

void replyCB(JMSMessage& msg)
{
  if (msg.getProperty("reply_id") != gReplyId) return;
  // A busy Organizer acknowledges at once and answers later.
  if (msg.getProperty("kind") == "ack") return;
  gAnswer = msg.getProperty("answer");
  gDone = true;
}

int notRunning()
{
  std::cerr << "ECCE is not running: start ECCE first" << std::endl;
  return 2;
}

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

bool toUrl(const string& arg, string& url, string& why)
{
  if (arg.find("://") != string::npos) { url = arg; return true; }
  string root = LocalData::dir();
  if (root.empty()) {
    why = "this session uses a data server: give the URL";
    return false;
  }
  char buf[PATH_MAX], rbuf[PATH_MAX];
  if (!realpath(arg.c_str(), buf)) { why = "no such folder: " + arg; return false; }
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
  if (!getenv("ECCE_REALUSERHOME")) {
    const char *h = getenv("HOME");
    if (h) setenv("ECCE_REALUSERHOME", h, 0);
  }
  const string verb = argc > 1 ? argv[1] : "";
  const bool make = verb == "make", create = verb == "create";
  std::vector<string> calcArgs, links;
  bool autoLinks = false, badArg = false;
  for (int i = 3; create && i < argc; i++) {
    const string a = argv[i];
    if (a == "--auto-links") autoLinks = true;
    else if (a == "--link" && i + 1 < argc) links.push_back(argv[++i]);
    else if (a.compare(0, 7, "--link=") == 0) links.push_back(a.substr(7));
    else if (a.compare(0, 2, "--") == 0) badArg = true;
    else calcArgs.push_back(a);
  }
  if (!((make && (argc == 3 || argc == 4)) || (verb == "move" && argc == 4) ||
        (create && !badArg && !calcArgs.empty()))) {
    std::cerr << "usage: ecce-workflow make <calculation> [name]\n"
                 "       ecce-workflow move <calculation> <workflow>\n"
                 "       ecce-workflow create <workflow-or-new-name> "
                 "<calculation>... [--link A:B[=role]]... [--auto-links]"
              << std::endl;
    return 64;
  }
  if (sessionKey().empty()) return notRunning();

  string calc, second, why;
  std::vector<string> calcUrls;
  if (create) {
    // An existing workflow is a path or URL; anything else names a new one.
    const string target = argv[2];
    struct stat st;
    if (target.find("://") != string::npos || stat(target.c_str(), &st) == 0) {
      if (!toUrl(target, second, why)) {
        std::cout << "error: " << why << std::endl;
        return 1;
      }
    } else if (target.find('/') != string::npos) {
      std::cout << "error: no such workflow: " << target << std::endl;
      return 1;
    } else {
      second = target;
    }
    for (const string& c : calcArgs) {
      string url;
      if (!toUrl(c, url, why)) {
        std::cout << "error: " << why << std::endl;
        return 1;
      }
      calcUrls.push_back(url);
    }
    // Links name calculations; a path given for one is reduced to its name.
    for (string& l : links) {
      const size_t colon = l.find(':'), eq = l.find('=', colon);
      if (colon == string::npos) {
        std::cout << "error: link " << l << ": give A:B" << std::endl;
        return 1;
      }
      auto base = [](string s) {
        while (s.size() > 1 && s.back() == '/') s.pop_back();
        const size_t slash = s.rfind('/');
        return slash == string::npos ? s : s.substr(slash + 1);
      };
      const string a = base(l.substr(0, colon));
      const string b = base(l.substr(colon + 1, eq == string::npos
                                                    ? string::npos : eq - colon - 1));
      l = a + ":" + b + (eq == string::npos ? "" : l.substr(eq));
    }
  } else if (!toUrl(argv[2], calc, why) ||
             (!make && !toUrl(argv[3], second, why))) {
    std::cout << "error: " << why << std::endl;
    return 1;
  }
  if (make && argc == 4) second = argv[3];

  MqttLink::setCredentialProvider(AuthCache::brokerCredential);
  JMSSubscriber sub("EcceWorkflow");
  if (!sub.messagingEnabled() || !sub.subscribe("ecce_workflow_reply", replyCB) ||
      !sub.startSubscriber() || sub.getSocketID() < 0)
    return notRunning();

  using clock = std::chrono::steady_clock;
  auto end = clock::now() + std::chrono::seconds(kAckSeconds);
  while (!MqttLink::instance().connected() && clock::now() < end)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  if (!MqttLink::instance().connected()) return notRunning();

  gReplyId = std::to_string(getpid()) + "-" + std::to_string(time(0));
  JMSPublisher pub("EcceWorkflow");
  JMSMessage *msg = pub.newMessage();
  msg->addProperty("reply_id", gReplyId);
  msg->addProperty("verb", verb);
  msg->addProperty("calc", calc);
  msg->addProperty("second", second);
  for (size_t i = 0; i < calcUrls.size(); i++)
    msg->addProperty("calc" + std::to_string(i), calcUrls[i]);
  for (size_t i = 0; i < links.size(); i++)
    msg->addProperty("link" + std::to_string(i), links[i]);
  if (autoLinks) msg->addProperty("autolinks", "1");
  bool sent = pub.publish("ecce_workflow_request", *msg);
  delete msg;
  if (!sent) return notRunning();

  end = clock::now() + std::chrono::seconds(kAnswerSeconds);
  while (!gDone && clock::now() < end) {
    sub.processMessage();
    if (!gDone) std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  if (!gDone) {
    std::cerr << "ecce-workflow: no answer from ECCE; see the Organizer"
              << std::endl;
    return 1;
  }
  std::cout << gAnswer << std::endl;
  return gAnswer.compare(0, 2, "ok") == 0 ? 0 : 1;
}
