// AuthCache's LAST_URL fallback must not answer a data server lookup with a
// compute machine's (ssh://) password, which RCommand caches ahead of it.
#include <cstdlib>
#include <iostream>
#include <string>
#include "tdat/AuthCache.H"
using namespace std;

static string ask(const string& url) {
  BasicAuth *a = AuthCache::getCache().getAuthentication(url, "u", "realm", 0);
  string p = a ? a->m_pass : "";
  delete a;
  return p;
}

int main() {
  setenv("ECCE_REALUSERHOME", "/nonexistent-authcache-test", 1);
  AuthCache& c = AuthCache::getCache();
  c.setURLPolicy(AuthCache::LAST_URL);
  c.addAuthentication("http://dataserver/Ecce/other", "u", "datapw", "realm", false);
  c.addAuthentication("ssh://cluster", "u", "clusterpw", "", false);
  int bad = 0;
  string d = ask("http://dataserver/Ecce/elsewhere");
  if (d == "clusterpw") { cout << "FAIL: data server got the cluster password\n"; bad++; }
  else if (d != "datapw") { cout << "FAIL: data server got '" << d << "'\n"; bad++; }
  string s = ask("ssh://cluster2");
  if (s == "datapw") { cout << "FAIL: ssh machine got the data server password\n"; bad++; }
  cout << (bad ? "FAILED" : "PASSED") << endl;
  return bad;
}
