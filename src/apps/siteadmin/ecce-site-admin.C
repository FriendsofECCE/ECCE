// ecce-site-admin: the server half of "ecce -admin" on a -remote client.
//
//   ecce-site-admin apply FILE    apply a request (SiteRequest) and publish
//   ecce-site-admin check         say whether this login may change siteconfig
//
// Run over ssh as the administrator's own login.  The request comes in a
// file, never on the command line, so nothing in it reaches a shell.  The
// last line printed is "ecce-site-admin: ok" or "ecce-site-admin: error: ...".

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

#include "tdat/SiteRequest.H"

static int finish(int status, const std::string& report)
{
  fflush(stdout);
  if (status == 0)
    printf("%s\necce-site-admin: ok\n", report.c_str());
  else
    printf("ecce-site-admin: error: %s\n", report.c_str());
  return status == 0 ? 0 : 1;
}

int main(int argc, char** argv)
{
  // New site files stay writable by the group that administers siteconfig.
  umask(002);

  const char* home = getenv("ECCE_HOME");
  if (home == NULL || *home == '\0')
    return finish(1, "ECCE_HOME is not set (run the ecce-site-admin on PATH)");

  if (argc == 2 && strcmp(argv[1], "check") == 0) {
    std::string dir = std::string(home) + "/siteconfig";
    if (access(dir.c_str(), W_OK) == 0)
      return finish(0, "You may change the site settings in " + dir + ".");
    // The same explanation a refused change gets.
    SiteRequest r;
    r.machine = "check";
    std::string report;
    int st = r.applyOnServer(home, report);
    return finish(st == 0 ? 1 : st, report);
  }

  if (argc != 3 || strcmp(argv[1], "apply") != 0) {
    fprintf(stderr, "usage: ecce-site-admin apply FILE | check\n");
    return 2;
  }

  std::ifstream in(argv[2], std::ios::binary);
  if (!in)
    return finish(1, std::string("cannot read ") + argv[2]);
  std::stringstream data;
  data << in.rdbuf();

  SiteRequest r;
  std::string err;
  if (!r.decode(data.str(), err))
    return finish(1, "the request was refused: " + err);

  std::string report;
  int status = r.applyOnServer(home, report);
  return finish(status, report);
}
