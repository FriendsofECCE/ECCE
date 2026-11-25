// A command run from a plain Command Prompt (no ecce.cmd environment) must
// find ECCE_HOME, the session pointer file and the local data folder, and
// must get the Organizer's reply although Windows gives its subscriber no
// wake-up socket (#1287). The reply part runs everywhere.

#include <stdlib.h>
#include <sys/stat.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "util/Ecce.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/JMSSubscriber.H"
#include "util/LocalData.H"
#include "util/MqttLink.H"

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#endif

using std::string;

static int g_fail = 0;
static int g_got = 0;

static void check(bool ok, const string& what)
{
  std::cout << (ok ? "PASS  " : "FAIL  ") << what << std::endl;
  if (!ok) g_fail++;
}

static void replyCB(JMSMessage&) { g_got++; }

static string env(const char* n)
{
  const char* v = getenv(n);
  return v ? v : "";
}

#ifdef _WIN32
static void setVar(const char* name, const char* value)
{
  _putenv_s(name, value ? value : "");   // "" removes it
}

static string slashes(string p)
{
  for (auto& c : p) if (c == '\\') c = '/';
  return p;
}

static void environmentChecks()
{
  char tmp[MAX_PATH];
  GetTempPathA(sizeof(tmp), tmp);
  const string profile = slashes(tmp) + "command_env_" +
                         std::to_string(GetCurrentProcessId());
  _mkdir(profile.c_str());
  const char* vars[] = {"ECCE_HOME", "ECCE_REALUSERHOME", "ECCE_LOCAL_DATA",
                        "ECCE_REMOTE_SERVER", "ECCE_REMOTE_DIR", "ECCE_LOCAL"};
  for (auto v : vars) setVar(v, nullptr);
  setVar("USERPROFILE", profile.c_str());

  Ecce::commandEnvironment();
  char exe[4096];
  DWORD n = GetModuleFileNameA(NULL, exe, sizeof(exe));
  string bin = slashes(string(exe, n));
  bin = bin.substr(0, bin.rfind('/'));
  check(env("ECCE_HOME") == bin.substr(0, bin.rfind('/')),
        "ECCE_HOME is the folder above the program's own (" + env("ECCE_HOME") + ")");
  check(env("ECCE_REALUSERHOME") == profile, "ECCE_REALUSERHOME is USERPROFILE");
  check(Ecce::sessionPointerFile().rfind(profile + "/.ECCE/session_", 0) == 0,
        "the session pointer file is in USERPROFILE (" + Ecce::sessionPointerFile() + ")");
  check(LocalData::dir() == profile + "/ecce-local",
        "the local data folder is ecce.cmd's default (" + LocalData::dir() + ")");

  // A user who connected to a server: ecce.cmd's server session.
  for (auto v : vars) setVar(v, nullptr);
  _mkdir((profile + "/.ECCE").c_str());
  _mkdir((profile + "/.ECCE/RemoteServer").c_str());
  const string ds = profile + "/.ECCE/RemoteServer/DataServers";
  std::ofstream(ds.c_str()) << "x\n";
  Ecce::commandEnvironment();
  check(env("ECCE_REMOTE_SERVER") == "1" && env("ECCE_LOCAL_DATA").empty() &&
        env("ECCE_REMOTE_DIR") == profile + "/.ECCE/RemoteServer",
        "a user's DataServers file makes it a server session");
  remove(ds.c_str());
  _rmdir((profile + "/.ECCE/RemoteServer").c_str());
  _rmdir((profile + "/.ECCE").c_str());
  _rmdir(profile.c_str());
  for (auto v : vars) setVar(v, nullptr);
}
#endif

int main()
{
#ifdef _WIN32
  environmentChecks();
#endif
  // In-process delivery needs no broker; the subscriber is the commands'.
  MqttLink::setLocalDelivery(true);
  JMSSubscriber sub("CommandEnvTest");
  check(sub.messagingEnabled() && sub.subscribe("command_env_reply", replyCB) &&
        sub.startSubscriber(), "the reply subscriber starts");
#ifdef _WIN32
  check(sub.getSocketID() < 0, "Windows: the subscriber has no socket");
#endif
  JMSPublisher pub("CommandEnvOrganizer");
  JMSMessage* msg = pub.newMessage();
  msg->addProperty("reply_id", "1");
  pub.publish("command_env_reply", *msg);
  delete msg;
  for (int i = 0; i < 40 && !g_got; i++) {
    sub.processMessage();
    if (!g_got) std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  check(g_got == 1, "processMessage() delivers the reply without a socket");
  return g_fail ? 1 : 0;
}
