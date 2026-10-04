/*
 * urlTest url              print what EcceURL answers for a fixed set of URLs
 * urlTest server <dir>     EDSIServerCentral in local mode (ECCE_LOCAL_DATA)
 * urlTest server-default   EDSIServerCentral reading siteconfig/DataServers
 * urlTest help KEY...       the URL each help key opens
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
#include "util/BrowserHelp.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/EDSIGaussianBasisSetLibrary.H"
#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H"
#include "dsm/TGaussianBasisSet.H"
#include "dsm/TGBSConfig.H"
#include "util/EcceException.H"
#include "dsm/ResourceDescriptor.H"

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
      "file:///data/local", "file:///data/local/", "file:///data/local/users", "file:///data/local/proj",
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
    check(home.getProtocol() == "file" &&
          home.getPath() == dir + "/users/local",
          "local mode: user home is <dir>/users/local", home.toString());
    check(central.checkServer(), "local mode: checkServer");
    EcceURL root = home.getEcceRoot();
    check(root.getPath() == dir, "local mode: getEcceRoot of home", home.getEcceRoot());
    check(EcceURL("file://" + dir).isSystemFolder() && EcceURL("file://" + dir + "/users").isSystemFolder(),
          "local mode: data root and users folder are protected");
    check(!EcceURL("file://" + dir + "/proj").isSystemFolder(), "local mode: a project is not protected");

    // The rest of checkServerSetup: user area, structure library, basis
    // set library, all on a bare data directory and the install tree.
    bool setup = false;
    try { setup = central.checkServerSetup(); }
    catch (const EcceException& e) { check(false, "local mode: checkServerSetup", e.what()); }
    check(setup, "local mode: checkServerSetup succeeds on a bare data directory");
    check(central.checkDefaultGBSL(), "local mode: basis set library reachable");

    EcceURL gbsl = central.getDefaultBasisSetLibrary();
    check(gbsl.getProtocol() == "file", "basis set library is a file:// URL", gbsl.toString());
    EDSIGaussianBasisSetLibrary lib(gbsl);
    vector<TGaussianBasisSet*> sets = lib.lookup("6-31G*", TGaussianBasisSet::UnknownGBSType, "C H");
    int shells = 0;
    for (size_t i = 0; i < sets.size(); i++) shells += sets[i]->num_contracted_sets("C");
    check(!sets.empty() && shells > 0, "load 6-31G* for C H through file://",
          "sets=" + string(1, '0' + (char)sets.size()));

    EcceURL sl = central.getDefaultStructureLibrary();
    EDSI *e = EDSIFactory::getEDSI(sl);
    vector<ResourceResult> kids;
    bool listed = e->listCollection(kids);
    string firstFile;
    string pending = sl.toString();
    for (int depth = 0; depth < 4 && firstFile.empty(); depth++) {
      e->setURL(EcceURL(pending));
      vector<ResourceResult> level;
      e->listCollection(level);
      pending = "";
      for (size_t j = 0; j < level.size(); j++) {
        if (level[j].resourcetype == ResourceDescriptor::RT_DOCUMENT) {
          firstFile = level[j].url.toString();
          break;
        }
        if (pending.empty()) pending = level[j].url.toString();
      }
    }
    check(listed && !kids.empty(), "structure library lists", sl.toString());
    e->setURL(EcceURL(firstFile));
    istream *in = e->getDataSet();
    string body;
    if (in) { char c; while (in->get(c)) body += c; delete in; }
    check(!body.empty(), "read a structure file from the library", firstFile);
    delete e;
  } else if (mode == "help") {
    BrowserHelp help;
    for (int i = 2; i < argc; i++)
      cout << argv[i] << " " << help.URL(argv[i]) << endl;
    return 0;
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
