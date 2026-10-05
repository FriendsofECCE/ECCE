// Which exceptions print "Throw Log" to stderr where they are thrown:
// a fault type always, a user-facing one only with ECCE_THROW_LOG set.
//   throwlog_test        -- default
//   throwlog_test all    -- with ECCE_THROW_LOG=1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>

#include "util/EcceException.H"
#include "util/InternalException.H"
#include "util/InvalidException.H"
#include "util/NullPointerException.H"

static int failures = 0;

template <class T>
static void expect(const char *what, bool logged, const T& make)
{
  std::ostringstream err;
  std::streambuf *old = std::cerr.rdbuf(err.rdbuf());
  try { make(); } catch (EcceException&) {}
  std::cerr.rdbuf(old);
  bool got = err.str().find("Throw Log") != std::string::npos;
  printf("%s  %s: %s\n", got == logged ? "ok  " : "FAIL", what,
         got ? "logged" : "silent");
  if (got != logged) failures++;
}

int main(int argc, char **argv)
{
  bool all = argc > 1 && strcmp(argv[1], "all") == 0;
  if (all) setenv("ECCE_THROW_LOG", "1", 1);
  else unsetenv("ECCE_THROW_LOG");

  expect("EcceException (user message)", all, [] {
    throw EcceException("Please select the sites that should be bonded.",
                        __FILE__, __LINE__);
  });
  expect("InvalidException", all, [] {
    throw InvalidException("Null Resource", __FILE__, __LINE__);
  });
  expect("InternalException", true, [] {
    throw InternalException("Cannot down cast", __FILE__, __LINE__);
  });
  expect("NullPointerException", true, [] {
    throw NullPointerException("null", __FILE__, __LINE__);
  });
  return failures ? 1 : 0;
}
