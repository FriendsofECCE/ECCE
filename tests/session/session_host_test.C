// The session key a program computes must not depend on how it was started
// (#1287): a command run from a Command Prompt, without ecce.cmd's HOST,
// must name the same MQTT topics as the Organizer ecce.cmd started.
// Windows only; elsewhere the host name is the same either way.

#include <stdlib.h>

#include <iostream>
#include <string>

#include "util/Ecce.H"

using std::string;

static int g_fail = 0;

static void check(bool ok, const string& what)
{
  std::cout << (ok ? "PASS  " : "FAIL  ") << what << std::endl;
  if (!ok) g_fail++;
}

#ifdef _WIN32
static void setVar(const char* name, const char* value)
{
  _putenv_s(name, value ? value : "");   // "" removes it
}

int main()
{
  const char* cn = getenv("COMPUTERNAME");
  const string computer = cn ? cn : "";
  check(!computer.empty(), "COMPUTERNAME is set");
  const string id = "0123456789abcdef";

  setVar("ECCE_HOST", nullptr);
  setVar("HOST", computer.c_str());           // as ecce.cmd starts the Organizer
  const string organizer = Ecce::sessionKeyFor(id);
  setVar("HOST", nullptr);                    // a Command Prompt
  const string prompt = Ecce::sessionKeyFor(id);
  check(organizer == prompt,
        "key without HOST (" + prompt + ") is ecce.cmd's (" + organizer + ")");

  setVar("COMPUTERNAME", "EcceTestHost");
  check(Ecce::sessionHost() == "EcceTestHost",
        "no HOST: the host is COMPUTERNAME (" + Ecce::sessionHost() + ")");
  setVar("HOST", "other");
  check(Ecce::sessionHost() == "other", "HOST still comes first");
  setVar("ECCE_HOST", "first");
  check(Ecce::sessionHost() == "first", "ECCE_HOST comes before HOST");
  setVar("COMPUTERNAME", computer.c_str());

  std::cout << (g_fail ? "FAILED" : "all passed") << std::endl;
  return g_fail ? 1 : 0;
}
#else
int main()
{
  std::cout << "Windows only" << std::endl;
  return 77;
}
#endif
