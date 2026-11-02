// AuthCache's LAST_URL rule (the job store) must never offer an ssh password
// to a host, port or user it was not cached for (#791): the exact entry or
// nothing, so the login asks.  The data server's realm-free lookup is kept.
//
//   authcache_host_test <source dir>

#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "tdat/AuthCache.H"

using namespace std;

static int g_fail = 0;
static void check(bool ok, const string& what)
{
  cout << (ok ? "PASS  " : "FAIL  ") << what << endl;
  if (!ok) g_fail++;
}

static string lookup(const string& url, const string& user)
{
  BasicAuth* ba = AuthCache::getCache().getAuthentication(url, user, "", 1);
  string pass = ba ? ba->m_pass : "";
  delete ba;
  return pass;
}

int main(int argc, char** argv)
{
  char dir[] = "/tmp/authcache-host-XXXXXX";
  if (!mkdtemp(dir)) return 1;
  setenv("HOME", dir, 1);
  setenv("ECCE_REALUSERHOME", dir, 1);
  if (argc >= 2) setenv("ECCE_HOME", argv[1], 1);
  setenv("ECCE_NO_MESSAGING", "1", 1);
  setenv("ECCE_AUTHCACHE_NO_BROADCAST", "1", 1);
  unsetenv("ECCE_SESSION_ID");

  AuthCache& c = AuthCache::getCache();
  for (AuthCache::URLPolicy p : {AuthCache::LAST_URL, AuthCache::BEST_URL}) {
    const string tag = p == AuthCache::LAST_URL ? "LAST_URL: " : "BEST_URL: ";
    c.flushCache();
    c.setURLPolicy(p);
    c.addAuthentication("ssh://hosta.example.org", "alice", "pw-a", "", false);
    c.addAuthentication("ssh://hostb.example.org", "alice", "pw-b", "", false);
    c.addAuthentication("ssh://hosta.example.org:2222", "alice", "pw-a2222",
                        "", false);

    check(lookup("ssh://hosta.example.org", "alice") == "pw-a",
          tag + "host A gets its own password");
    check(lookup("ssh://hostb.example.org", "alice") == "pw-b",
          tag + "host B gets its own password, never A's");
    check(lookup("ssh://hosta.example.org:2222", "alice") == "pw-a2222",
          tag + "host A on port 2222 gets that port's password");
    c.remove("ssh://hostb.example.org", "alice", "");
    check(lookup("ssh://hostb.example.org", "alice") == "",
          tag + "host B with nothing cached gets nothing, not A's");
    check(lookup("ssh://hostc.example.org", "alice") == "",
          tag + "an unknown host gets nothing");
    check(lookup("ssh://hosta.example.org", "bob") == "",
          tag + "host A as another user gets nothing");
    check(lookup("ssh://hosta.example.org:2200", "alice") == "",
          tag + "host A on another port gets nothing");
  }

  // The job store still reaches its data server whatever realm it asks with.
  c.flushCache();
  c.setURLPolicy(AuthCache::LAST_URL);
  c.addAuthentication("ssh://hosta.example.org", "alice", "pw-a", "", false);
  c.addAuthentication("http://localhost:1/", "alice", "pw-data", "\"Ecce\"",
                      false);
  check(lookup("http://localhost:1/Ecce/users/alice/", "alice") == "pw-data",
        "LAST_URL: the data server's password is found without its realm");
  c.flushCache();
  c.addAuthentication("http://localhost:1/", "alice", "pw-data", "", false);
  check(lookup("ssh://hostb.example.org", "alice") == "",
        "LAST_URL: an ssh host never gets the data server's password");

  string cmd = string("rm -rf '") + dir + "'";
  if (system(cmd.c_str()) != 0) cerr << "could not remove " << dir << endl;
  cout << (g_fail ? "FAILED" : "ALL PASSED") << endl;
  return g_fail ? 1 : 0;
}
