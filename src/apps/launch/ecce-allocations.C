/*
 * ecce-allocations [--refresh] [--timeout SECONDS] [machine]  (#1319)
 *
 * What Tools > Allocations shows, for a script. The request goes to the
 * running Organizer, which holds the ssh logins: it runs the code the
 * window's Refresh button runs (AllocationQuery), so this tool never
 * queries a cluster itself. Without --refresh the Organizer reports what it
 * knows now and opens no login. With --refresh it starts a check that may log
 * in, and this tool asks again once a second until no account is still
 * being checked or --timeout seconds (default 30) have passed; accounts
 * not answered by then are printed as "still checking".
 *
 * One line per machine and account, tab separated:
 *   machine  account  usage line or state  checked YYYY-MM-DD HH:MM (N min ago)
 * Exit 0: every line has a current value; 1: some has none (not checked,
 * still checking, not recognised, old) or nothing matched; 2: no running ECCE
 * session; 4: the Organizer stopped answering; 5: it ended; 64: usage.
 */
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <ctime>
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
#include "util/MqttLink.H"
#include "util/SessionLease.H"

using std::string;
using std::vector;

namespace {

const int kAckSeconds = 8;          // an Organizer answers at once
const int kDefaultTimeout = 30;

string gReplyId;
bool gDone = false;
vector<string> gLines;

void replyCB(JMSMessage& msg)
{
  if (msg.getProperty("reply_id") != gReplyId) return;
  int n = msg.getIntProperty("n");
  gLines.clear();
  for (int i = 0; i < n; i++) gLines.push_back(msg.getProperty("line" + std::to_string(i)));
  gDone = true;
}

int notRunning()
{
  std::cerr << "ECCE is not running: start ECCE first" << std::endl;
  return 2;
}

// As ecce-annotate: the id in the environment, else the newest session.
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

vector<string> split(const string& s)
{
  vector<string> f;
  size_t a = 0;
  for (;;) {
    size_t b = s.find('\t', a);
    f.push_back(s.substr(a, b == string::npos ? b : b - a));
    if (b == string::npos) return f;
    a = b + 1;
  }
}

string checkedText(long when)
{
  time_t t = (time_t)when;
  struct tm tmv;
  localtime_r(&t, &tmv);
  char buf[32];
  strftime(buf, sizeof buf, "%Y-%m-%d %H:%M", &tmv);
  long age = (long)(time(0) - t);
  string ago = age < 90 ? "just now"
             : age < 5400 ? std::to_string((age + 30) / 60) + " min ago"
             : age < 129600 ? std::to_string((age + 1800) / 3600) + " h ago"
                            : std::to_string((age + 43200) / 86400) + " days ago";
  return string("checked ") + buf + " (" + ago + ")";
}

// Fields from the Organizer: machine account state host used raw when old why,
// state being value, checking or none. Returns true for a current value.
bool printLine(const string& line)
{
  vector<string> f = split(line);
  if (f.size() < 9) { std::cout << line << std::endl; return false; }
  const string &state = f[2], &host = f[3], &used = f[4], &raw = f[5], &why = f[8];
  const long when = strtol(f[6].c_str(), 0, 10);
  const bool old = f[7] == "1";
  const string known = !raw.empty() ? raw : used;
  string text, checked;
  bool current = false;
  if (when > 0) checked = checkedText(when);
  if (state == "checking") {
    text = when > 0 ? known + " (cached; still checking)" : "still checking";
  } else if (when > 0 && !old) {
    if (used.empty()) text = "not recognised" + (raw.empty() ? string() : ": " + raw);
    else { text = known; current = true; }
  } else {
    const string reason = why.empty() ? "not logged in to " + host : why;
    text = when > 0 ? known + " (old; " + reason + ")" : "not checked: " + reason;
  }
  std::cout << f[0] << '\t' << f[1] << '\t' << text << '\t' << checked << std::endl;
  return current;
}

}  // namespace

