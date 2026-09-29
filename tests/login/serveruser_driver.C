// Tiny driver for Ecce::serverUser()'s resolution order, used by
// tests/login/serveruser.py. Prints the resolved name and exits 0, or
// prints "ERROR: <what>" and exits 1 if Ecce's assertions fire (e.g.
// ECCE_REALUSER unset).
#include <cstdio>
#include "util/Ecce.H"
#include "util/EcceException.H"

int main()
{
  try {
    printf("%s\n", Ecce::serverUser());
    return 0;
  } catch (EcceException& e) {
    printf("ERROR: %s\n", e.what());
    return 1;
  }
}
