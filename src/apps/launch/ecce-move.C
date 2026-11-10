/*
 * ecce-move <calculation or folder>... <folder>   (#824)
 * ecce-rename <calculation or folder> <new name>
 * ecce-copy <calculation or folder>... <folder>   (#890)
 * ecce-mkdir [-p] [--json] <project or folder>...   (#870)
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
 * "name: not moved: reason" / "name: not renamed: reason". Exit 4: no answer in time (it may have been carried out); 5: the Organizer ended before answering. Exit 0: all done;
 * 1: some were not; 2: no running ECCE session; 64: usage.
 * ecce-mkdir makes a project or folder (-p: also the missing parents, and no
 * error if it exists), printing "path: created" / "path: exists" /
 * "path: not created: reason"; --json prints one object per path.
 * One source, built four times: ECCE_RENAME_COMMAND, ECCE_COPY_COMMAND and
 * ECCE_MKDIR_COMMAND select the other tools.
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
#include "util/SessionLease.H"

using std::string;
using std::vector;

namespace {

const char *const kNoAnswer =
    "no answer from the Organizer; it may have been carried out — check before repeating";
const char *const kOrganizerEnded =
    "the Organizer ended before answering; the request may have been carried out — check before repeating";
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

// ecce-mkdir: the URL of a folder that need not exist yet. The deepest
// existing ancestor must be inside the data folder; the rest is appended.
bool mkUrl(const string& arg, string& url, string& why)
{
  if (arg.find("://") != string::npos) { url = arg; return true; }
  string root = LocalData::dir();
  if (root.empty()) {
    why = "this session uses a data server: give the folder's URL";
    return false;
  }
  char cwd[PATH_MAX], buf[PATH_MAX], rbuf[PATH_MAX];
  string abs = arg;
  if (abs.empty() || abs[0] != '/') {
    if (!getcwd(cwd, sizeof cwd)) { why = "no current folder"; return false; }
    abs = string(cwd) + "/" + abs;
  }
  vector<string> parts;   // lexical normalisation: "." and ".." without the disk
  size_t i = 0;
  while (i <= abs.size()) {
    size_t j = abs.find('/', i);
    if (j == string::npos) j = abs.size();
    string seg = abs.substr(i, j - i);
    if (seg == "..") { if (!parts.empty()) parts.pop_back(); }
    else if (!seg.empty() && seg != ".") parts.push_back(seg);
    i = j + 1;
  }
  string existing, rest;
  size_t keep = parts.size();
  for (; keep > 0; keep--) {
    existing.clear();
    for (size_t k = 0; k < keep; k++) existing += "/" + parts[k];
    if (realpath(existing.c_str(), buf)) break;
  }
  if (keep == 0) { why = "not inside the data folder"; return false; }
  for (size_t k = keep; k < parts.size(); k++) rest += "/" + parts[k];
  string base = realpath(root.c_str(), rbuf) ? string(rbuf) : root;
  string path = string(buf);
  if (path != base && path.compare(0, base.size() + 1, base + "/") != 0) {
    why = "not inside the data folder " + base;
    return false;
  }
  if (path == base && rest.empty()) { why = "that is the data folder itself"; return false; }
  url = "file://" + path + rest;
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

#ifdef ECCE_RENAME_COMMAND
  const bool rename = true;
  const char *me = "ecce-rename";
  const char *verb = "rename";
#elif defined(ECCE_MKDIR_COMMAND)
  const bool rename = false;
  const char *me = "ecce-mkdir";
  const char *verb = "mkdir";
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
  const bool making = string(verb) == "mkdir";
  const string done = rename ? "renamed to " : copying ? "copied to " : "moved to ";
  const string undone = making ? "not created" : rename ? "not renamed"
                      : copying ? "not copied" : "not moved";
  vector<string> args;
  bool parents = false, json = false;
  for (int i = 1; i < argc; i++) {
    string a = argv[i];
    if (making && a == "-p") parents = true;
    else if (making && a == "--json") json = true;
    else args.push_back(a);
  }
  if (making) {
    if (args.empty()) {
      std::cerr << "usage: ecce-mkdir [-p] [--json] <project or folder>..." << std::endl;
      return 64;
    }
  } else if (args.size() < 2 || (rename && args.size() != 2)) {
    std::cerr << (rename ? "usage: ecce-rename <calculation or folder> <new name>"
                         : copying ? "usage: ecce-copy <calculation or folder>... <folder>"
                                   : "usage: ecce-move <calculation or folder>... <folder>")
              << std::endl;
    return 64;
  }
  const string last = making ? string() : args.back();   // the folder, or the new name
  if (!making) args.pop_back();

  const string sessKey = sessionKey();
  if (sessKey.empty()) return notRunning();

  bool allOk = true;
  vector<string> urls, names;   // names: what the user typed, for the output
  vector<string> refused;       // mkdir: paths refused before the request
  for (size_t i = 0; i < args.size(); i++) {
    string url, why;
    if (making ? mkUrl(args[i], url, why) : toUrl(args[i], url, why)) {
      urls.push_back(url);
      names.push_back(args[i]);
    } else {
      if (json) refused.push_back("{\"path\":" + jsonString(args[i]) +
                                  ",\"ok\":false,\"reason\":" + jsonString(why) + "}");
      else std::cout << args[i] << ": " << undone << ": " << why << std::endl;
      allOk = false;
    }
  }
  if (urls.empty()) {
    if (json) {
      std::cout << "[";
      for (size_t i = 0; i < refused.size(); i++) std::cout << (i ? "," : "") << refused[i];
      std::cout << "]" << std::endl;
    }
    return 1;
  }
  string secondUrl = last;
  if (making) secondUrl = parents ? "parents" : "";
  else if (!rename) {
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
  // Armed before the request: the pid that will answer it (#816).
  SessionLease::ProgramWatch organizer(sessKey, "organizer");
  organizer.arm();
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
      end = clock::now() + std::chrono::seconds(kBatchSeconds);
    }
  }
  if (organizerEnded) {
    std::cerr << "ecce-move: " << kOrganizerEnded << std::endl;
    return 5;
  }
  if (timedOut) {
    if (!gAck) return notRunning();
    for (size_t i = 0; i < names.size(); i++)
      std::cout << names[i] << ": " << kNoAnswer << std::endl;
    return 4;  // no answer: may have been carried out
  }

  if (making) {
    vector<string> objs = refused;
    for (size_t i = 0; i < gLines.size(); i++) {
      size_t a = gLines[i].find('\t');
      size_t b = a == string::npos ? a : gLines[i].find('\t', a + 1);
      string name = i < names.size() ? names[i] : gLines[i].substr(0, a);
      if (b == string::npos) { allOk = false; continue; }
      string ok = gLines[i].substr(a + 1, b - a - 1), rest = gLines[i].substr(b + 1);
      const bool existed = rest == "exists";
      if (ok != "ok") allOk = false;
      if (json) {
        string o = "{\"path\":" + jsonString(name) + ",\"ok\":" + (ok == "ok" ? "true" : "false");
        if (ok == "ok") o += string(",\"created\":") + (existed ? "false" : "true") +
                             ",\"url\":" + jsonString(existed ? urls[i] : rest);
        else o += ",\"reason\":" + jsonString(rest);
        objs.push_back(o + "}");
      } else if (ok == "ok") std::cout << name << ": " << (existed ? "exists" : "created") << std::endl;
      else std::cout << name << ": " << undone << ": " << rest << std::endl;
    }
    if (json) {
      std::cout << "[";
      for (size_t i = 0; i < objs.size(); i++) std::cout << (i ? "," : "") << objs[i];
      std::cout << "]" << std::endl;
    }
    return allOk ? 0 : 1;
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