int main(int argc, char **argv)
{
  Ecce::commandEnvironment();

  bool refresh = false;
  int timeout = kDefaultTimeout;
  string machine;
  for (int i = 1; i < argc; i++) {
    string a = argv[i];
    if (a == "--refresh") refresh = true;
    else if (a == "--timeout" && i + 1 < argc) timeout = atoi(argv[++i]);
    else if (a == "--help" || a == "-h" || (!a.empty() && a[0] == '-') || !machine.empty() ||
             a.empty()) {
      std::ostream& o = (a == "--help" || a == "-h") ? std::cout : std::cerr;
      o << "usage: ecce-allocations [--refresh] [--timeout SECONDS] [machine]\n"
           "  Prints the Tools > Allocations table, one line per machine and account:\n"
           "  machine, account, the usage line, when it was checked.\n"
           "  Without --refresh: what is known now (answers are kept 15 minutes);\n"
           "  no login is opened.\n"
           "  --refresh: log in where needed and ask the clusters again. Waits up to\n"
           "  --timeout seconds (default " << kDefaultTimeout << ") and prints what has\n"
           "  arrived; accounts not yet answered are printed as still checking.\n"
           "  machine: only that machine's accounts.\n"
           "  Exit 0: all current; 1: some without a current value; 2: ECCE not running."
        << std::endl;
      return (a == "--help" || a == "-h") ? 0 : 64;
    }
    else machine = a;
  }
  if (timeout < 1) timeout = 1;

  const string sessKey = sessionKey();
  if (sessKey.empty()) return notRunning();

  MqttLink::setCredentialProvider(AuthCache::brokerCredential);

  JMSSubscriber sub("EcceAllocations");
  if (!sub.messagingEnabled() || !sub.subscribe("ecce_allocations_reply", replyCB) ||
      !sub.startSubscriber())
    return notRunning();

  using clock = std::chrono::steady_clock;
  auto end = clock::now() + std::chrono::seconds(kAckSeconds);
  while (!MqttLink::instance().connected() && clock::now() < end)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  if (!MqttLink::instance().connected()) return notRunning();

  SessionLease::ProgramWatch organizer(sessKey, "organizer");
  organizer.arm();
  JMSPublisher pub("EcceAllocations");

  // One request and its answer. 0: answered; 2: never answered; 4: stopped
  // answering; 5: the Organizer ended.
  const string base = std::to_string(getpid()) + "-" + std::to_string(time(0));
  int round = 0;
  auto ask = [&](bool force, bool first) {
    gReplyId = base + "-" + std::to_string(round++);
    gDone = false;
    JMSMessage *msg = pub.newMessage();
    msg->addProperty("reply_id", gReplyId);
    msg->addProperty("machine", machine);
    msg->addProperty("force", force ? "yes" : "no");
    bool sent = pub.publish("ecce_allocations_request", *msg);
    delete msg;
    if (!sent) return first ? 2 : 4;
    auto until = clock::now() + std::chrono::seconds(kAckSeconds);
    while (!gDone) {
      if (clock::now() >= until) return first ? 2 : 4;
      sub.processMessage();
      if (gDone) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      if (organizer.gone()) return 5;
    }
    return 0;
  };

  auto stillChecking = [&]() {
    for (size_t i = 0; i < gLines.size(); i++) {
      vector<string> f = split(gLines[i]);
      if (f.size() > 2 && f[2] == "checking") return true;
    }
    return false;
  };

  int rc = ask(refresh, true);
  if (rc == 2) return notRunning();
  if (rc == 5) {
    std::cerr << "ecce-allocations: the Organizer ended before answering" << std::endl;
    return 5;
  }
  if (rc == 0 && refresh) {
    auto limit = clock::now() + std::chrono::seconds(timeout);
    while (stillChecking() && clock::now() < limit) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      vector<string> keep = gLines;
      rc = ask(false, false);
      if (rc != 0) { gLines = keep; break; }
    }
  }

  if (gLines.empty()) {
    std::cout << (machine.empty() ? string("no machine with an allocation command and an account was found")
                                  : "no allocation account found for " + machine)
              << std::endl;
    return rc == 0 ? 1 : rc;
  }
  bool allCurrent = true;
  for (size_t i = 0; i < gLines.size(); i++)
    if (!printLine(gLines[i])) allCurrent = false;
  if (rc == 5) {
    std::cerr << "ecce-allocations: the Organizer ended before answering" << std::endl;
    return 5;
  }
  if (rc != 0) {
    std::cerr << "ecce-allocations: the Organizer stopped answering" << std::endl;
    return rc;
  }
  return allCurrent ? 0 : 1;
}
