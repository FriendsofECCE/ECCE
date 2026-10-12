// The Organizer's "Local data" node must count as a system folder however
// its path is spelled (#288).
#include "util/EcceURL.H"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
using std::string;

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main()
{
  char tmpl[] = "/tmp/systemfolder_XXXXXX";
  if (!mkdtemp(tmpl)) return 2;
  string dir = tmpl;
  mkdir((dir + "/users").c_str(), 0755);
  mkdir((dir + "/users/local").c_str(), 0755);
  mkdir((dir + "/users/local/proj").c_str(), 0755);
  string link = dir + "_link";
  if (symlink(dir.c_str(), link.c_str()) != 0) return 2;
  setenv("ECCE_LOCAL_DATA", dir.c_str(), 1);

  CHECK(EcceURL("file://" + dir).isSystemFolder());
  CHECK(EcceURL("file://" + dir + "/users").isSystemFolder());
  CHECK(EcceURL("file://" + dir + "/users/local").isSystemFolder());
  CHECK(EcceURL("file://" + dir + "/users/local/").isSystemFolder());
  CHECK(EcceURL("file://" + link + "/users/local").isSystemFolder());
  CHECK(EcceURL("file://" + dir + "/users/local").isLocalDataHome());
  CHECK(!EcceURL("file://" + dir + "/users/local/proj").isSystemFolder());
  CHECK(!EcceURL("file://" + dir + "/users/local/proj").isLocalDataHome());

  unlink(link.c_str());
  rmdir((dir + "/users/local/proj").c_str());
  rmdir((dir + "/users/local").c_str());
  rmdir((dir + "/users").c_str());
  rmdir(dir.c_str());
  if (fails == 0) printf("systemfolder: ok\n");
  return fails ? 1 : 0;
}
