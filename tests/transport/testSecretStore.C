// SecretStore (#777): with a stub secret-tool that logs its argv and stdin,
// the password is only ever on stdin; and a round trip through whatever
// ECCE_SECRET_TOOL names (the real secret-tool when the wrapper script
// secretstore_real_test.sh runs this under a keyring).
//   testSecretStore stub     needs STUB_DIR (writable); the stub is made here
//   testSecretStore real     needs ECCE_SECRET_TOOL and a session bus
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

#include "comm/SecretStore.H"

static int failures = 0;
static void check(const std::string& n, bool ok)
{
  std::cout << (ok ? "ok   " : "FAIL ") << n << std::endl;
  if (!ok) failures++;
}
static std::string slurp(const std::string& p)
{
  std::ifstream in(p);
  std::stringstream s;
  s << in.rdbuf();
  return s.str();
}

int main(int argc, char** argv)
{
  const std::string mode = argc > 1 ? argv[1] : "stub";
  const std::string pw = "s3cr3t pass'word";
  SecretStore::Login l;
  l.user = "bashuser";
  l.host = "cluster.example";
  l.port = 2222;

  if (mode == "stub") {
    const char* d = getenv("STUB_DIR");
    if (!d) return 2;
    const std::string dir = d;
    const std::string tool = dir + "/secret-tool";
    {
      std::ofstream o(tool);
      o << "#!/bin/sh\n"
           "echo \"$@\" >> " << dir << "/argv.log\n"
           "case $1 in\n"
           "  store) cat > " << dir << "/stdin.log; : > " << dir << "/have;;\n"
           "  lookup) [ -e " << dir << "/have ] && printf %s \"$(cat " << dir << "/stdin.log)\" ;;\n"
           "  clear) rm -f " << dir << "/have;;\n"
           "esac\n";
    }
    chmod(tool.c_str(), 0755);
    setenv("ECCE_SECRET_TOOL", tool.c_str(), 1);
    setenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/nonexistent", 1);
    check("available with the tool and a bus", SecretStore::available());
    std::string got;
    check("nothing remembered at first", !SecretStore::lookup(l, got));
    check("save", SecretStore::save(l, pw));
    check("lookup returns the password", SecretStore::lookup(l, got) && got == pw);
    check("the password never appears in argv", slurp(dir + "/argv.log").find("s3cr3t") == std::string::npos);
    check("the password was on stdin", slurp(dir + "/stdin.log") == pw);
    check("the host and user are attributes", slurp(dir + "/argv.log").find("host cluster.example") != std::string::npos);
    check("forget", SecretStore::forget(l) && !SecretStore::lookup(l, got));
    unsetenv("DBUS_SESSION_BUS_ADDRESS");
    check("not available without a session bus", !SecretStore::available());
    setenv("HOME", dir.c_str(), 1);
    unsetenv("ECCE_REALUSERHOME");
    mkdir((dir + "/.ECCE").c_str(), 0700);
    SecretStore::noteRemembered(l);
    SecretStore::noteRemembered(l);
    check("the index lists one login, host names only",
          SecretStore::remembered().size() == 1 &&
          slurp(dir + "/.ECCE/remembered-logins") == "bashuser@cluster.example:2222\n");
    SecretStore::noteForgotten(l);
    check("and it is removed again", SecretStore::remembered().empty());
  } else {
    check("available", SecretStore::available());
    std::string got;
    SecretStore::forget(l);
    check("save", SecretStore::save(l, pw));
    check("lookup", SecretStore::lookup(l, got) && got == pw);
    check("forget", SecretStore::forget(l));
    check("gone", !SecretStore::lookup(l, got));
  }
  std::cout << (failures ? "FAILED " : "PASSED ") << failures << std::endl;
  return failures ? 1 : 0;
}
