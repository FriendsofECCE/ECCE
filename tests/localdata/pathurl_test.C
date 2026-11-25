// LocalData::pathUrl (#1285): which paths are inside the data folder.
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

#include "util/Filesystem.H"
#include "util/LocalData.H"

namespace fs = ecce::fs;
static int failures = 0;

static void expect(const std::string& path, const std::string& root,
                   bool ok, const std::string& want)
{
  std::string url, why;
  bool got = LocalData::pathUrl(path, root, url, why);
  const std::string& out = got ? url : why;
  bool pass = got == ok && out.find(want) != std::string::npos;
  std::printf("%s %s -> %s %s\n", pass ? "ok  " : "FAIL", path.c_str(),
              got ? "url" : "refused", out.c_str());
  if (!pass) failures++;
}

int main()
{
  char tmpl[] = "/tmp/pathurl-XXXXXX";
  if (!mkdtemp(tmpl)) return 1;
  const fs::path top = fs::canonical(tmpl);
  const fs::path data = top / "data", calc = data / "users" / "local" / "p" / "c";
  fs::create_directories(calc);
  fs::create_directories(top / "data2" / "x");
  fs::create_directory_symlink(data, top / "link");
  const std::string d = data.generic_string(), t = top.generic_string();

  expect(calc.generic_string(), d, true, "file://" + calc.generic_string());
  expect(calc.generic_string() + "/", d, true, "file://" + calc.generic_string());
  expect(t + "/link/users/local/p/c", d, true, "file://" + calc.generic_string());
  expect(calc.generic_string(), t + "/link", true, "file://" + calc.generic_string());
  expect(d + "/users/../users/local", d, true, "file://" + d + "/users/local");
  expect(d, d, false, "not inside the data folder");
  expect(t + "/data2/x", d, false, "not inside the data folder");
  expect(t, d, false, "not inside the data folder");
  expect(d + "/missing", d, false, "no such folder");
  if (chdir(d.c_str()) == 0)
    expect("users/local/p/c", d, true, "file://" + calc.generic_string());

  fs::remove_all(top);
  std::printf(failures ? "FAILED\n" : "PASSED\n");
  return failures ? 1 : 0;
}
