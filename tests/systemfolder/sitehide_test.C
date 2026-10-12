// site_runtime's ECCE_HIDE_MO_DIAGRAM line is read as written (#280).
#include "util/Ecce.H"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
using std::string;

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static void writeSite(const string& dir, const char* text)
{
  FILE* f = fopen((dir + "/siteconfig/site_runtime").c_str(), "w");
  if (!f) { printf("cannot write\n"); exit(2); }
  fputs(text, f);
  fclose(f);
}

int main()
{
  char tmpl[] = "/tmp/sitehide_XXXXXX";
  if (!mkdtemp(tmpl)) return 2;
  string dir = tmpl;
  mkdir((dir + "/siteconfig").c_str(), 0755);
  setenv("ECCE_HOME", dir.c_str(), 1);
  unsetenv("ECCE_REMOTE_SERVER");

  writeSite(dir, "ECCE_STORE_TRAJECTORIES yes\n# ECCE_HIDE_MO_DIAGRAM yes\n");
  CHECK(!Ecce::ecceHideMoDiagram());
  writeSite(dir, "ECCE_STORE_TRAJECTORIES yes\nECCE_HIDE_MO_DIAGRAM yes\n");
  CHECK(Ecce::ecceHideMoDiagram());
  CHECK(!Ecce::ecceNoUpdateCheck());
  writeSite(dir, "ECCE_HIDE_MO_DIAGRAM no\n");
  CHECK(!Ecce::ecceHideMoDiagram());

  unlink((dir + "/siteconfig/site_runtime").c_str());
  rmdir((dir + "/siteconfig").c_str());
  rmdir(dir.c_str());
  if (fails == 0) printf("sitehide: ok\n");
  return fails ? 1 : 0;
}
