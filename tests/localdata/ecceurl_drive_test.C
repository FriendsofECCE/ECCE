// EcceURL with a Windows drive in a file URL (#1289): the commands send
// "file:///C:/...", the Organizer's tree holds "file://C:/...". The drive's
// colon must survive parsing and both spellings must name the same node.
#include <cstdio>
#include <cstdlib>
#include <string>

#include "util/EcceURL.H"
#include "util/Filesystem.H"
#include "util/LocalData.H"

namespace fs = ecce::fs;
static int failures = 0;

static void check(bool ok, const std::string& what)
{
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) failures++;
}

static void parts(const std::string& in, const std::string& host, int port,
                  const std::string& file)
{
  EcceURL u(in);
  check(u.getHost() == host && u.getPort() == port && u.getFile() == file,
        in + " -> host '" + u.getHost() + "' port " +
        std::to_string(u.getPort()) + " file '" + u.getFile() + "'");
}

int main()
{
  // A ':' in the path is not a port.
  const std::string drive = "file:///C:/Users/s/ecce-local/users/local/p/c";
  const std::string back = EcceURL(drive).toString();
  check(back.find("C:/Users/s") != std::string::npos,
        drive + " -> " + back + " keeps the drive colon");
  parts("file:///home/a:b/c", "", -1, "/home/a:b/c");
  // Ports in the authority still parse.
  parts("http://host:8080/a/b", "host", 8080, "/a/b");
  parts("http://host:8080", "host", 8080, "");
  parts("sftp://u@h:2222/folder", "u@h", 2222, "/folder");
  parts("file:///home/u/x", "", -1, "/home/u/x");

#ifdef _WIN32
  const EcceURL slash3(drive), slash2("file://C:/Users/s/ecce-local/users/local/p/c");
  check(slash3 == slash2, slash3.toString() + " == " + slash2.toString());
  check(slash3.isChildOf(EcceURL("file://C:/Users/s/ecce-local")),
        "file:///C:/... is below the tree's file://C:/... root");
#endif

  // What a command sends for a path must find the node the tree made for it
  // (the tree's URLs are "file://" + the data folder + the rest).
  const fs::path top = fs::temp_directory_path() / "ecceurl-drive-test";
  const fs::path calc = top / "users" / "local" / "p" / "c";
  fs::create_directories(calc);
  const std::string root = fs::canonical(top).generic_string();
  std::string url, why;
  if (LocalData::pathUrl(calc.string(), root, url, why)) {
    const EcceURL sent(url), tree("file://" + root + "/users/local/p/c");
    check(sent == tree, url + " == " + tree.toString());
    check(sent.isChildOf(EcceURL("file://" + root)),
          url + " is below file://" + root);
  } else {
    check(false, "pathUrl refused: " + why);
  }
  fs::remove_all(top);

  std::printf(failures ? "FAILED\n" : "PASSED\n");
  return failures ? 1 : 0;
}
