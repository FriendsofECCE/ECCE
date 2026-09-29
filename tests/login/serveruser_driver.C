// Tiny driver for Ecce::serverUser()'s resolution order and the two
// session/remembered-login setters, used by tests/login/serveruser.py.
//
//   serveruser_driver                 -- print Ecce::serverUser()
//   serveruser_driver set NAME        -- Ecce::setSessionServerUser(NAME),
//                                        then print Ecce::serverUser()
//   serveruser_driver remember NAME   -- Ecce::rememberServerUser(NAME),
//                                        then print Ecce::serverUser()
//
// Prints "ERROR: <what>" and exits 1 if Ecce's assertions fire (e.g.
// ECCE_REALUSER unset).
#include <cstdio>
#include <cstring>
#include "util/Ecce.H"
#include "util/EcceException.H"

int main(int argc, char **argv)
{
  try {
    if (argc == 3 && strcmp(argv[1], "set") == 0) {
      Ecce::setSessionServerUser(argv[2]);
    } else if (argc == 3 && strcmp(argv[1], "remember") == 0) {
      Ecce::rememberServerUser(argv[2]);
    }
    printf("%s\n", Ecce::serverUser());
    return 0;
  } catch (EcceException& e) {
    printf("ERROR: %s\n", e.what());
    return 1;
  }
}
