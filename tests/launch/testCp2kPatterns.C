// Launch::cp2kCopyBackPatterns: the copy-back wildcard a CP2K deck's
// &PDOS section asks for, matched case-insensitively (#720).
//
//   &PDOS / &pdos / &Pdos  -> *.pdos
//
// The section must start a line: a "!" comment or a keyword that merely
// contains the name does not count, and a deck without the section yields "".

#include <iostream>
#include <string>

#include "comm/Launch.H"

using namespace std;

static int failures = 0;

static void check(const char* name, bool ok)
{
  cout << (ok ? "ok   " : "FAIL ") << name << endl;
  if (!ok) failures++;
}

int main()
{
  check("&PDOS", Launch::cp2kCopyBackPatterns("&PDOS") == "*.pdos");
  check("&pdos", Launch::cp2kCopyBackPatterns("&pdos") == "*.pdos");
  check("&Pdos", Launch::cp2kCopyBackPatterns("&Pdos") == "*.pdos");
  check("&PDOS mixed case",
        Launch::cp2kCopyBackPatterns("&PDoS") == "*.pdos");
  check("a commented-out section",
        Launch::cp2kCopyBackPatterns("! &PDOS\n&END\n") == "");
  check("a keyword that contains the name",
        Launch::cp2kCopyBackPatterns("&PDOS_EXTRA\n") == "");
  check("section with trailing text",
        Launch::cp2kCopyBackPatterns("&PDOS  NLUMO 40\n") == "*.pdos");
  check("no section", Launch::cp2kCopyBackPatterns(
            "&FORCE_EVAL\n  &PRINT\n  &END PRINT\n&END FORCE_EVAL\n") == "");
  check("a full deck",
        Launch::cp2kCopyBackPatterns(
            "&FORCE_EVAL\n  &PRINT\n    &pdos\n    &END PDOS\n"
            "  &END PRINT\n&END FORCE_EVAL\n") == "*.pdos");
  check("another code's deck",
        Launch::cp2kCopyBackPatterns("%pal\n! RKS\n* xyz 0 1\n*\n") == "");
  return failures ? 1 : 0;
}
