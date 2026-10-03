/*
 * urlTest url              print what EcceURL answers for a fixed set of URLs
 * urlTest server <dir>     EDSIServerCentral in local mode (ECCE_LOCAL_DATA)
 * urlTest server-default   EDSIServerCentral reading siteconfig/DataServers
 *
 * "url" prints "<url> <function> <result>" lines; run_tests.py runs it
 * against the libraries as they were before the file:// work and as they
 * are now, and requires the http lines to be identical.
 */
#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include "util/EcceURL.H"
#include "dsm/EDSIServerCentral.H"

static int failures = 0;
static void check(bool ok, const string& name, const string& why = "")
{
  if (ok) cout << "PASS " << name << endl;
  else { cout << "FAIL " << name << ": " << why << endl; failures++; }
}

int main(int argc, char **argv)
{
  string mode = argc > 1 ? argv[1] : "";
  if (mode == "url") {
    const char *urls[] = {
      "http://h:8096/Ecce", "http://h:8096/Ecce/", "http://h:8096/Ecce/users",
      "http://h:8096/Ecce/users/tester", "http://h:8096/Ecce/users/tester/proj",
      "http://h:8096/Ecce/system/x", "http://h:8096/Ecce/share/StructureLibrary",
      "http://h:8096/other", "https://h/Ecce/users/tester/p/calc",
      "file:///data/local", "file:///data/local/", "file:///data/local/proj",
      "file:///data/localx", "file:///elsewhere/proj", 0 };
    for (int i = 0; urls[i]; i++) {
      EcceURL u(urls[i]);
      cout << urls[i] << " isSystemFolder " << u.isSystemFolder() << endl;
      cout << urls[i] << " getEcceRoot [" << u.getEcceRoot() << "]" << endl;
      cout << urls[i] << " isEcceRoot " << u.isEcceRoot() << endl;
    }
    return 0;
  }
  if (mode == "server" && argc > 2) {
    string dir = argv[2];
    EDSIServerCentral central;       // no siteconfig/DataServers in ECCE_HOME
    EcceURL home = central.getDefaultUserHome();
    check(home.getProtocol() == "file" && home.getPath() == dir,
          "local mode: user home is the data directory", home.toString());
    check(central.checkServer(), "local mode: checkServer");
    EcceURL root = home.getEcceRoot();
    check(root.getPath() == dir, "local mode: getEcceRoot of home", home.getEcceRoot());
    check(home.isSystemFolder(), "local mode: data root is protected");
    check(!EcceURL(dir + "/proj").isSystemFolder(), "local mode: a project is not protected");
  } else if (mode == "server-default") {
    EDSIServerCentral central;
    EcceURL home = central.getDefaultUserHome();
    cout << "home " << home.toString() << endl;
    check(home.getProtocol().find("http") == 0, "default mode: http server from DataServers",
          home.toString());
    string tail = string("/users/") + getenv("ECCE_REALUSER");
    check(home.getPath().size() >= tail.size() &&
          home.getPath().compare(home.getPath().size() - tail.size(), tail.size(), tail) == 0,
          "default mode: user home under /users/<user>", home.getPath());
  } else {
    return 2;
  }
  cout << (failures ? "FAILED " : "ALL OK ") << failures << " failure(s)" << endl;
  return failures ? 1 : 0;
}
