// Version order and release-line choice for the update check (#273).
#include "util/ReleaseCheck.H"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <string>
using std::string;

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

// The first entry nests a look-alike tag_name and braces inside a string.
static const char* kJson =
  "[{\"tag_name\":\"v9.0.0-beta.2\",\"html_url\":\"https://x/9.0.0-beta.2\",\"draft\":false,\"prerelease\":true,"
  "\"author\":{\"login\":\"a\",\"list\":[1,2,{\"tag_name\":\"v99.0.0\"}]},\"body\":\"he said \\\"hi\\\" {not json}\"},"
  "{\"tag_name\":\"v9.0.0-beta.10\",\"html_url\":\"https://x/9.0.0-beta.10\",\"draft\":false,\"prerelease\":true},"
  "{\"tag_name\":\"v9.0.0-beta.9\",\"html_url\":\"https://x/9.0.0-beta.9\",\"draft\":false,\"prerelease\":true},"
  "{\"tag_name\":\"v9.0.1\",\"html_url\":\"https://x/9.0.1\",\"draft\":true,\"prerelease\":false},"
  "{\"tag_name\":\"v8.18.10\",\"html_url\":\"https://x/8.18.10\",\"draft\":false,\"prerelease\":false},"
  "{\"tag_name\":\"v8.18.2\",\"html_url\":\"https://x/8.18.2\",\"draft\":false,\"prerelease\":false},"
  "{\"tag_name\":\"v8.19.0-rc.1\",\"html_url\":\"https://x/8.19.0-rc.1\",\"draft\":false,\"prerelease\":true}]";

static const char* kStable =
  "[{\"tag_name\":\"v9.0.0\",\"html_url\":\"https://x/9.0.0\",\"draft\":false,\"prerelease\":false},"
  "{\"tag_name\":\"v9.1.0-alpha.1\",\"html_url\":\"https://x/a1\",\"draft\":false,\"prerelease\":true},"
  "{\"tag_name\":\"v9.0.0-beta.3\",\"html_url\":\"https://x/b3\",\"draft\":false,\"prerelease\":true}]";

static bool find(const char* cur, const char* json, string& v,
                 bool previews = false)
{
  ReleaseCheck::Release r;
  bool f = ReleaseCheck::newerRelease(cur, json, r, previews);
  v = f ? r.version : "";
  return f;
}

int main()
{
  using namespace ReleaseCheck;
  CHECK(compareVersions("9.0.0-beta.10", "9.0.0-beta.9") > 0);
  CHECK(compareVersions("v9.0.0-alpha.9", "9.0.0-beta.1") < 0);
  CHECK(compareVersions("9.0.0-beta.1", "9.0.0") < 0);
  CHECK(compareVersions("9.0.0~beta.1", "9.0.0-beta.1") == 0);
  CHECK(compareVersions("8.18.10", "8.18.2") > 0);
  CHECK(compareVersions("9.0", "9.0.0") == 0);
  CHECK(compareVersions("9.0.0-beta", "9.0.0-beta.1") < 0);
  CHECK(isPreview("9.0.0-beta.1") && !isPreview("8.18.2"));

  string v;
  CHECK(find("9.0.0-beta.1", kJson, v) && v == "9.0.0-beta.10");   // newer
  CHECK(!find("9.0.0-beta.10", kJson, v));                          // same
  CHECK(!find("9.0.0-beta.11", kJson, v));                          // older
  CHECK(find("8.18.1", kJson, v) && v == "8.18.10");   // 8.x: stable 8.x only
  CHECK(!find("8.18.10", kJson, v));
  CHECK(!find("9.0.0", kJson, v));     // stable 9: no previews, no drafts
  CHECK(find("9.0.0-beta.1", kStable, v) && v == "9.1.0-alpha.1");
  CHECK(!find("9.0.0", kStable, v));
  CHECK(!find("8.18.2", kStable, v));

  // 8.x: major.minor line only
  {
    const char* j8 =
      "[{\"tag_name\":\"v8.19.0\",\"html_url\":\"u\",\"draft\":false,\"prerelease\":false},"
      "{\"tag_name\":\"v8.18.3\",\"html_url\":\"u3\",\"draft\":false,\"prerelease\":false},"
      "{\"tag_name\":\"v8.18.4-rc.1\",\"html_url\":\"u4\",\"draft\":false,\"prerelease\":true}]";
    CHECK(find("8.18.2", j8, v) && v == "8.18.3");
    CHECK(!find("8.18.3", j8, v));
    CHECK(find("8.19.0-rc.1", j8, v) == true && v == "8.19.0");
    CHECK(find("8.18.2", j8, v, true) && v == "8.18.4-rc.1");
    CHECK(!find("8.17.0", j8, v));
  }
  // "also previews" lets a stable install hear about previews in its line
  CHECK(find("9.0.0", kStable, v, true) && v == "9.1.0-alpha.1");
  CHECK(find("9.0.0-beta.1", kJson, v, true) && v == "9.0.0-beta.10");

  CHECK(!find("9.0.0", "", v));                                     // offline
  CHECK(!find("9.0.0", "not json", v));
  CHECK(!find("9.0.0", "{\"message\":\"API rate limit exceeded\"}", v));
  CHECK(!find("9.0.0", "[{\"tag_name\":", v));
  CHECK(!find("garbage", kJson, v));

  CHECK(checkDue(0, 100000));
  CHECK(!checkDue(100000, 100000 + 3600));
  CHECK(checkDue(100000, 100000 + 86400));
  CHECK(checkDue(200000, 100000));

  setenv("ECCE_NO_UPDATE_CHECK", "1", 1);
  CHECK(disabledByEnvironment());
  setenv("ECCE_NO_UPDATE_CHECK", "0", 1);
  CHECK(!disabledByEnvironment());
  unsetenv("ECCE_NO_UPDATE_CHECK");
  CHECK(!disabledByEnvironment());
  setenv("ECCE_UPDATE_CHECK_URL", "file:///tmp/x.json", 1);
  CHECK(releasesUrl() == "file:///tmp/x.json");

  // site switch: siteconfig/site_runtime under ECCE_HOME
  {
    char dir[] = "/tmp/rcheckXXXXXX";
    if (mkdtemp(dir)) {
      string d = dir;
      setenv("ECCE_HOME", dir, 1);
      unsetenv("ECCE_REMOTE_SERVER");
      string cmd = "mkdir -p " + d + "/siteconfig";
      CHECK(system(cmd.c_str()) == 0);
      FILE* f = fopen((d + "/siteconfig/site_runtime").c_str(), "w");
      fputs("# ECCE_NO_UPDATE_CHECK yes\nECCE_JOB_CHECK true\n", f);
      fclose(f);
      CHECK(!disabledBySite());
      f = fopen((d + "/siteconfig/site_runtime").c_str(), "w");
      fputs("ECCE_JOB_CHECK true\nECCE_NO_UPDATE_CHECK yes\n", f);
      fclose(f);
      CHECK(disabledBySite() && disabled());
      cmd = "rm -rf " + d;
      system(cmd.c_str());
    }
  }

  if (fails == 0) printf("releasecheck: all passed\n");
  return fails ? 1 : 0;
}
